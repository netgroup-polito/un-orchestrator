#include "rest_server.h"
#include "../database_manager/SQLite/SQLiteManager.h"

static const char LOG_MODULE_NAME[] = "Rest-Server";

GraphManager *RestServer::gm = NULL;
SQLiteManager *dbmanager = NULL;

SecurityManager *secmanager = NULL;

bool RestServer::init(SQLiteManager *dbm, int core_mask)
{
	try
	{
		gm = new GraphManager(core_mask);

	} catch (...) {
		return false;
	}

	if (Configuration::instance()->getUserAuthentication()) {
		dbmanager = dbm;
		dbmanager->cleanTables();
		secmanager = new SecurityManager(dbmanager);
	}

	sleep(2); //XXX This give time to the controller to be initialized

	map <string,string> bootGraph = Configuration::instance()->getBootGraphs();
	//Handle the file containing the graphs to be deployed
	for(map<string,string>::iterator iter = bootGraph.begin(); iter!=bootGraph.end(); iter++)
	{
		if (!readGraphFromFile(iter->first,iter->second)) {
			delete gm;
			return false;
		}
	}
	
	//Register new HTTP haders not defined in the pistache library
	Header::Registry::registerHeader<XAuthToken>();
	
	//Actually inizialize the server
	auto opts = Http::Endpoint::options()
            .threads(1) 	//we use just a single thread
            .flags(Tcp::Options::InstallSignalHandler);	
        httpEndpoint->init(opts);
        setupRoutes(); 

	ULOG_INFO("System initialized");

	return true;
}

void RestServer::start() 
{
	httpEndpoint->setHandler(router.handler());
	httpEndpoint->serve();
}

void RestServer::shutdown()
{
	httpEndpoint->shutdown();
	delete (gm);
}

void RestServer::setupRoutes() {
	using namespace Rest;
	
	Routes::Post(router, "/login", Routes::bind(&RestServer::login, this));
	Routes::Post(router, "/users/:name", Routes::bind(&RestServer::createUser, this));
	Routes::Get(router, "/users/:name", Routes::bind(&RestServer::getUser, this));
	
	Routes::Post(router, "/NF-FG/", Routes::bind(&RestServer::newGraph, this));
	Routes::Put(router, "/NF-FG/:graphID", Routes::bind(&RestServer::updateGraph, this));
	Routes::Get(router, "/NF-FG/:graphID", Routes::bind(&RestServer::getGraph, this));
	Routes::Delete(router, "/NF-FG/:graphID", Routes::bind(&RestServer::deleteGraph, this));
	Routes::Get(router, "/NF-FG/status/:graphID", Routes::bind(&RestServer::getGraphStatus, this));
	Routes::Get(router, "/NF-FG", Routes::bind(&RestServer::getGraphs, this));

}

/************************************************/

/*
*	HTTP methods
*/

