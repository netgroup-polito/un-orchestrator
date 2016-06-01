#include "rest_server.h"

GraphManager *RestServer::gm = NULL;

/*
*
* SQLiteManager pointer
*
*/
SQLiteManager *dbmanager = NULL;
bool client_auth = false;

bool RestServer::init(SQLiteManager *dbm, bool cli_auth, char *nffg_filename,int core_mask, char *ports_file_name, string un_address, bool orchestrator_in_band, char *un_interface, char *ipsec_certificate)
{
	char *nffg_file_name = new char[BUFFER_SIZE];
	if(nffg_filename != NULL && strcmp(nffg_filename, "") != 0)
		strcpy(nffg_file_name, nffg_filename);
	else
		nffg_file_name = NULL;

	try
	{
		gm = new GraphManager(core_mask,string(ports_file_name),un_address,orchestrator_in_band,string(un_interface),string(ipsec_certificate));

	}catch (...)
	{
		return false;
	}

	//Handle the file containing the first graph to be deployed
	if(nffg_file_name != NULL)
	{
		sleep(2); //XXX This give time to the controller to be initialized

		if(!readGraphFromFile(nffg_file_name))
		{
			delete gm;
			return false;
		}
	}

	client_auth = cli_auth;

	if(client_auth)
		dbmanager = dbm;

	return true;
}

bool RestServer::readGraphFromFile(char *nffg_filename)
{
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Considering the graph described in file '%s'",nffg_filename);

	std::ifstream file;
	file.open(nffg_filename);
	if(file.fail())
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot open the file %s",nffg_filename);
		return false;
	}

	stringstream stream;
	string str;
	while (std::getline(file, str))
	    stream << str << endl;

	if(createGraphFromFile(stream.str()) == 0)
		return false;

	return true;
}

void RestServer::terminate()
{
	delete(gm);
}

void RestServer::request_completed (void *cls, struct MHD_Connection *connection,
						void **con_cls, enum MHD_RequestTerminationCode toe)
{
	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);

	if (NULL == con_info)
		return;

	if(con_info->length != 0)
	{
		free(con_info->message);
		con_info->message = NULL;
	}

	free (con_info);
	*con_cls = NULL;
}

int RestServer::answer_to_connection (void *cls, struct MHD_Connection *connection,
			const char *url, const char *method, const char *version,
			const char *upload_data,
			size_t *upload_data_size, void **con_cls)
{

	if(NULL == *con_cls)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "New %s request for %s using version %s", method, url, version);
		if(LOGGING_LEVEL <= ORCH_DEBUG)
			MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);

		struct connection_info_struct *con_info;
		con_info = (struct connection_info_struct*)malloc (sizeof (struct connection_info_struct));

		assert(con_info != NULL);
		if (NULL == con_info)
			return MHD_NO;

		if ((0 == strcmp (method, PUT)) || (0 == strcmp (method, POST)) || (0 == strcmp (method, DELETE)) )
		{
			con_info->message = (char*)malloc(REQ_SIZE * sizeof(char));
			con_info->length = 0;
		}
		else if (0 == strcmp (method, GET))
			con_info->length = 0;
		else
		{
			con_info->message = (char*)malloc(REQ_SIZE * sizeof(char));
			con_info->length = 0;
		}

		*con_cls = (void*) con_info;
		return MHD_YES;
	}

	if (0 == strcmp (method, GET))
		return doGet(connection,url);
	else if( (0 == strcmp (method, PUT)) || (0 == strcmp (method, POST)) || (0 == strcmp (method, DELETE)))
	{
		struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
		assert(con_info != NULL);
		if (*upload_data_size != 0)
		{
			strcpy(&con_info->message[con_info->length],upload_data);
			con_info->length += *upload_data_size;
			*upload_data_size = 0;
			return MHD_YES;
		}
		else if (NULL != con_info->message)
		{
			con_info->message[con_info->length] = '\0';
			if(0 == strcmp (method, PUT))
				return doPut(connection,url,con_cls);
			else if(0 == strcmp (method, POST))
				return doPost(connection,url,con_cls,client_auth);
			else
				return doDelete(connection,url,con_cls);
		}
	}
	else
	{
		//Methods not implemented
		struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
		assert(con_info != NULL);
		if (*upload_data_size != 0)
		{
			strcpy(&con_info->message[con_info->length],upload_data);
			con_info->length += *upload_data_size;
			*upload_data_size = 0;
			return MHD_YES;
		}
		else
		{
			con_info->message[con_info->length] = '\0';
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "++++Method \"%s\" not implemented",method);
			struct MHD_Response *response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
			int ret = MHD_queue_response (connection, MHD_HTTP_NOT_IMPLEMENTED, response);
			MHD_destroy_response (response);
			return ret;
		}
	}

	//Just to remove a warning in the compiler
	return MHD_YES;
}


