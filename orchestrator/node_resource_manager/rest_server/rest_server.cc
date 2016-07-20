#include "rest_server.h"
#include "../database_manager/SQLite/SQLiteManager.h"

GraphManager *RestServer::gm = NULL;
SQLiteManager *dbmanager = NULL;

SecurityManager *secmanager = NULL;

bool client_auth = false;

bool RestServer::init(SQLiteManager *dbm, bool cli_auth, map<string,string> &boot_graphs,int core_mask,set<string> physical_ports, string un_address, bool orchestrator_in_band, char *un_interface, char *ipsec_certificate, string name_resolver_ip, int name_resolver_port)
{

	try
	{
		gm = new GraphManager(core_mask,physical_ports,un_address,orchestrator_in_band,string(un_interface),string(ipsec_certificate), name_resolver_ip, name_resolver_port);

	} catch (...) {
		return false;
	}

	client_auth = cli_auth;

	if (client_auth) {
		dbmanager = dbm;
		dbmanager->cleanTables();
		secmanager = new SecurityManager(dbmanager);
	}

	sleep(2); //XXX This give time to the controller to be initialized

	//Handle the file containing the graphs to be deployed
	for(map<string,string>::iterator iter = boot_graphs.begin(); iter!=boot_graphs.end(); iter++)
	{
		if (!readGraphFromFile(iter->first,iter->second)) {
			delete gm;
			return false;
		}
	}

	return true;
}

bool RestServer::readGraphFromFile(const string &nffgResourceName, string &nffgFileName) {
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__,
			"Considering the graph described in file '%s'", nffgFileName.c_str());

	std::ifstream file;
	file.open(nffgFileName.c_str());
	if (file.fail()) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,
				"Cannot open the file %s", nffgFileName.c_str());
		return false;
	}

	stringstream stream;
	string str;
	while (std::getline(file, str))
		stream << str << endl;

	if (createGraphFromFile(nffgResourceName,stream.str()) == 0)
		return false;

	return true;
}

void RestServer::terminate() {
	delete (gm);
}

bool RestServer::isLoginRequest(const char *method, const char *url) {
	/*
	 * Checking method name and url is enough because the REST server
	 * already verifies that the request is well-formed.
	 */
	return (strcmp(method, POST) == 0 && url[0] == '/'
			&& strncmp(url + sizeof(char), BASE_URL_LOGIN,
					sizeof(char) * strlen(BASE_URL_LOGIN)) == 0);
}

void RestServer::request_completed(void *cls, struct MHD_Connection *connection,
		void **con_cls, enum MHD_RequestTerminationCode toe) {
	struct connection_info_struct *con_info =
			(struct connection_info_struct *) (*con_cls);

	if (NULL == con_info)
		return;

	if (con_info->length != 0) {
		free(con_info->message);
		con_info->message = NULL;
	}

	free(con_info);
	*con_cls = NULL;
}

int RestServer::answer_to_connection(void *cls,
		struct MHD_Connection *connection, const char *url, const char *method,
		const char *version, const char *upload_data, size_t *upload_data_size,
		void **con_cls) {

	if (NULL == *con_cls) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "New %s request for %s using version %s", method, url, version);

		if (LOGGING_LEVEL <= ORCH_DEBUG)
			MHD_get_connection_values(connection, MHD_HEADER_KIND, &print_out_key, NULL);

		struct connection_info_struct *con_info;
		con_info = (struct connection_info_struct*) malloc(
				sizeof(struct connection_info_struct));

		assert(con_info != NULL);
		if (NULL == con_info)
			return MHD_NO;

		if ((0 == strcmp(method, PUT)) || (0 == strcmp(method, POST))
				|| (0 == strcmp(method, DELETE))) {
			con_info->message = (char*) malloc(REQ_SIZE * sizeof(char));
			con_info->length = 0;
		} else if (0 == strcmp(method, GET))
			con_info->length = 0;
		else {
			con_info->message = (char*) malloc(REQ_SIZE * sizeof(char));
			con_info->length = 0;
		}

		*con_cls = (void*) con_info;
		return MHD_YES;
	}

	// Process request

	if (0 == strcmp(method, GET))
		return doOperation(connection, con_cls, GET, url);

	if ((0 == strcmp(method, PUT)) || (0 == strcmp(method, POST))
			|| (0 == strcmp(method, DELETE))) {

		struct connection_info_struct *con_info =
				(struct connection_info_struct *) (*con_cls);
		assert(con_info != NULL);
		if (*upload_data_size != 0) {
			strcpy(&con_info->message[con_info->length], upload_data);
			con_info->length += *upload_data_size;
			*upload_data_size = 0;
			return MHD_YES;

		} else if (NULL != con_info->message) {
			con_info->message[con_info->length] = '\0';
			if (0 == strcmp(method, PUT))
				return doOperation(connection, con_cls, PUT, url);
			else if (0 == strcmp(method, POST))
				return doOperation(connection, con_cls, POST, url);
			else
				return doOperation(connection, con_cls, DELETE, url);
		}
	} else {
		// Methods not implemented
		struct connection_info_struct *con_info =
				(struct connection_info_struct *) (*con_cls);
		assert(con_info != NULL);
		if (*upload_data_size != 0) {
			strcpy(&con_info->message[con_info->length], upload_data);
			con_info->length += *upload_data_size;
			*upload_data_size = 0;
			return MHD_YES;
		} else {
			con_info->message[con_info->length] = '\0';
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method \"%s\" not implemented",method);
			return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
		}
	}

	//Just to remove a warning in the compiler
	return MHD_YES;
}

