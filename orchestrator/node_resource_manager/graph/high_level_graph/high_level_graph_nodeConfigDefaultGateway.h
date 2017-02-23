#ifndef PROJECT_HIGH_LEVEL_GRAPH_NODECONFIGDEFAULTGATEWAY_H
#define PROJECT_HIGH_LEVEL_GRAPH_NODECONFIGDEFAULTGATEWAY_H 1

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include <iostream>
#include <sstream>

#include <string.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;

using namespace std;

namespace highlevel
{

class NodeConfigDefaultGateway
{
private:
    /**
    *	@brief: the id of the configuration (e.g., 00000003)
    */
    string id;

    /**
	*	@brief: the IP of the default gateway
	*/
    string defaultGateway;

public:
    NodeConfigDefaultGateway();
    NodeConfigDefaultGateway(string id, string defaultGateway);

    /**
	*	@brief: return the ID of the configuration
	*/
    string getId();

    /**
	*	@brief: return the IP address of the configured default gateway
	*/
    string getDefaultGateway();

    ~NodeConfigDefaultGateway();

    /**
    *	Check if two configurations are the same.
    **/
    bool operator==(const NodeConfigDefaultGateway &other) const;

    /**
    *	Create the json representing the node configuration, according to the NF-FG formalism
    **/
    Object toJSON();
};

}


#endif //PROJECT_HIGH_LEVEL_GRAPH_NODECONFIGDEFAULTGATEWAY_H