// POST /login
void RestServer::login(const Rest::Request& request, Http::ResponseWriter response) 
{
	ULOG_INFO("Received a 'login' request");

	int rc = 0;
	//struct MHD_Response *response;
	char username[BUFFER_SIZE], password[BUFFER_SIZE];
	unsigned char hash_token[HASH_SIZE+1], temp[HASH_SIZE+1];
	char hash_pwd[BUFFER_SIZE], nonce[HASH_SIZE+1], timestamp[BUFFER_SIZE], tmp[BUFFER_SIZE], user_tmp[BUFFER_SIZE];

	if (dbmanager == NULL) {
		ULOG_INFO("Login can be performed only if authentication is enabled through the configuration file");
		//return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
		response.send(Http::Code::Not_Implemented);
		return;
	}

	string req = request.body();

	ULOG_INFO("Content:");
	ULOG_INFO("%s", req.c_str());
	
	//Extract the 'host' header
	auto headers = request.headers();
	auto host = headers.get<Http::Header::Host>();
	string h = host->host();
	ULOG_DBG_INFO("Header host: %s", h.c_str());
	//If the header is not in the request, the server returns authomatically
	
	if (!parsePostBody(req, username, password)) 
	{
		ULOG_INFO("Login error: Malformed content");
		response.send(Http::Code::Bad_Request);
		return;
	}

	try {
		SHA256((const unsigned char*) password, strlen(password), hash_token);
		hash_token[HASH_SIZE] = '\0';

		strcpy(tmp, "");
		strcpy(hash_pwd, "");
		strcpy(nonce, "");

		for (int i = 0; i < HASH_SIZE; i++) {
			sprintf(tmp, "%.2x", hash_token[i]);
			strcat(hash_pwd, tmp);
		}

		strcpy(user_tmp, username);

		if(!dbmanager->userExists(user_tmp, hash_pwd)) 
		{
			// It is checking whether the user exists or not. Note that it does not use the "plain" password, 
			// but the sha256 version of the password.
			ULOG_ERR("Login failed: wrong username or password!");
			response.send(Http::Code::Unauthorized);
			return;
		}

		if(dbmanager->isLogged(user_tmp)) {
			char *token = NULL;
			user_info_t *pUI = NULL;

			pUI = dbmanager->getLoggedUserByName(user_tmp);
			token = pUI->token;

			Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
			Pistache::Http::Header::CacheControl cc(cd);
			response.headers()
				.add<Pistache::Http::Header::CacheControl>(cd)
				.add<Pistache::Http::Header::ContentType>(MIME(Text,Plain));
						
			ULOG_INFO("User has been correctly authenticated!");
			
			response.send(Http::Code::Ok, token);
			return;
		}

		rc = RAND_bytes(temp, /*sizeof(temp)*/HASH_SIZE);
		if (rc != 1) {
			ULOG_ERR("An error occurred while generating nonce!");
			response.send(Http::Code::Internal_Server_Error);
			return;
		}
		tmp[HASH_SIZE] = '\0';

		strcpy(tmp, "");
		strcpy(hash_pwd, "");

		int i = 0;
		for (; i < HASH_SIZE; i++) {
			sprintf(tmp, "%.2x", temp[i]);
			strcat(nonce, tmp);
		}
		nonce[HASH_SIZE] = '\0';

		time_t now = time(0);
		tm *ltm = localtime(&now);

		strcpy(timestamp, "");
		sprintf(timestamp, "%d/%d/%d %d:%d", ltm->tm_mday, 1 + ltm->tm_mon, 1900 + ltm->tm_year, ltm->tm_hour, 1 + ltm->tm_min);

		// Insert login information into the database
		dbmanager->insertLogin(user_tmp, nonce, timestamp);

		Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
		Pistache::Http::Header::CacheControl cc(cd);
		response.headers()
			.add<Pistache::Http::Header::CacheControl>(cd)
			.add<Pistache::Http::Header::ContentType>(MIME(Text,Plain));
			
		ULOG_INFO("User has been correctly authenticated!");
		ULOG_DBG_INFO("Token generated: %s", nonce);

		response.send(Http::Code::Ok, nonce);
		return;

	} catch (...) {
		ULOG_ERR("An error occurred during user login!");
		response.send(Http::Code::Internal_Server_Error);
		return;
	}
}

// POST /users/:name
void RestServer::createUser(const Rest::Request& request, Http::ResponseWriter response) 
{
	if (dbmanager == NULL) {
		ULOG_INFO("Login can be performed only if authentication is enabled through the configuration file");
		//return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
		response.send(Http::Code::Not_Implemented);
		return;
	}

	//Retrieve the username from the URL
	auto username = request.param(":name").as<std::string>();
	
	ULOG_INFO("Received the request of creating a new user: '%s'");

	char *group = NULL, *password = NULL;

	unsigned char *hash_token = new unsigned char[HASH_SIZE]();
	char *hash_pwd = new char[BUFFER_SIZE]();
	char *tmp = new char[HASH_SIZE]();
	char *pwd = new char[HASH_SIZE]();

	string req = request.body();

	ULOG_INFO("Content:");
	ULOG_INFO("%s", req.c_str());

	//Extract the 'host' header
	auto headers = request.headers();
	auto host = headers.get<Http::Header::Host>();
	string h = host->host();
	ULOG_DBG_INFO("Header 'Host': %s", h.c_str());
	//If the header is not in the request, the server returns authomatically
	
	auto token_header = headers.get<XAuthToken>();
	string t = token_header->token();
	ULOG_DBG_INFO("Header 'X-Auth-Token: %s", t.c_str());

	char *token = (char*)t.c_str();

	if (!parsePostBody(req, NULL, &password, &group)) 
	{
		ULOG_INFO("Create user error: Malformed content");
		response.send(Http::Code::Bad_Request);
		return;
	}

	char *t_group = new char[strlen(group)+1]();
	strncpy(t_group, group, strlen(group));

	try {
		if (username.c_str() == NULL || group == NULL || password == NULL) {
			ULOG_ERR("Client unathorized!");
			//FIXME: does this make sense? Username cannot be NULL, otherwise the router does not bring the request
			//to this method. Group and pwd cannot be NULL, since we have already checked that the content of the 
			//request is correct
			response.send(Http::Code::Unauthorized);
			return;
		}

		strncpy(pwd, password, strlen(password));

		SHA256((const unsigned char*)pwd, strlen(pwd), hash_token);

		for (int i = 0; i < HASH_SIZE; i++) {
			sprintf(tmp, "%.2x", hash_token[i]);
			strcat(hash_pwd, tmp);
		}

		//FIXME: here, we should only check the username, not also the password
		if(dbmanager->userExists((char*)(username.c_str()), hash_pwd)) {
			ULOG_ERR("User creation failed: already existing!");
			response.send(Http::Code::Unauthorized);
			return;
		}

		if(!dbmanager->resourceExists(BASE_URL_GROUP, t_group)) {
			ULOG_ERR("User creation failed! The group '%s' cannot be recognized!", t_group);
			response.send(Http::Code::Unauthorized);
			//TODO: the returned code is probably wrong
			return;
		}
		
		//FIXME: I'm not sure that the HTTP code used are appropriate

		user_info_t *creator = dbmanager->getUserByToken(token);

		dbmanager->insertUser((char*)(username.c_str()), hash_pwd, t_group);

		dbmanager->insertResource(BASE_URL_USER, (char*)(username.c_str()), creator->user);

		/* TODO: This is just a provisional solution for handling
		 * user creation permissions for those users that are
		 * dynamically created via a POST request
		 */
		dbmanager->insertUserCreationPermission((char*)(username.c_str()), BASE_URL_GRAPH, ALLOW);
		dbmanager->insertUserCreationPermission((char*)(username.c_str()), BASE_URL_USER, ALLOW);
		dbmanager->insertUserCreationPermission((char*)(username.c_str()), BASE_URL_GROUP, ALLOW);

		delete t_group;

		response.send(Http::Code::Accepted);
		return;

	} catch (...) {
		ULOG_ERR("An error occurred during user login!");
		response.send(Http::Code::Internal_Server_Error);
		return;
	}
}