int RestServer::print_out_key(void *cls, enum MHD_ValueKind kind,
		const char *key, const char *value) {
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "%s: %s", key, value);
	return MHD_YES;
}

int RestServer::login(struct MHD_Connection *connection, void **con_cls) {

	int ret = 0, rc = 0;
	struct MHD_Response *response;
	char username[BUFFER_SIZE], password[BUFFER_SIZE];
	unsigned char hash_token[HASH_SIZE], temp[BUFFER_SIZE];
	char hash_pwd[BUFFER_SIZE], nonce[BUFFER_SIZE], timestamp[BUFFER_SIZE], tmp[BUFFER_SIZE], user_tmp[BUFFER_SIZE];

	struct connection_info_struct *con_info = (struct connection_info_struct *) (*con_cls);
	assert(con_info != NULL);

	if (dbmanager == NULL) {
		con_info->message[con_info->length] = '\0';
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Login can be performed only if authentication is required.");
		return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User login");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s", con_info->message);

	if (MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host") == NULL) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
	}

	if (!parsePostBody(*con_info, username, password)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Login error: Malformed content");
		return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
	}

	try {

		SHA256((const unsigned char*) password, strlen(password), hash_token);

		strcpy(tmp, "");
		strcpy(hash_pwd, "");
		strcpy(nonce, "");

		for (int i = 0; i < HASH_SIZE; i++) {
			sprintf(tmp, "%.2x", hash_token[i]);
			strcat(hash_pwd, tmp);
		}

		strcpy(user_tmp, username);

		if(!dbmanager->userExists(user_tmp, hash_pwd)) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Login failed: wrong username or password!");
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}

		if(dbmanager->isLogged(user_tmp)) {
			char *token = NULL;
			user_info_t *pUI = NULL;

			pUI = dbmanager->getLoggedUserByName(user_tmp);
			token = pUI->token;

			response = MHD_create_response_from_buffer (strlen(token),(void*) token, MHD_RESPMEM_PERSISTENT);
			MHD_add_response_header (response, "Content-Type",TOKEN_TYPE);
			MHD_add_response_header (response, "Cache-Control",NO_CACHE);
			ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);

			return ret;
		}

		rc = RAND_bytes(temp, sizeof(temp));
		if (rc != 1) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while generating nonce!");
			return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
		}

		strcpy(tmp, "");
		strcpy(hash_pwd, "");

		for (int i = 0; i < HASH_SIZE; i++) {
			sprintf(tmp, "%.2x", temp[i]);
			strcat(nonce, tmp);
		}

		time_t now = time(0);
		tm *ltm = localtime(&now);

		strcpy(timestamp, "");
		sprintf(timestamp, "%d/%d/%d %d:%d", ltm->tm_mday, 1 + ltm->tm_mon, 1900 + ltm->tm_year, ltm->tm_hour, 1 + ltm->tm_min);

		// Insert login information into the database
		dbmanager->insertLogin(user_tmp, nonce, timestamp);

		response = MHD_create_response_from_buffer (strlen((char *)nonce),(void*) nonce, MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Content-Type",TOKEN_TYPE);
		MHD_add_response_header (response, "Cache-Control",NO_CACHE);
		ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);

		return ret;

	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during user login!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::createUser(char *username, struct MHD_Connection *connection, connection_info_struct *con_info) {
	char *group, *password;

	unsigned char *hash_token = new unsigned char[HASH_SIZE]();
	char *hash_pwd = new char[BUFFER_SIZE]();
	char *tmp = new char[HASH_SIZE]();
	char *pwd = new char[HASH_SIZE]();

	assert(con_info != NULL);

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User creation:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s", con_info->message);

	if (MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host") == NULL) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
	}

	if (!parsePostBody(*con_info, NULL, &password, &group)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Create user error: Malformed content");
		return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
	}

	char *t_group = new char[strlen(group)+1]();
	strncpy(t_group, group, strlen(group));

	try {
		if (username == NULL || group == NULL || password == NULL) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Client unathorized!");
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}

		strncpy(pwd, password, strlen(password));

		SHA256((const unsigned char*)pwd, strlen(pwd), hash_token);

		for (int i = 0; i < HASH_SIZE; i++) {
			sprintf(tmp, "%.2x", hash_token[i]);
			strcat(hash_pwd, tmp);
		}

		if(dbmanager->userExists(username, hash_pwd)) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "User creation failed: already existing!");
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}

		if(!dbmanager->resourceExists(BASE_URL_GROUP, t_group)) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "User creation failed! The group '%s' cannot be recognized!", t_group);
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}

		char *token = (char *) MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Auth-Token");

		if(token == NULL) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Token\" header not present in the request");
			return false;
		}

		user_info_t *creator = dbmanager->getUserByToken(token);

		dbmanager->insertUser(username, hash_pwd, t_group);

		dbmanager->insertResource(BASE_URL_USER, username, creator->user);

		/* TODO: This is just a provisional solution for handling
		 * user creation permissions for those users that are
		 * dynamically created via a POST request
		 */
		dbmanager->insertUserCreationPermission(username, BASE_URL_GRAPH, ALLOW);
		dbmanager->insertUserCreationPermission(username, BASE_URL_USER, ALLOW);
		dbmanager->insertUserCreationPermission(username, BASE_URL_GROUP, ALLOW);

		delete t_group;

		return httpResponse(connection, MHD_HTTP_ACCEPTED);

	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during user login!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}


