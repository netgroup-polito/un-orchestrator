#ifndef PROJECT_CONFIGURATION_AGENT_H
#define PROJECT_CONFIGURATION_AGENT_H 1

#include <map>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <signal.h>

#include "../../utils/constants.h"
#include "../../utils/logger.h"
#include "../graph/high_level_graph/high_level_graph.h"


using namespace std;

class ConfigurationAgent {

public:
    ConfigurationAgent();
    ~ConfigurationAgent();

    /**
    *	@brief: set the IP address of the default gateway
    */
    bool setDefaultGateway(string ipAddress);

    /**
    *	@brief: set the configuration if the node
    */
    bool setNodeConfiguration(highlevel::Graph *graph);

};


#endif //PROJECT_CONFIGURATION_AGENT_H
