#ifndef REST_SERVER_H_
#define REST_SERVER_H_ 1

/**
*	@brief: the REST server is based on the microhttpd library:
*				www.gnu.org/software/libmicrohttpd/
*
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
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/net.h>
#include <pistache/peer.h>

using namespace Pistache;


#include <microhttpd.h>

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

	struct connection_info_struct
	{
		char *message;
		size_t length;
	};

	static int print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

	static int doOperation(struct MHD_Connection *connection, void **con_cls, const char *method, const char *url);

	static int doGetStatus(struct MHD_Connection *connection,const char *graphID);
	
	static int readGraph(struct MHD_Connection *connection, char *graphID);
	static int readMultipleGraphs(struct MHD_Connection *connection, user_info_t *usr);
	static int readMultipleUsers(struct MHD_Connection *connection, user_info_t *usr);
	static int readConfiguration(struct MHD_Connection *connection);
	static int retrieveTemplateId(struct MHD_Connection *connection, string graphId, string vnfId);

	static int doPost(struct MHD_Connection *connection, const char *url, void **con_cls, bool client_auth);

	
	

	static int doPut(struct MHD_Connection *connection, const char *url, void **con_cls);
	static void parsePutBody(struct connection_info_struct &con_info,highlevel::Graph &graph, bool newGraph);

	static int doDelete(struct MHD_Connection *connection,const char *url, void **con_cls);

	
	

	

	static bool isLoginRequest(const char *method, const char *url);

	static int deployNewGraph(struct MHD_Connection *connection, struct connection_info_struct *con_info, char *resource, user_info_t *usr);

	static int createGroup(struct MHD_Connection *connection, struct connection_info_struct *con_info, char *resource, user_info_t *usr);

	static int deleteGraph(struct MHD_Connection *connection, char *resource);
	static int deleteUser(struct MHD_Connection *connection, char *username);

	static int deleteGroup(struct MHD_Connection *connection, char *group);

	static int readMultipleGroups(struct MHD_Connection *connection, user_info_t *usr);

	static int readUser(struct MHD_Connection *connection, char *username);


	/**
	 * @brief:	The doOperationOnResource methods are responsible for checking user permissions related to the operation to perform
	 * 			and call the proper handler. The first version is operations on generic resources (e.g. NF-FG, interfaces, users, ...), the
	 * 			second one is for single resources, which are mapped to a generic one (e.g. NF-FG/myGraph, users/bob, ...). The third
	 * 			one is for working with some extra details related to a single resource (e.g. NF-FG/myGraph/flow_id).
	 */
	static int doOperationOnResource(struct MHD_Connection *connection, struct connection_info_struct *con_info, user_info_t *usr, const char *method, const char *generic_resource);
	static int doOperationOnResource(struct MHD_Connection *connection, struct connection_info_struct *con_info, user_info_t *usr, const char *method, const char *generic_resource, const char *resource);
	static int doOperationOnResource(struct MHD_Connection *connection, struct connection_info_struct *con_info, user_info_t *usr, const char *method, const char *generic_resource, const char *resource, const char *extra_info);

	static int doPutOnSingleResource(struct MHD_Connection *connection, void **con_cls, char *generic_resource, char *resource, char *user);

	static int doPutGraph(struct MHD_Connection *connection, struct connection_info_struct *con_info, char *generic_resource, char *resource);

	static int httpResponse(struct MHD_Connection *connection, int code);

public:
	RestServer(Address addr)
        : httpEndpoint(std::make_shared<Http::Endpoint>(addr))
    { }


	

	static void terminate();

	static int answer_to_connection (void *cls, struct MHD_Connection *connection,
						const char *url, const char *method, const char *version,
						const char *upload_data, size_t *upload_data_size, void **con_cls);

	static void request_completed (void *cls, struct MHD_Connection *connection, void **con_cls,
						enum MHD_RequestTerminationCode toe);

//IVANO: XXX: the following variables/methods have been added during the porting

	bool init(SQLiteManager *dbm, int core_mask);
	void start();

private:
	std::shared_ptr<Http::Endpoint> httpEndpoint;
	Rest::Router router;
	
	void setupRoutes();
	
	// POST /login
	void login(const Rest::Request& request, Http::ResponseWriter response);
	// POST /users/:username
	void createUser(const Rest::Request& request, Http::ResponseWriter response);
	
	bool readGraphFromFile(const string &nffgResourceName, string &nffgFileName);
	int createGraphFromFile(const string &graphID, string toBeCreated);
	void parseGraphFromFile(string toBeCreated,highlevel::Graph &graph, bool newGraph);
	
	bool parsePostBody(string content, char *user, char *pwd);
	bool parseLoginForm(Value value, char *user, char *pwd);
	bool parsePostBody(string content, char **user, char **pwd, char **group);
	bool parseUserCreationForm(Value value, char **pwd, char **group);
};

#endif //REST_SERVER_H_