bool RestServer::parsePostBody(struct connection_info_struct &con_info,
		char *user, char *pwd) {
	Value value;
	read(con_info.message, value);
	return parseLoginForm(value, user, pwd);
}

bool RestServer::parsePostBody(struct connection_info_struct &con_info,
		char **user, char **pwd, char **group) {
	Value value;
	read(con_info.message, value);
	return parseUserCreationForm(value, pwd, group);
}

bool RestServer::parseLoginForm(Value value, char *user, char *pwd) {
	try {
		Object obj = value.getObject();

		bool foundUser = false, foundPwd = false;

		//Identify the flow rules
		for (Object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			const string& name = i->first;
			const Value& value = i->second;

			if (name == USER) {
				foundUser = true;
				strcpy(user,value.getString().c_str());
			} else if (name == PASS) {
				foundPwd = true;
				strcpy(pwd, value.getString().c_str());
			} else {
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
						"Invalid key: %s", name.c_str());
				return false;
			}
		}
		if (!foundUser) {
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"Key \"%s\" not found", USER);
			return false;
		} else if (!foundPwd) {
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"Key \"%s\" not found", PASS);
			return false;
		}
	} catch (exception& e) {
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"The content does not respect the JSON syntax: ", e.what());
		return false;
	}

	return true;
}

bool RestServer::parseUserCreationForm(Value value, char **pwd, char **group) {
	try {
		Object obj = value.getObject();

		bool foundPwd = false, foundGroup = false;

		//Identify the flow rules
		for (Object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			const string& name = i->first;
			const Value& value = i->second;

			if (name == PASS) {
				foundPwd = true;
				(*pwd) = (char *) value.getString().c_str();
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\tPwd: %s", *pwd);

			} else if (name == GROUP) {
				foundGroup = true;
				(*group) = (char *) value.getString().c_str();
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\tGrp: %s", *group);
			} else {
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
						"Invalid key: %s", name.c_str());
				return false;
			}
		}

		if (!foundPwd) {
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"Key \"%s\" not found", PASS);
			return false;
		} else if (!foundGroup) {
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"Key \"%s\" not found", GROUP);
		}
	} catch (exception& e) {
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"The content does not respect the JSON syntax: ", e.what());
		return false;
	}

	return true;
}


int RestServer::createGraphFromFile(const string &graphID, string toBeCreated) {

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Graph ID: %s", graphID.c_str());
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Graph content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",
			toBeCreated.c_str());

	highlevel::Graph *graph = new highlevel::Graph(graphID);

	if (!parseGraphFromFile(toBeCreated, *graph, true)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed content");
		return 0;
	}

	graph->print();
	try {
		if (!gm->newGraph(graph)) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__,
					"The graph description is not valid!");
			return 0;
		}
		// If security is required, update database
		if(dbmanager != NULL)
			dbmanager->insertResource(BASE_URL_GRAPH, graphID.c_str(), ADMIN);
	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__,
				"An error occurred during the creation of the graph!");
		return 0;
	}

	return 1;
}