// GET /users/:name
void RestServer::getUser(const Rest::Request& request, Http::ResponseWriter response) 
{
	if (dbmanager == NULL) {
		ULOG_INFO("Login can be performed only if authentication is enabled through the configuration file");
		//return httpResponse(connection, MHD_HTTP_NOT_IMPLEMENTED);
		response.send(Http::Code::Not_Implemented);
		return;
	}

	//Retrieve the username from the URL
	auto username = request.param(":name").as<std::string>();
	
	ULOG_INFO("Received the request related to user: %s",(char*)username.c_str());

	assert(dbmanager != NULL);

	// Check whether the user exists
	if (!dbmanager->resourceExists(BASE_URL_USER, username.c_str())) {
		ULOG_INFO("Method GET is not supported for this resource (i.e. it does not exist)");
	
		response.headers()
			.add<Pistache::Http::Header::Allow>(Http::Method::Post);
		
		response.send(Http::Code::Method_Not_Allowed);
		return;
	}

	try {
		json_spirit::Object json;

		user_info_t *usr = dbmanager->getUserByName((char*)username.c_str());

		json["username"] = usr->user;
		json["group"] = usr->group;

		stringstream ssj;
		write_formatted(json, ssj);
		string sssj = ssj.str();
		
		Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
		Pistache::Http::Header::CacheControl cc(cd);
		response.headers()
			.add<Pistache::Http::Header::CacheControl>(cd)
			.add<Pistache::Http::Header::ContentType>(MIME(Text,Json));
			
		ULOG_INFO("User has been correctly authenticated!");

		response.send(Http::Code::Ok, sssj);
		return;
	} catch (...) {
		ULOG_ERR("An error occurred while retrieving the user description!");
		response.send(Http::Code::Internal_Server_Error);
		return;
	}
}