int RestServer::print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "%s: %s", key, value);
	return MHD_YES;
}

int RestServer::doPost(struct MHD_Connection *connection, const char *url, void **con_cls, bool client_auth)
{
	struct MHD_Response *response;

	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
	assert(con_info != NULL);

	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	int ret = 0, rc = 0;
	unsigned char hash_token[HASH_SIZE], temp[BUFFER_SIZE];

	char hash_pwd[BUFFER_SIZE], nonce[BUFFER_SIZE], timestamp[BUFFER_SIZE];

	char *user, *pass;

	char tmp[BUFFER_SIZE], user_tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;

	while( pnt!= NULL )
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL_LOGIN) != 0)
				{
put_malformed_url:
					logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					int ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				else
				{
					if(!client_auth)
					{
						con_info->message[con_info->length] = '\0';
						logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" only exists if client authentication is required", url);
						struct MHD_Response *response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
						int ret = MHD_queue_response (connection, MHD_HTTP_NOT_IMPLEMENTED, response);
						MHD_destroy_response (response);
						return ret;
					}
				}
				break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}
	if(i != 1)
	{
		//the URL is malformed
		goto put_malformed_url;
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User login");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",con_info->message);

	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}

	if(!parsePostBody(*con_info,&user,&pass))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed content");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}

	try
	{
		if(user != NULL && pass != NULL){

			SHA256((const unsigned char*)pass, sizeof(pass) - 1, hash_token);

		    	strcpy(tmp, "");
		    	strcpy(hash_pwd, "");

		    	for (int i = 0; i < HASH_SIZE; i++) {
        			sprintf(tmp, "%x", hash_token[i]);
        			strcat(hash_pwd, tmp);
    			}

			strcpy(user_tmp, user);

			dbmanager->selectUsrPwd(user, (char *)hash_pwd);

			if(strcmp(dbmanager->getUser(), user_tmp) == 0 && strcmp(dbmanager->getPwd(), (char *)hash_pwd) == 0){
				if(strcmp(dbmanager->getToken(), "") == 0){

					rc = RAND_bytes(temp, sizeof(temp));
					if(rc != 1)
					{
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while generating nonce!");
						response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
						ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
						MHD_destroy_response (response);
						return ret;
					}

					strcpy(tmp, "");
					strcpy(hash_pwd, "");

					for (int i = 0; i < HASH_SIZE; i++) {
        					sprintf(tmp, "%x", temp[i]);
        					strcat(nonce, tmp);
    					}

					/*
					*
					* Calculating a timestamp
					*
					*/
					time_t now = time(0);

					tm *ltm = localtime(&now);

					strcpy(timestamp, "");
					sprintf(timestamp, "%d/%d/%d %d:%d", ltm->tm_mday, 1 + ltm->tm_mon, 1900 + ltm->tm_year, ltm->tm_hour, 1 + ltm->tm_min);

					dbmanager->updateTokenAndTimestamp(user_tmp, (char *)nonce, (char *)timestamp);
				}

				response = MHD_create_response_from_buffer (strlen((char *)nonce),(void*) nonce, MHD_RESPMEM_PERSISTENT);
				MHD_add_response_header (response, "Content-Type",TOKEN_TYPE);
				MHD_add_response_header (response, "Cache-Control",NO_CACHE);
				ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
				MHD_destroy_response (response);

				return ret;
			}

			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Client unauthorized");
			response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
			ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);
			MHD_destroy_response (response);

			return ret;
		}else{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Client unauthorized");
			response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
			ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);
			MHD_destroy_response (response);
			return ret;
		}
	}catch (...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during user login!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}

	//TODO: put the proper content in the answer
	response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
	stringstream absolute_url;
	absolute_url << REST_URL << ":" << REST_PORT << url;
	MHD_add_response_header (response, "Location", absolute_url.str().c_str());
	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);

	MHD_destroy_response (response);
	return ret;
}