bool RestServer::parseGraphFromFile(string toBeCreated, highlevel::Graph &graph,
		bool newGraph) //startup. cambiare nome alla funzione
		{
	Value value;
	read(toBeCreated, value);
	return GraphParser::parseGraph(value, graph, newGraph, gm);
}

bool RestServer::parsePutBody(struct connection_info_struct &con_info,
		highlevel::Graph &graph, bool newGraph) {
	Value value;
	read(con_info.message, value);
	return GraphParser::parseGraph(value, graph, newGraph, gm);
}

/*
 * Version on generic resources
 */
int RestServer::doOperationOnResource(struct MHD_Connection *connection, struct connection_info_struct *con_info, user_info_t *usr, const char *method, const char *generic_resource) {

	// If security is required, check that the generic resource exist
	if (dbmanager != NULL && !dbmanager->resourceExists(generic_resource)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", generic_resource);
		return httpResponse(connection, MHD_HTTP_NOT_FOUND);
	}

	if(strcmp(method, GET) == 0) {
		// If security is required, check READ permission on the generic resource
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, generic_resource)) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User not authorized to execute %s \"%s\"", method, generic_resource);
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}

		// Case currently implemented: read a graph
		if (strcmp(generic_resource, BASE_URL_GRAPH) == 0)
			return readMultipleGraphs(connection, usr);
		else if (strcmp(generic_resource, BASE_URL_USER) == 0)
			return readMultipleUsers(connection, usr);
		else if (strcmp(generic_resource, BASE_URL_GROUP) == 0)
			return readMultipleGroups(connection, usr);

		return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
	} else if(strcmp(method, PUT) == 0) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "There are no operations using PUT with generic resources");
		return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
	} else if(strcmp(method, DELETE) == 0) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "There are no operations using DELETE with generic resources");
		return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
	} else if(strcmp(method, POST) == 0) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "There are no operations using POST with generic resources");
		return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method %s is currently not supported to operate on generic resources", method);
	return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
}

/*
 * Version for single resources
 */
int RestServer::doOperationOnResource(struct MHD_Connection *connection, struct connection_info_struct *con_info, user_info_t *usr, const char *method, const char *generic_resource, const char *resource) {

	// GET: can be only read... at the moment!
	if(strcmp(method, GET) == 0) {

		// If security is required, check READ authorization
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, generic_resource, resource)) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User not authorized to execute %s on %s", method, resource);
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}

		// Cases currently implemented
		if(strcmp(generic_resource, BASE_URL_GRAPH) == 0)
			return readGraph(connection, (char *) resource);
		else if(strcmp(generic_resource, BASE_URL_USER) == 0)
			return readUser(connection, (char *) resource);

	// PUT: for single resource, it can be only creation... at the moment!
	} else if(strcmp(method, PUT) == 0) {

		// If security is required, check CREATE authorization
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _CREATE, generic_resource, resource)) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "User not authorized to execute %s on %s", method, resource);
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}
		if(strcmp(generic_resource, BASE_URL_GRAPH) == 0)
			return deployNewGraph(connection, con_info, (char *) resource, usr);
		else if(strcmp(generic_resource, BASE_URL_GROUP) == 0)
			return createGroup(connection, con_info, (char *) resource, usr);;

	// DELETE: for single resource, it can be only deletion... at the moment!
	} else if(strcmp(method, DELETE) == 0) {

		// Check authorization for deleting the single resource
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _DELETE, generic_resource, resource)) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "User not authorized to execute %s on %s", method, resource);
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}

		if(strcmp(generic_resource, BASE_URL_GRAPH) == 0)
			return deleteGraph(connection, (char *) resource);
		else if(strcmp(generic_resource, BASE_URL_USER) == 0)
			return deleteUser(connection, (char *) resource);
		else if(strcmp(generic_resource, BASE_URL_GROUP) == 0)
			return deleteGroup(connection, (char *) resource);

	} else if(strcmp(method, POST) == 0) {

		if(strcmp(generic_resource, BASE_URL_USER) == 0) {
			// Check authorization
			if (dbmanager != NULL && !secmanager->isAuthorized(usr, _CREATE, generic_resource, resource)) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "User not authorized to execute %s on %s", method, resource);
				return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
			}
			return createUser((char *) resource, connection, con_info);
		}

		return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
	}

	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: %s on /%s/%s not implemented!", method, generic_resource, resource);
	return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
}

