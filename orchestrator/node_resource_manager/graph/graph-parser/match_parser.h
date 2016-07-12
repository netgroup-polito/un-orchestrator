#ifndef MATCH_PARSER_H_
#define MATCH_PARSER_H_ 1

#pragma once

//#include "rest_server.h"

#include <string>
#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include "../../graph_manager/graph_manager.h"

using namespace json_spirit;
using namespace std;

class MatchParser
{

friend class GraphParser;

protected:

	static string nfId(string id_port);
	static unsigned int nfPort(string name_port);
	static bool nfIsPort(string name_port);

	static string epName(string name_port);
	static unsigned int epPort(string name_port);

	static bool parseMatch(Object object, highlevel::Match &match, highlevel::Action &action, map<string,string > &iface_id, map<string,string > &internal_id, map<string,pair<string,string> > &vlan_id, map<string,string> &gre_id, highlevel::Graph &graph);

private:
	static bool validateMac(const char* mac);
	static bool validateIpv4(const string &ipAddress);
	static bool validateIpv6(const string &ipAddress);
	static bool validateIpv4Netmask(const string &netmask);

public:
	static string graphID(string name_port);
};

#endif //MATCH_PARSER_H_