// POST /NF-FG/
void RestServer::newGraph(const Rest::Request& request, Http::ResponseWriter response) 
{
	ULOG_INFO("Received a POST request for a new graph");

	user_info_t *usr = NULL;
	
	string gID;

	//generate a random UUID for the new NF-FG
	do
	{
		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		gID = boost::lexical_cast<std::string>(uuid);
	}while(gm->graphExists(gID.c_str()));

	//Authenticate the user, if needed
	if(dbmanager != NULL)
	{
		auto headers = request.headers();
		auto token_header = headers.get<XAuthToken>();
		string t = token_header->token();
		ULOG_DBG_INFO("Header 'X-Auth-Token: %s", t.c_str());
		
		usr = dbmanager->getUserByToken(t.c_str());
		
		if(usr==NULL)
		{
			ULOG_INFO("The token is not valid");
			response.send(Http::Code::Unauthorized);
			return;
		}
		
		
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, /*generic_resource*/"NF-FG", gID.c_str()))
		{
			ULOG_INFO("User not authorized to execute the PUT on NF-FG '%s'",gID.c_str());
			response.send(Http::Code::Unauthorized);
			return;
		}
		
		
	}

	ULOG_DBG_INFO("The graph will be associated with the UUID: %s", gID.c_str());

	//TODO check other headers?

	
	// If security is required, check whether the graph already exists in the database

	/* this check prevent updates!
	if(dbmanager != NULL && dbmanager->resourceExists(BASE_URL_GRAPH, resource)) {
		ULOG_ERR("Error: cannot deploy an already existing graph in the database!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}

	// The same check within the graph manager
	if(gm->graphExists(resource)) {
		ULOG_ERR("Error: cannot deploy an already existing graph in the manager!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}*/



	highlevel::Graph *graph = new highlevel::Graph(gID);

	ULOG_INFO("Resource to be created: %s/%s", BASE_URL_GRAPH, gID.c_str());
	
	string req = request.body();

	ULOG_INFO("Content:");
	ULOG_INFO("%s", req.c_str());

	bool newGraph = true;

	// Check whether the body is well formed
	try
	{
		parsePutBody(req, *graph, newGraph);
	}catch(GraphParserException* e)
	{
		ULOG_INFO("Malformed content");
		Object json;
		json["module"]=e->getModule();
		json["message"]=e->what();
		stringstream ssj;
		write_formatted(json, ssj);
		string sssj = ssj.str();
	
		Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
		Pistache::Http::Header::CacheControl cc(cd);
		response.headers()
			.add<Pistache::Http::Header::CacheControl>(cd)
			.add<Pistache::Http::Header::ContentType>(MIME(Text,Json));
		
		response.send(Http::Code::Bad_Request,sssj);
		return;
	}

	graph->print();

	try {
		if (!gm->newGraph(graph)) {
			ULOG_INFO("The graph description is not valid!");
			response.send(Http::Code::Bad_Request);
			return;
		}
		ULOG_INFO("The graph has been properly created!");
		ULOG_INFO("");		
	}
	catch (...) {
		ULOG_ERR("An error occurred during the creation of the graph!");
		response.send(Http::Code::Internal_Server_Error);
		return;
	}

	// If security is required, update database
	if(dbmanager != NULL)
		dbmanager->insertResource(BASE_URL_GRAPH, gID.c_str(), usr->user);

	//Return the UUID
	
	json_spirit::Object json;
	json["nffg-uuid"] = gID;

	stringstream ssj;
	write_formatted(json, ssj);
	string sssj = ssj.str();
	
	stringstream absolute_url;
	absolute_url << REST_URL << ":" << REST_PORT << "/" << BASE_URL_GRAPH << "/" << gID;
	
	Pistache::Http::Header::Location l(absolute_url.str());
	response.headers()
		.add<Pistache::Http::Header::Location>(l);
	
	response.send(Http::Code::Created,sssj);
	return;
}

// PUT /NF-FG/:graphID
void RestServer::updateGraph(const Rest::Request& request, Http::ResponseWriter response) 
{
	ULOG_INFO("Received a PUT request for updating a graph");

	//Retrieve the graphID
	auto gID = request.param(":graphID").as<std::string>();
	
	user_info_t *usr = NULL;

	//Authenticate the user, if needed
	if(dbmanager != NULL)
	{
		auto headers = request.headers();
		auto token_header = headers.get<XAuthToken>();
		string t = token_header->token();
		ULOG_DBG_INFO("Header 'X-Auth-Token: %s", t.c_str());
		
		usr = dbmanager->getUserByToken(t.c_str());
		
		if(usr==NULL)
		{
			ULOG_INFO("The token is not valid");
			response.send(Http::Code::Unauthorized);
			return;
		}
		
		
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, /*generic_resource*/"NF-FG", gID.c_str()))
		{
			ULOG_INFO("User not authorized to execute the PUT on NF-FG '%s'",gID.c_str());
			response.send(Http::Code::Unauthorized);
			return;
		}
	}

	//TODO check other headers?
	
	// Check whether the graph exists in the local database and in the graph manager
	if ( (dbmanager != NULL && !dbmanager->resourceExists(BASE_URL_GRAPH, gID.c_str())) || (dbmanager == NULL && !gm->graphExists(gID.c_str()))) 
	{
		ULOG_INFO("The NF-FG to be updated does not exist!");
				
		response.headers()
			.add<Pistache::Http::Header::Allow>(Http::Method::Post);
		
		response.send(Http::Code::Method_Not_Allowed);//TODO change with Not found?
		return;
	}


	// If security is required, check whether the graph already exists in the database

	/* this check prevent updates!
	if(dbmanager != NULL && dbmanager->resourceExists(BASE_URL_GRAPH, resource)) {
		ULOG_ERR("Error: cannot deploy an already existing graph in the database!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}

	// The same check within the graph manager
	if(gm->graphExists(resource)) {
		ULOG_ERR("Error: cannot deploy an already existing graph in the manager!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}*/

	highlevel::Graph *graph = new highlevel::Graph(gID);

	ULOG_INFO("Resource to be updated: %s/%s", BASE_URL_GRAPH, gID.c_str());
	
	string req = request.body();

	ULOG_INFO("Content:");
	ULOG_INFO("%s", req.c_str());

	bool newGraph = true;

	// Check whether the body is well formed
	try
	{
		parsePutBody(req, *graph, newGraph);
	}catch(GraphParserException* e)
	{
		ULOG_INFO("Malformed content");
		Object json;
		json["module"]=e->getModule();
		json["message"]=e->what();
		stringstream ssj;
		write_formatted(json, ssj);
		string sssj = ssj.str();
	
		Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
		Pistache::Http::Header::CacheControl cc(cd);
		response.headers()
			.add<Pistache::Http::Header::CacheControl>(cd)
			.add<Pistache::Http::Header::ContentType>(MIME(Text,Json));
		
		response.send(Http::Code::Bad_Request,sssj);
		return;
	}

	graph->print();

	try {
		if (!gm->updateGraph(gID,graph)) {
			ULOG_INFO("The graph description is not valid!");
			response.send(Http::Code::Bad_Request);
			return;
		}
		ULOG_INFO("The graph has been properly updated!");
		ULOG_INFO("");
	}
	catch (...) {
		ULOG_ERR("An error occurred during the creation of the graph!");
		response.send(Http::Code::Internal_Server_Error);
		return;
	}

	// If security is required, update database
	if(dbmanager != NULL)
		dbmanager->insertResource(BASE_URL_GRAPH, gID.c_str(), usr->user);
	
	response.send(Http::Code::Accepted);
	return;
}

