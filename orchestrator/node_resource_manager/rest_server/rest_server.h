#ifndef REST_SERVER_H_
#define REST_SERVER_H_ 1

/**
*	Documentation on HTTP return values can be found at:
*		http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
*
*	Documentation on HTTP headers can be found at:
*		http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
*/


#pragma once

#define __STDC_FORMAT_MACROS


//XXX the follwing includes are for the new server
#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/http_defs.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/net.h>
#include <pistache/peer.h>

#include "x-auth-token-header.h"

using namespace Pistache;

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sstream>

#include <openssl/sha.h>
#include <openssl/rand.h>

#include "../graph_manager/graph_manager.h"
#include "../../utils/constants.h"
#include "../../utils/logger.h"
#include "../../utils/configuration.h"

#include "../graph/generic_actions/vlan_action.h"
#include "../graph/output_actions/action_port.h"

#include "../graph/graph-parser/graph_parser.h"

#include "../graph/high_level_graph/nf_port_configuration.h"

#include "../database_manager/SQLite/SQLiteManager.h"

#include "../security_manager/security_manager.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include <sstream>
#include <fstream>

class GraphManager;

class RestServer
{

private:

	static GraphManager *gm;


#if 0
	static int readMultipleUsers(struct MHD_Connection *connection, user_info_t *usr);

	static int createGroup(struct MHD_Connection *connection, struct connection_info_struct *con_info, char *resource, user_info_t *usr);

	
	static int deleteUser(struct MHD_Connection *connection, char *username);

	static int deleteGroup(struct MHD_Connection *connection, char *group);

	static int readMultipleGroups(struct MHD_Connection *connection, user_info_t *usr);
#endif

public:
	RestServer(Address addr)
        : httpEndpoint(std::make_shared<Http::Endpoint>(addr))
    { }

	bool init(SQLiteManager *dbm, int core_mask);
	void start();
	void shutdown();

private:
	std::shared_ptr<Http::Endpoint> httpEndpoint;
	Rest::Router router;
	
	void setupRoutes();
	
	// POST /login
	void login(const Rest::Request& request, Http::ResponseWriter response);
	// POST /users/:username
	void createUser(const Rest::Request& request, Http::ResponseWriter response);
	// GET /users/:username
	void getUser(const Rest::Request& request, Http::ResponseWriter response);
	
	// PUT /NF-FG/:graphID
	void putGraph(const Rest::Request& request, Http::ResponseWriter response);
	// GET /NF-FG/:graphID
	void getGraph(const Rest::Request& request, Http::ResponseWriter response);
	// DELETE /NF-FG/:graphID
	void deleteGraph(const Rest::Request& request, Http::ResponseWriter response);
	// GET /NF-FG/status/:graphID
	void getGraphStatus(const Rest::Request& request, Http::ResponseWriter response);
	// GET /NF-FG/
	void getGraphs(const Rest::Request& request, Http::ResponseWriter response);

	
	bool readGraphFromFile(const string &nffgResourceName, string &nffgFileName);
	int createGraphFromFile(const string &graphID, string toBeCreated);
	void parseGraphFromFile(string toBeCreated,highlevel::Graph &graph, bool newGraph);
	
	bool parsePostBody(string content, char *user, char *pwd);
	bool parseLoginForm(Value value, char *user, char *pwd);
	bool parsePostBody(string content, char **user, char **pwd, char **group);
	bool parseUserCreationForm(Value value, char **pwd, char **group);
	
	void parsePutBody(string content, highlevel::Graph &graph, bool newGraph);
};

#endif //REST_SERVER_H_
