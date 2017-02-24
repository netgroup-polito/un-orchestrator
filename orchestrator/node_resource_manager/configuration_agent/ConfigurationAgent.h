#ifndef PROJECT_CONFIGURATION_AGENT_H
#define PROJECT_CONFIGURATION_AGENT_H 1

#include <map>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <fstream>

#include "../graph/high_level_graph/high_level_graph.h"
#include "../graph/graph-parser/node_configuration_parser.h"

using namespace std;

class ConfigurationAgent {

public:
    ConfigurationAgent();
    ~ConfigurationAgent();

    /**
    *	@brief: callback called after a subscription to the topic of configuration of the DoubleDecker
    */
    void on_configurationEvent(char *graph);

    /**
    *	@brief: set the IP address of the default gateway
    */
    bool setDefaultGateway(string ipAddress);

    /**
    *	@brief: set the configuration if the node
    */
    bool setNodeConfiguration(highlevel::Graph *graph);

};

class ConfigurationAgentException: public std::exception
{
public:
    virtual const char* what() const throw()
    {
        return "ConfigurationAgentException";
    }
};


#endif //PROJECT_CONFIGURATION_AGENT_H
