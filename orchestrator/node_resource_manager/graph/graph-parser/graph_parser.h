#ifndef GRAPH_PARSER_H_
#define GRAPH_PARSER_H_ 1

#pragma once

#include <string>
#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include "../../graph_manager/graph_manager.h"

#include "match_parser.h"

using namespace json_spirit;
using namespace std;

class GraphParser
{

friend class RestServer;

protected:
	static bool parseGraph(Value value, highlevel::Graph &graph, bool newGraph, GraphManager *gm);
};

#endif //GRAPH_PARSER_H_