// GET /NF-FG/:graphID
void RestServer::getGraph(const Rest::Request& request, Http::ResponseWriter response)
{
	ULOG_INFO("Received a GET request for a graph");

	//Retrieve the graphID
	auto graphID = request.param(":graphID").as<std::string>();
	
	user_info_t *usr = NULL;

	//Authenticate the user, if needed
	if(dbmanager != NULL)
	{
		auto headers = request.headers();
		auto token_header = headers.get<XAuthToken>();
		string t = token_header->token();
		ULOG_DBG_INFO("Header 'X-Auth-Token: %s", t.c_str());
		
		usr = dbmanager->getUserByToken(t.c_str());
		
		if(usr==NULL)
		{
			ULOG_INFO("The token is not valid");
			response.send(Http::Code::Unauthorized);
			return;
		}
		
		
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, /*generic_resource*/"NF-FG", graphID.c_str()))
		{
			ULOG_INFO("User not authorized to execute the GET on NF-FG '%s'",graphID.c_str());
			response.send(Http::Code::Unauthorized);
			return;
		}		
	}

	ULOG_INFO("Required NF-FG: %s", graphID.c_str());

	// Check whether the graph exists in the local database and in the graph manager
	if ( (dbmanager != NULL && !dbmanager->resourceExists(BASE_URL_GRAPH, graphID.c_str())) || (dbmanager == NULL && !gm->graphExists(graphID.c_str()))) {
		ULOG_INFO("The required NF-FG does not exist!");
				
		response.headers()
			.add<Pistache::Http::Header::Allow>(Http::Method::Post);
		
		response.send(Http::Code::Method_Not_Allowed);//TODO change with Not found?
		return;
	}

	try {
		Object json = gm->toJSON(graphID);
		stringstream ssj;
		write_formatted(json, ssj);
		string sssj = ssj.str();
		
		Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
		Pistache::Http::Header::CacheControl cc(cd);
		response.headers()
			.add<Pistache::Http::Header::CacheControl>(cd)
			.add<Pistache::Http::Header::ContentType>(MIME(Text,Json));
		
		response.send(Http::Code::Ok,sssj);
		return;
		
	} catch (...) {
		ULOG_ERR("An error occurred while retrieving the graph description!");
		response.send(Http::Code::Internal_Server_Error);
		return;
	}
}