bool RestServer::parsePostBody(struct connection_info_struct &con_info,char **user, char **pwd)
{
	Value value;
	read(con_info.message, value);
	return parseLoginForm(value, user, pwd);
}

bool RestServer::parseLoginForm(Value value, char **user, char **pwd)
{
	try
	{
		Object obj = value.getObject();

	  	bool foundUser = false, foundPwd = false;

		//Identify the flow rules
		for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
		{
	 	    const string& name  = i->first;
		    const Value&  value = i->second;

		    if(name == USER)
		    {
		  		foundUser = true;
		  		(*user) = (char *)value.getString().c_str();
		    }
		    else if(name == PASS)
		    {
				foundPwd = true;
		    		(*pwd) = (char *)value.getString().c_str();
		    }
		    else
		    {
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key: %s",name.c_str());
				return false;
		    }
		}
		if(!foundUser)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" not found",USER);
			return false;
		}
		else if(!foundPwd)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" not found",PASS);
			return false;
		}
	}catch(exception& e)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The content does not respect the JSON syntax: ",e.what());
		return false;
	}

	return true;
}

int RestServer::doPut(struct MHD_Connection *connection, const char *url, void **con_cls)
{
	struct MHD_Response *response;

	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
	assert(con_info != NULL);

	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	char graphID[BUFFER_SIZE];

	char tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL_GRAPH) != 0)
				{
put_malformed_url:
					logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					int ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				break;
			case 1:
				strcpy(graphID,pnt);
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}
	if(i != 2)
	{
		//the URL is malformed
		goto put_malformed_url;
	}

	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}

	const char *c_type = MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Content-Type");
	if(strcmp(c_type,JSON_C_TYPE) != 0)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content-Type must be: "JSON_C_TYPE);
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE, response);
		MHD_destroy_response (response);
		return ret;
	}

	bool newGraph = !(gm->graphExists(graphID));

	string gID(graphID);
	highlevel::Graph *graph = new highlevel::Graph(gID);

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource to be created/updated: %s",graphID);
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",con_info->message);

	if(!parsePutBody(*con_info,*graph,newGraph))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed content");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}

	graph->print();
	try
	{
		//if client authentication is required
		if(dbmanager != NULL){
			const char *token = MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "X-Auth-Token");

			if(!checkAuthentication(connection, token, dbmanager))
			{
				//User unauthenticated!
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);
				MHD_destroy_response (response);
				return ret;
			}
		}

		if(newGraph)
		{
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "A new graph must be created");
			if(!gm->newGraph(graph))
			{
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph description is not valid!");
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;
			}
		}
		else
		{
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "An existing graph must be updated");
			if(!gm->updateGraph(graphID,graph))
			{
				delete(graph);
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph description is not valid!");
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;
			}
		}
	}catch (...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the %s of the graph!",(newGraph)? "creation" : "update");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has been properly %s!",(newGraph)? "created" : "updated");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "");

	//TODO: put the proper content in the answer
	response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
	stringstream absolute_url;
	absolute_url << REST_URL << ":" << REST_PORT << url;
	MHD_add_response_header (response, "Location", absolute_url.str().c_str());
	int ret = MHD_queue_response (connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response (response);
	return ret;
}

int RestServer::createGraphFromFile(string toBeCreated)
{
	char graphID[BUFFER_SIZE];
	strcpy(graphID,GRAPH_ID);

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Graph ID: %s",graphID);
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Graph content:");
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",toBeCreated.c_str());

	string gID(graphID);
	highlevel::Graph *graph = new highlevel::Graph(gID);

	if(!parseGraphFromFile(toBeCreated,*graph,true))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed content");
		return 0;
	}

	graph->print();
	try
	{
		if(!gm->newGraph(graph))
		{
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph description is not valid!");
			return 0;
		}
	}catch (...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the creation of the graph!");
		return 0;
	}

	return 1;
}

bool RestServer::parseGraphFromFile(string toBeCreated,highlevel::Graph &graph, bool newGraph) //startup. cambiare nome alla funzione
{
	Value value;
	read(toBeCreated, value);
	return GraphParser::parseGraph(value, graph, newGraph, gm);
}

