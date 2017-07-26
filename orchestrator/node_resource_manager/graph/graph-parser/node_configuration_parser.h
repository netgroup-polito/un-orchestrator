#ifndef PROJECT_NODE_CONFIGURATION_PARSER_H
#define PROJECT_NODE_CONFIGURATION_PARSER_H

#pragma once

#include <string>
#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "../../../utils/constants.h"
#include "../../../utils/logger.h"
#include "node_configuration_parser_exception.h"

#include "../../graph/high_level_graph/high_level_graph.h"

class NodeConfigurationParser {

friend class ConfigurationAgent;

protected:

    /**
    *	@brief: parser of a node configuration
    */

    static void nodeConfigurationParser(Value value, highlevel::Graph &graph);
};


#endif //PROJECT_NODE_CONFIGURATION_PARSER_H