// DELETE /NF-FG/:graphID
void RestServer::deleteGraph(const Rest::Request& request, Http::ResponseWriter response)
{
	ULOG_INFO("Received a DELETE request for a graph");

	//Retrieve the graphID
	auto graphID = request.param(":graphID").as<std::string>();
	
	user_info_t *usr = NULL;

	//Authenticate the user, if needed
	if(dbmanager != NULL)
	{
		auto headers = request.headers();
		auto token_header = headers.get<XAuthToken>();
		string t = token_header->token();
		ULOG_DBG_INFO("Header 'X-Auth-Token: %s", t.c_str());
		
		usr = dbmanager->getUserByToken(t.c_str());
		
		if(usr==NULL)
		{
			ULOG_INFO("The token is not valid");
			response.send(Http::Code::Unauthorized);
			return;
		}
		
		
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, /*generic_resource*/"NF-FG", graphID.c_str()))
		{
			ULOG_INFO("User not authorized to execute the DELETE the NF-FG '%s'",graphID.c_str());
			response.send(Http::Code::Unauthorized);
			return;
		}		
	}

	ULOG_INFO("Received request for deleting %s/%s", BASE_URL_GRAPH, graphID.c_str());

	// If security is required, check whether the graph does exist in the database
	if(dbmanager != NULL && !dbmanager->resourceExists(BASE_URL_GRAPH, graphID.c_str())) {
		ULOG_INFO("Cannot delete a non-existing graph in the database!");
		
		response.headers()
			.add<Pistache::Http::Header::Allow>(Http::Method::Put);
		
		response.send(Http::Code::Method_Not_Allowed);//TODO change with Not found?
		return;
	}

	// Check whether the graph does exist in the graph manager
	if (!gm->graphExists(graphID.c_str())) {
		ULOG_INFO("Cannot delete a non-existing graph in the manager!");
	
		response.headers()
			.add<Pistache::Http::Header::Allow>(Http::Method::Put);
		
		response.send(Http::Code::Method_Not_Allowed);//TODO change with Not found?
		return;
	}
	//FIXME:not sure if it is possible, with authentication enabled, that the graph is only in the GM or in the database

	try {

		// Delete the graph
		if (!gm->deleteGraph(graphID.c_str())) {
			ULOG_ERR("deleteGraph returns false!");		
		
			response.send(Http::Code::Bad_Request);
			return;
		}

		// If security is required, update database
		if(dbmanager != NULL)
			dbmanager->deleteResource(BASE_URL_GRAPH, (char*) graphID.c_str());

	} catch (...) {
		ULOG_ERR("An error occurred during the destruction of the graph!");
		response.send(Http::Code::Internal_Server_Error);
		return;
	}

	ULOG_INFO("The graph has been properly deleted!");

	response.send(Http::Code::No_Content);
	return;
}

// GET /NF-FG/status/:graphID
void RestServer::getGraphStatus(const Rest::Request& request, Http::ResponseWriter response)
{

	ULOG_INFO("Received a GET status request for a graph");

	//Retrieve the graphID
	auto graphID = request.param(":graphID").as<std::string>();
	
	user_info_t *usr = NULL;

	//Authenticate the user, if needed
	if(dbmanager != NULL)
	{
		auto headers = request.headers();
		auto token_header = headers.get<XAuthToken>();
		string t = token_header->token();
		ULOG_DBG_INFO("Header 'X-Auth-Token: %s", t.c_str());
		
		usr = dbmanager->getUserByToken(t.c_str());
		
		if(usr==NULL)
		{
			ULOG_INFO("The token is not valid");
			response.send(Http::Code::Unauthorized);
			return;
		}
		
		
		if (dbmanager != NULL && !secmanager->isAuthorized(usr, _READ, /*generic_resource*/"NF-FG", graphID.c_str()))
		{
			ULOG_INFO("User not authorized to execute the GET on NF-FG '%s'",graphID.c_str());
			response.send(Http::Code::Unauthorized);
			return;
		}		
	}

	ULOG_INFO("Required NF-FG: %s", graphID.c_str());


	ULOG_INFO("Required get status for NF-FG: %s",graphID.c_str());

	if(!gm->graphExists(graphID.c_str()))
	{
		ULOG_INFO("Resource \"%s\" does not exist", graphID.c_str());
		response.send(Http::Code::Not_Found);
		return;
	}

	Object json;
	json["status"]="complete";
	json["percentage_completed"]=100;
	stringstream ssj;
	write_formatted(json, ssj );
	string sssj = ssj.str();
	
	Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
	Pistache::Http::Header::CacheControl cc(cd);
	response.headers()
		.add<Pistache::Http::Header::CacheControl>(cd)
		.add<Pistache::Http::Header::ContentType>(MIME(Text,Json));
		
	response.send(Http::Code::Ok,sssj);
	return;
}