int RestServer::doOperationOnResource(struct MHD_Connection *connection, struct connection_info_struct *con_info, user_info_t *usr, const char *method, const char *generic_resource, const char *resource, const char *extra_info) {

	// GET: can be only read... at the moment!
	if(strcmp(method, GET) == 0) {

		// If security is required, check READ authorization
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, generic_resource, resource)) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User not authorized to execute %s on %s", method, resource);
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}
		if(strcmp(generic_resource,BASE_URL_GRAPH)==0 && strcmp(resource,URL_STATUS)==0)
			return doGetStatus(connection,extra_info);
	}
	// PUT, POST, DELETE: currently not supported
	else if(strcmp(method, POST) == 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: POST on /%s/%s/%s not implemented!", generic_resource, resource, extra_info);
		return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
	}

	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: %s on /%s/%s/%s not implemented!", method, generic_resource, resource, extra_info);
	return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
}


int RestServer::httpResponse(struct MHD_Connection *connection, int code) {
	struct MHD_Response *response;

	response = MHD_create_response_from_buffer(0, (void*) "", MHD_RESPMEM_PERSISTENT);
	int ret = MHD_queue_response(connection, code, response);
	MHD_destroy_response(response);
	return ret;
}

int RestServer::doOperation(struct MHD_Connection *connection, void **con_cls, const char *method, const char *url) {
	int ret = 0;

	user_info_t *usr = NULL;

	struct connection_info_struct *con_info = (struct connection_info_struct *) (*con_cls);
	assert(con_info != NULL);

	// The stuff below is a sequence of routing checks for HTTP requests (both header and body)

	// Check Host field in HTTP header
	if (MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host") == NULL) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
	}

	// HTTP body must be empty in GET and DELETE requests
	if(strcmp(method, GET) == 0 || strcmp(method, DELETE) == 0) {
		if (con_info->length != 0) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s with body is not allowed", method);
			return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
		}

	// PUT and POST requests must contain JSON data in their body
	} else if(strcmp(method, PUT) == 0 || strcmp(method, POST) == 0) {
		const char *c_type = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Content-Type");
		if ((c_type == NULL) || (strcmp(c_type, JSON_C_TYPE) != 0)) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content-Type must be: "JSON_C_TYPE);
			return httpResponse(connection, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE);
		}
	}

	// ...end of routine HTTP requests checks

	// If security is required, check whether the current message is a login request
	if(dbmanager != NULL && isLoginRequest(method, url)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Received a login request!");
		// execute login routine
		return login(connection, con_cls);
	}

	// If security is required, try to authenticate the client
	char *token = NULL;
	if (dbmanager != NULL) {
		token = (char *) MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Auth-Token");

		if(token == NULL) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Token\" header not present in the request");
			return false;
		}

		if(!secmanager->isAuthenticated(connection, token)) {
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Token: %s",token);
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "User not authenticated: request rejected!");
			return httpResponse(connection, MHD_HTTP_UNAUTHORIZED);
		}
	}

	// check for invalid URL
	assert(url && url[0]=='/'); // neither NULL nor empty

	// Fetch from URL the generic resource name, resource name and extra info
	std::string generic_resource, resource, extra;
	std::stringstream urlstream(url+1); // +1 to skip first "/"
	if (getline(urlstream, generic_resource, '/'))
		if (getline(urlstream, resource, '/'))
			if (getline(urlstream, extra, '/')) {}

	// Fetch user information
	if(dbmanager != NULL)
		usr = dbmanager->getUserByToken(token);

	// If operation on a generic resource (e.g. /NF-FG)
	if(!generic_resource.empty() && resource.empty() && extra.empty())
		ret = doOperationOnResource(connection, con_info, usr, method,
				generic_resource.c_str());

	// If operation on a single resource (e.g. /NF-FG/myGraph) 
	else if(!generic_resource.empty() && !resource.empty() && extra.empty())
		ret = doOperationOnResource(connection, con_info, usr, method,
				generic_resource.c_str(), resource.c_str());

	// If operation on a specific feature of a single resource (e.g. /NF-FG/myGraph/flowID)
	else if(!generic_resource.empty() && !resource.empty() && !extra.empty())
		ret = doOperationOnResource(connection, con_info, usr, method,
				generic_resource.c_str(), resource.c_str(), extra.c_str());

	// all other requests (e.g. a request to "/") --> 404
	else {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Returning 404 for %s request on %s", method, url);
		return httpResponse(connection, MHD_HTTP_NOT_FOUND);
	}

	/*
	 * The usr variable points to a memory space that is allocated inside the getUserByToken() method,
	 * by using malloc(), so I have to free that memory.
	 * FIXME, this needs to be changed, as the struct
	 * members of usr are only valid because we leak that
	 * sqlite statment there...
	 */
	if(usr != NULL)
		free(usr);

	return ret;
}