bool RestServer::parsePutBody(struct connection_info_struct &con_info,highlevel::Graph &graph, bool newGraph)
{
	Value value;
	read(con_info.message, value);
	return GraphParser::parseGraph(value, graph, newGraph, gm);
}


int RestServer::doGet(struct MHD_Connection *connection, const char *url)
{
	struct MHD_Response *response;
	int ret;

	bool request = 0; //false->graph - true->interfaces

	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	char graphID[BUFFER_SIZE];
	char tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL_GRAPH) == 0)
					request = false;
				else if(strcmp(pnt,BASE_URL_IFACES) == 0)
					request = true;
				else
				{
get_malformed_url:
					logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				break;
			case 1:
				strcpy(graphID,pnt);
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}
	if( (!request && i != 2) || (request == 1 && i != 1) )
	{
		//the URL is malformed
		goto get_malformed_url;
	}

	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}

	//if client authentication is required
	if(dbmanager != NULL)
	{
		const char *token = MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "X-Auth-Token");

		if(!checkAuthentication(connection, token, dbmanager))
		{
			//User unauthenticated!
			response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
			int ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);
			MHD_destroy_response (response);
			return ret;
		}
	}

	if(!request)
		//request for a graph description
		return doGetGraph(connection,graphID);
	else
		//request for interfaces description
		return doGetInterfaces(connection);
}

int RestServer::doGetGraph(struct MHD_Connection *connection,char *graphID)
{
	struct MHD_Response *response;
	int ret;

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s",graphID);

	if(!gm->graphExists(graphID))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method GET is not supported for this resource");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}

	try
	{
		Object json = gm->toJSON(graphID);
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
	}catch(...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the graph description!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}
}

int RestServer::doGetInterfaces(struct MHD_Connection *connection)
{
	struct MHD_Response *response;
	int ret;

	try
	{
		Object json = gm->toJSONPhysicalInterfaces();
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
	}catch(...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the description of the physical interfaces!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}
}

int RestServer::doDelete(struct MHD_Connection *connection, const char *url, void **con_cls)
{
	struct MHD_Response *response;
	int ret;

	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	char graphID[BUFFER_SIZE];
	char flowID[BUFFER_SIZE];
	bool specificFlow = false;

	char tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL_GRAPH) != 0)
				{
delete_malformed_url:
					logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				break;
			case 1:
				strcpy(graphID,pnt);
				break;
			case 2:
				strcpy(flowID,pnt);
				specificFlow = true;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}
	if((i != 2) && (i != 3))
	{
		//the URL is malformed
		goto delete_malformed_url;
	}

	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}

	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
	assert(con_info != NULL);
	if(con_info->length != 0)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "DELETE with body is not allowed");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}

	//if client authentication is required
	if(dbmanager != NULL){
		const char *token = MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "X-Auth-Token");

		if(!checkAuthentication(connection, token, dbmanager))
		{
			//User unauthenticated!
			response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
			int ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);
			MHD_destroy_response (response);
			return ret;
		}
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Deleting resource: %s/%s",graphID,(specificFlow)?flowID:"");

	if(!gm->graphExists(graphID) || (specificFlow && !gm->flowExists(graphID,flowID)))
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Method DELETE is not supported for this resource");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}

	try
	{
		if(!specificFlow)
		{
			//The entire graph must be deleted
			if(!gm->deleteGraph(graphID))
			{
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;
			}
			else
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The graph has been properly deleted!");
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "");
		}
		else
		{
			//A specific flow must be deleted
			if(!gm->deleteFlow(graphID,flowID))
			{
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
				MHD_destroy_response (response);
				return ret;
			}
			else
				logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "The flow has been properly deleted!");
		}

		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_NO_CONTENT, response);
		MHD_destroy_response (response);
		return ret;
	}catch(...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the destruction of the graph!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}
}

bool RestServer::checkAuthentication(struct MHD_Connection *connection,const char *token,SQLiteManager *dbmanager)
{

	if(token == NULL)
	{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Token\" header not present in the request");
		return false;
	}
	else
	{
		dbmanager->selectToken((char *)token);
		if(strcmp(dbmanager->getToken(), token) == 0){
			//User authenticated!
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "User authenticated");
			return true;
		}
		else
		{
			//User unauthenticated!
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "User unauthenticated");
			return false;
		}
	}
}