// GET /NF-FG/
void RestServer::getGraphs(const Rest::Request& request, Http::ResponseWriter response)
{

	ULOG_INFO("Received a GET request for all graphs");
	user_info_t *usr = NULL;

	//Authenticate the user, if needed
	if(dbmanager != NULL)
	{
		auto headers = request.headers();
		auto token_header = headers.get<XAuthToken>();
		string t = token_header->token();
		ULOG_DBG_INFO("Header 'X-Auth-Token: %s", t.c_str());
		
		usr = dbmanager->getUserByToken(t.c_str());
		
		if(usr==NULL)
		{
			ULOG_INFO("The token is not valid");
			response.send(Http::Code::Unauthorized);
			return;
		}
	}


	if((dbmanager != NULL) && !dbmanager->resourceExists(BASE_URL_GRAPH)) 
	{
		ULOG_INFO("The generic resource %s does not exist in the local database", BASE_URL_GRAPH);
		response.headers()
			.add<Pistache::Http::Header::Allow>(Http::Method::Put);
		
		response.send(Http::Code::Method_Not_Allowed);//TODO change with Not found?
		return;
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
		{
			Object json = gm->toJSON(*i);
			json["nffg-uuid"] = (*i);
			
			nffg_array.push_back(json);
		}

		nffg[BASE_URL_GRAPH] = nffg_array;

		stringstream ssj;
		write_formatted(nffg, ssj);
		string sssj = ssj.str();
		
		Pistache::Http::CacheDirective cd(Pistache::Http::CacheDirective::NoCache);
		Pistache::Http::Header::CacheControl cc(cd);
		response.headers()
			.add<Pistache::Http::Header::CacheControl>(cd)
			.add<Pistache::Http::Header::ContentType>(MIME(Text,Json));
		
		response.send(Http::Code::Ok,sssj);
		return;
	} catch (...) {
		ULOG_ERR("An error occurred while retrieving the graph description!");
		response.send(Http::Code::Internal_Server_Error);
		return;;
	}
}

/*
*	Helper methods
*/

bool RestServer::readGraphFromFile(const string &nffgResourceName, string &nffgFileName) {
	ULOG_INFO("Considering the graph described in file '%s'", nffgFileName.c_str());

	std::ifstream file;
	file.open(nffgFileName.c_str());
	if (file.fail()) {
		ULOG_ERR("Cannot open the file %s", nffgFileName.c_str());
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

int RestServer::createGraphFromFile(const string &graphID, string toBeCreated) {

	ULOG_INFO("Graph ID: %s", graphID.c_str());
	ULOG_INFO("Graph content:");
	ULOG_INFO("%s",
			toBeCreated.c_str());

	highlevel::Graph *graph = new highlevel::Graph(graphID);

	try
	{
		parseGraphFromFile(toBeCreated, *graph, true);
	}catch(GraphParserException*)
	{
		ULOG_ERR("Malformed contentt");
		return 0;
	}
	

	graph->print();
	try {
		if (!gm->newGraph(graph)) {
			ULOG_INFO("The graph description is not valid!");
			return 0;
		}
		// If security is required, update database
		if(dbmanager != NULL)
			dbmanager->insertResource(BASE_URL_GRAPH, graphID.c_str(), ADMIN);
	} catch (...) {
		ULOG_ERR("An error occurred during the creation of the graph!");
		return 0;
	}

	return 1;
}

void RestServer::parseGraphFromFile(string toBeCreated, highlevel::Graph &graph,
		bool newGraph) //startup. cambiare nome alla funzione
		{
	Value value;
	read(toBeCreated, value);
	GraphParser::parseGraph(value, graph, newGraph, gm);
	return;
}


//TODO: merge together the two following methods
bool RestServer::parsePostBody(string content, char *user, char *pwd) 
{
	Value value;
	read(content, value);
	return parseLoginForm(value, user, pwd);
}

bool RestServer::parseLoginForm(Value value, char *user, char *pwd) 
{
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
				ULOG_DBG_INFO("Invalid key: %s", name.c_str());
				return false;
			}
		}
		if (!foundUser) {
			ULOG_DBG_INFO("Key \"%s\" not found", USER);
			return false;
		} else if (!foundPwd) {
			ULOG_DBG_INFO("Key \"%s\" not found", PASS);
			return false;
		}
	} catch (std::exception& e) {
		ULOG_DBG_INFO("The content does not respect the JSON syntax: ", e.what());
		return false;
	}

	return true;
}


//TODO: merge together the two following methods
//TODO: the user parameter is completely useles!
bool RestServer::parsePostBody(string content, char **user, char **pwd, char **group) 
{
	Value value;
	read(content, value);
	return parseUserCreationForm(value, pwd, group);
}

bool RestServer::parseUserCreationForm(Value value, char **pwd, char **group) 
{
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
				ULOG_DBG_INFO("\tPwd: %s", *pwd);

			} else if (name == GROUP) {
				foundGroup = true;
				(*group) = (char *) value.getString().c_str();
				ULOG_DBG_INFO("\tGrp: %s", *group);
			} else {
				ULOG_DBG_INFO("Invalid key: %s", name.c_str());
				return false;
			}
		}

		if (!foundPwd) {
			ULOG_DBG_INFO("Key \"%s\" not found", PASS);
			return false;
		} else if (!foundGroup) {
			ULOG_DBG_INFO("Key \"%s\" not found", GROUP);
		}
	} catch (std::exception& e) {
		ULOG_DBG_INFO("The content does not respect the JSON syntax: ", e.what());
		return false;
	}

	return true;
}

