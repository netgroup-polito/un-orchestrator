#ifndef MATCH_PARSER_H_
#define MATCH_PARSER_H_ 1

#pragma once

#include <string>
#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "../../../utils/logger.h"


#include "graph_parser_utils.h"
#include "../generic_actions/eth_action.h"

#include "../../graph_manager/graph_manager.h"

using namespace json_spirit;
using namespace std;

class MatchParser
{

friend class GraphParser;

protected:

	static void parseMatch(Object match_element, highlevel::Match &match, highlevel::Action &action, map<string,string > &iface_id, map<string,string > &internal_id, map<string,pair<string,string> > &vlan_id, map<string,gre_info_t> &gre_id, map<string,hoststack_info_t> &hoststack_id, map<string, map<string, bool> > trusted_ports, map<string, map<string,string> >trusted_ports_mac_addresses);

};

#endif //MATCH_PARSER_H_