int RestServer::readGraph(struct MHD_Connection *connection, char *graphID) 
{
	struct MHD_Response *response;
	int ret;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s", graphID);

	// Check whether the graph exists in the local database and in the graph manager
	if ( (dbmanager != NULL && !dbmanager->resourceExists(BASE_URL_GRAPH, graphID)) || (dbmanager == NULL && !gm->graphExists(graphID))) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method GET is not supported for this resource (i.e. it does not exist)");
		response = MHD_create_response_from_buffer(0, (void*) "",
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Allow", PUT);
		ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED,
				response);
		MHD_destroy_response(response);
		return ret;
	}

	try {
		Object json = gm->toJSON(graphID);
		stringstream ssj;
		write_formatted(json, ssj);
		string sssj = ssj.str();
		char *aux = (char*) malloc(sizeof(char) * (sssj.length() + 1));
		strcpy(aux, sssj.c_str());
		response = MHD_create_response_from_buffer(strlen(aux), (void*) aux,
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Content-Type", JSON_C_TYPE);
		MHD_add_response_header(response, "Cache-Control", NO_CACHE);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the graph description!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::readUser(struct MHD_Connection *connection, char *username) {
	assert(dbmanager != NULL);

	struct MHD_Response *response;
	int ret;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s", username);

	// Check whether the user exists
	if (!dbmanager->resourceExists(BASE_URL_USER, username)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method GET is not supported for this resource (i.e. it does not exist)");
		response = MHD_create_response_from_buffer(0, (void*) "",
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Allow", PUT);
		ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED,
				response);
		MHD_destroy_response(response);
		return ret;
	}

	try {
		json_spirit::Object json;

		user_info_t *usr = dbmanager->getUserByName(username);

		json["username"] = usr->user;
		json["group"] = usr->group;

		stringstream ssj;
		write_formatted(json, ssj);
		string sssj = ssj.str();
		char *aux = (char*) malloc(sizeof(char) * (sssj.length() + 1));
		strcpy(aux, sssj.c_str());
		response = MHD_create_response_from_buffer(strlen(aux), (void*) aux,
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Content-Type", JSON_C_TYPE);
		MHD_add_response_header(response, "Cache-Control", NO_CACHE);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the user description!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::readMultipleGroups(struct MHD_Connection *connection, user_info_t *usr) {
	assert(usr != NULL && dbmanager != NULL);

	struct MHD_Response *response;
	int ret;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s", BASE_URL_GROUP);

	// Check whether groups exists as a generic resourse in the local database
	if (!dbmanager->resourceExists(BASE_URL_GROUP)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The generic resource %s does not exist in the local database", BASE_URL_GROUP);
		response = MHD_create_response_from_buffer(0, (void*) "",
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Allow", PUT);
		ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED,
				response);
		MHD_destroy_response(response);
		return ret;
	}

	try {
		Object groups;
		Array groups_array;
		std::list<std::string> names;

		// search the names in the database
		dbmanager->getAllowedResourcesNames(usr, _READ, BASE_URL_GROUP, &names);

		std::list<std::string>::iterator i;

		for(i = names.begin(); i != names.end(); ++i) {
			Object obj;
			obj["name"] = *i;
			groups_array.push_back(obj);
		}

		groups[BASE_URL_GROUP] = groups_array;

		stringstream ssj;
		write_formatted(groups, ssj);
		string sssj = ssj.str();
		char *aux = (char*) malloc(sizeof(char) * (sssj.length() + 1));
		strcpy(aux, sssj.c_str());
		response = MHD_create_response_from_buffer(strlen(aux), (void*) aux,
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Content-Type", JSON_C_TYPE);
		MHD_add_response_header(response, "Cache-Control", NO_CACHE);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the graph description!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::readMultipleUsers(struct MHD_Connection *connection, user_info_t *usr) {
	assert(usr != NULL && dbmanager != NULL);

	struct MHD_Response *response;
	int ret;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s", BASE_URL_USER);

	// Check whether NF-FG exists as a generic resourse in the local database
	if (!dbmanager->resourceExists(BASE_URL_USER)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The generic resource %s does not exist in the local database", BASE_URL_GRAPH);
		response = MHD_create_response_from_buffer(0, (void*) "",
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Allow", PUT);
		ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED,
				response);
		MHD_destroy_response(response);
		return ret;
	}

	try {
		Object users;
		Array users_array;
		std::list<std::string> names;
		user_info_t *user = NULL;

		// If security is required, search the names in the database
		dbmanager->getAllowedResourcesNames(usr, _READ, BASE_URL_USER, &names);

		std::list<std::string>::iterator i;

		for(i = names.begin(); i != names.end(); ++i) {
			user = dbmanager->getUserByName((*i).c_str());
			Object u;
			u["username"] = user->user;
			u["group"] = user->group;
			users_array.push_back(u);
		}

		users[BASE_URL_USER] = users_array;

		stringstream ssj;
		write_formatted(users, ssj);
		string sssj = ssj.str();
		char *aux = (char*) malloc(sizeof(char) * (sssj.length() + 1));
		strcpy(aux, sssj.c_str());
		response = MHD_create_response_from_buffer(strlen(aux), (void*) aux,
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Content-Type", JSON_C_TYPE);
		MHD_add_response_header(response, "Cache-Control", NO_CACHE);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the graph description!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::readMultipleGraphs(struct MHD_Connection *connection, user_info_t *usr) {
	assert(usr != NULL);

	struct MHD_Response *response;
	int ret;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s", BASE_URL_GRAPH);

	// Check whether NF-FG exists as a generic resourse in the local database
	if (!dbmanager->resourceExists(BASE_URL_GRAPH)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The generic resource %s does not exist in the local database", BASE_URL_GRAPH);
		response = MHD_create_response_from_buffer(0, (void*) "",
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Allow", PUT);
		ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED,
				response);
		MHD_destroy_response(response);
		return ret;
	}

	try {
		Object nffg;
		Array nffg_array;
		std::list<std::string> names;

		// If security is required, search the names in the database
		if(dbmanager != NULL)
			dbmanager->getAllowedResourcesNames(usr, _READ, BASE_URL_GRAPH, &names);
		else
			// Otherwise, retrieve all the NF-FGs
			gm->getGraphsNames(&names);

		std::list<std::string>::iterator i;

		for(i = names.begin(); i != names.end(); ++i)
			nffg_array.push_back(gm->toJSON(*i));

		nffg[BASE_URL_GRAPH] = nffg_array;

		stringstream ssj;
		write_formatted(nffg, ssj);
		string sssj = ssj.str();
		char *aux = (char*) malloc(sizeof(char) * (sssj.length() + 1));
		strcpy(aux, sssj.c_str());
		response = MHD_create_response_from_buffer(strlen(aux), (void*) aux,
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Content-Type", JSON_C_TYPE);
		MHD_add_response_header(response, "Cache-Control", NO_CACHE);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the graph description!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::createGroup(struct MHD_Connection *connection, struct connection_info_struct *con_info, char *resource, user_info_t *usr) {

	assert(dbmanager != NULL);

	int ret = 0;
	struct MHD_Response *response;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Received request for deploying %s/%s", BASE_URL_GROUP, resource);

	// Check whether the group already exists in the database
	if(dbmanager->resourceExists(BASE_URL_GROUP, resource)) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: cannot create an already existing group in the database!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource to be created/updated: %s/%s", BASE_URL_GROUP, resource);
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s", con_info->message);

	// Update database
	dbmanager->insertResource(BASE_URL_GROUP, resource, usr->user);

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The group has been properly created!");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "");

	//TODO: put the proper content in the answer
	response = MHD_create_response_from_buffer(0, (void*) "", MHD_RESPMEM_PERSISTENT);
	stringstream absolute_url;
	absolute_url << REST_URL << ":" << REST_PORT << "/" << BASE_URL_GROUP << "/" << resource;
	MHD_add_response_header(response, "Location", absolute_url.str().c_str());
	ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response(response);
	return ret;
}

int RestServer::deployNewGraph(struct MHD_Connection *connection, struct connection_info_struct *con_info, char *resource, user_info_t *usr) {

	int ret = 0;
	struct MHD_Response *response;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Received request for deploying %s/%s", BASE_URL_GRAPH, resource);
	// If security is required, check whether the graph already exists in the database

	/* this check prevent updates!
	if(dbmanager != NULL && dbmanager->resourceExists(BASE_URL_GRAPH, resource)) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: cannot deploy an already existing graph in the database!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}

	// The same check within the graph manager
	if(gm->graphExists(resource)) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error: cannot deploy an already existing graph in the manager!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}*/

	string gID(resource);

	highlevel::Graph *graph = new highlevel::Graph(gID);

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource to be created/updated: %s/%s", BASE_URL_GRAPH, resource);
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s", con_info->message);

	bool newGraph = true;

	// Check whether the body is well formed
	if (!parsePutBody(*con_info, *graph, newGraph)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed content");
		return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
	}

	graph->print();

	try {
		if(gm->graphExists(resource)) {
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "An existing graph must be updated");
			if (!gm->updateGraph(gID,graph)) {
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph description is not valid!");
				return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
			}
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has been properly updated!");
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "");
		}else{
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "A new graph must be created");
			if (!gm->newGraph(graph)) {
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph description is not valid!");
				return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
			}
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has been properly created!");
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "");
		}
	}
	catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the creation of the graph!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	// If security is required, update database
	if(dbmanager != NULL)
		dbmanager->insertResource(BASE_URL_GRAPH, resource, usr->user);

	//TODO: put the proper content in the answer
	response = MHD_create_response_from_buffer(0, (void*) "", MHD_RESPMEM_PERSISTENT);
	stringstream absolute_url;
	absolute_url << REST_URL << ":" << REST_PORT << "/" << BASE_URL_GRAPH << "/" << resource;
	MHD_add_response_header(response, "Location", absolute_url.str().c_str());
	ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response(response);
	return ret;
}

int RestServer::deleteGraph(struct MHD_Connection *connection, char *resource) {

	int ret = 0;
	struct MHD_Response *response;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Received request for deleting %s/%s", BASE_URL_GRAPH, resource);

	// If security is required, check whether the graph does exist in the database
	if(dbmanager != NULL && !dbmanager->resourceExists(BASE_URL_GRAPH, resource)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Cannot delete a non-existing graph in the database!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}

	// Check whether the graph does exist in the graph manager
	if (!gm->graphExists(resource)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Cannot delete a non-existing graph in the manager!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}

	try {

		// Delete the graph
		if (!gm->deleteGraph(resource)) {
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "deleteGraph returns false!");
			return httpResponse(connection, MHD_HTTP_BAD_REQUEST);
		}

		// If security is required, update database
		if(dbmanager != NULL)
			dbmanager->deleteResource(BASE_URL_GRAPH, resource);

	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the destruction of the graph!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has been properly deleted!");

	return httpResponse(connection, MHD_HTTP_NO_CONTENT);
}

int RestServer::deleteGroup(struct MHD_Connection *connection, char *group) {
	assert(dbmanager != NULL);

	int ret = 0;
	struct MHD_Response *response;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Received request for deleting %s/%s", BASE_URL_GROUP, group);

	// Check whether the user does exist in the database
	if(!dbmanager->resourceExists(BASE_URL_GROUP, group)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Cannot delete a non-existing group in the database!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}

	if(!dbmanager->usersExistForGroup(group)) {
			dbmanager->deleteGroup(group);
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The group has been properly deleted!");
			return httpResponse(connection, MHD_HTTP_ACCEPTED);
	} else {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Cannot remove a group having one or more members!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}
}

int RestServer::deleteUser(struct MHD_Connection *connection, char *username) {

	int ret = 0;
	struct MHD_Response *response;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Received request for deleting %s/%s", BASE_URL_USER, username);

	// If security is required, check whether the user does exist in the database
	if(dbmanager != NULL && !dbmanager->resourceExists(BASE_URL_USER, username)) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Cannot delete a non-existing user in the database!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}

	try {
		// If security is required, update database
		if(dbmanager != NULL) {
			dbmanager->deleteResource(BASE_URL_USER, username);
			dbmanager->deleteUser(username);
		}
	} catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the deletion of the user!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The user has been properly deleted!");

	return httpResponse(connection, MHD_HTTP_NO_CONTENT);
}

int RestServer::doGetStatus(struct MHD_Connection *connection,const char *graphID)
{
	struct MHD_Response *response;
	int ret;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required get status for resource: %s",graphID);

	if(!gm->graphExists(graphID))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", graphID);
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
		MHD_destroy_response (response);
		return ret;
	}

	Object json;
	json["status"]="complete";
	json["percentage_completed"]=100;
	stringstream ssj;
	write_formatted(json, ssj );
	string sssj = ssj.str();
	char *aux = (char*)malloc(sizeof(char) * (sssj.length()+1));
	strcpy(aux,sssj.c_str());
	response = MHD_create_response_from_buffer (strlen(aux),(void*) aux, MHD_RESPMEM_PERSISTENT);
	MHD_add_response_header (response, "Content-Type",JSON_C_TYPE);
	MHD_add_response_header (response, "Cache-Control",NO_CACHE);
	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);
	return ret;

}