//TODO: remove this method and put the code in the caller
void RestServer::parsePutBody(string content, highlevel::Graph &graph, bool newGraph) 
{
	Value value;
	read(content, value);
	GraphParser::parseGraph(value, graph, newGraph, gm);
	return;
}

/******************************************/


#if 0

int RestServer::readMultipleGroups(struct MHD_Connection *connection, user_info_t *usr) {
	assert(usr != NULL && dbmanager != NULL);

	struct MHD_Response *response;
	int ret;

	ULOG_INFO("Required resource: %s", BASE_URL_GROUP);

	// Check whether groups exists as a generic resourse in the local database
	if (!dbmanager->resourceExists(BASE_URL_GROUP)) {
		ULOG_INFO("The generic resource %s does not exist in the local database", BASE_URL_GROUP);
		response = MHD_create_response_from_buffer(0, (void*) "",
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Allow", _PUT);
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
		ULOG_ERR("An error occurred while retrieving the graph description!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::readMultipleUsers(struct MHD_Connection *connection, user_info_t *usr) {
	assert(usr != NULL && dbmanager != NULL);

	struct MHD_Response *response;
	int ret;

	ULOG_INFO("Required resource: %s", BASE_URL_USER);

	// Check whether NF-FG exists as a generic resourse in the local database
	if (!dbmanager->resourceExists(BASE_URL_USER)) {
		ULOG_INFO("The generic resource %s does not exist in the local database", BASE_URL_GRAPH);
		response = MHD_create_response_from_buffer(0, (void*) "",
				MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "Allow", _PUT);
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
		ULOG_ERR("An error occurred while retrieving the graph description!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
}

int RestServer::createGroup(struct MHD_Connection *connection, struct connection_info_struct *con_info, char *resource, user_info_t *usr) {

	assert(dbmanager != NULL);

	int ret = 0;
	struct MHD_Response *response;

	ULOG_INFO("Received request for deploying %s/%s", BASE_URL_GROUP, resource);

	// Check whether the group already exists in the database
	if(dbmanager->resourceExists(BASE_URL_GROUP, resource)) {
		ULOG_ERR("Error: cannot create an already existing group in the database!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}

	ULOG_INFO("Resource to be created/updated: %s/%s", BASE_URL_GROUP, resource);
	ULOG_INFO("Content:");
	ULOG_INFO("%s", con_info->message);

	// Update database
	dbmanager->insertResource(BASE_URL_GROUP, resource, usr->user);

	ULOG_INFO("The group has been properly created!");
	ULOG_INFO("");

	//TODO: put the proper content in the answer
	response = MHD_create_response_from_buffer(0, (void*) "", MHD_RESPMEM_PERSISTENT);
	stringstream absolute_url;
	absolute_url << REST_URL << ":" << REST_PORT << "/" << BASE_URL_GROUP << "/" << resource;
	MHD_add_response_header(response, "Location", absolute_url.str().c_str());
	ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response(response);
	return ret;
}

int RestServer::deleteGroup(struct MHD_Connection *connection, char *group) {
	assert(dbmanager != NULL);

	int ret = 0;
	struct MHD_Response *response;

	ULOG_INFO("Received request for deleting %s/%s", BASE_URL_GROUP, group);

	// Check whether the user does exist in the database
	if(!dbmanager->resourceExists(BASE_URL_GROUP, group)) {
		ULOG_INFO("Cannot delete a non-existing group in the database!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", _PUT);
		ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response (response);
		return ret;
	}

	if(!dbmanager->usersExistForGroup(group)) {
			dbmanager->deleteGroup(group);
			ULOG_INFO("The group has been properly deleted!");
			return httpResponse(connection, MHD_HTTP_ACCEPTED);
	} else {
		ULOG_INFO("Cannot remove a group having one or more members!");
		return httpResponse(connection, MHD_HTTP_FORBIDDEN);
	}
}

int RestServer::deleteUser(struct MHD_Connection *connection, char *username) {

	int ret = 0;
	struct MHD_Response *response;

	ULOG_INFO("Received request for deleting %s/%s", BASE_URL_USER, username);

	// If security is required, check whether the user does exist in the database
	if(dbmanager != NULL && !dbmanager->resourceExists(BASE_URL_USER, username)) {
		ULOG_INFO("Cannot delete a non-existing user in the database!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header (response, "Allow", _PUT);
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
		ULOG_ERR("An error occurred during the deletion of the user!");
		return httpResponse(connection, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	ULOG_INFO("The user has been properly deleted!");

	return httpResponse(connection, MHD_HTTP_NO_CONTENT);
}

#endif
