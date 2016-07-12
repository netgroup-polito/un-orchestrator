#ifndef HIGH_LEVEL_GRAPH_ENPOINT_VLAN_H_
#define HIGH_LEVEL_GRAPH_ENPOINT_VLAN_H_ 1

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

class EndPointVlan
{
private:
	/**
	*	@brief: the id of the endpoint port (e.g., 00000003)
	*/
	string id;

	/**
	*	@brief: the name of the endpoint port (e.g., ingress)
	*/
	string name;

	/**
	*	@brief: the vlan id (e.g., 270)
	*/
	string vlan_id;

	/**
	*	@brief: the interface of the port (e.g., eth0)
	*/
	string interface;

public:

	EndPointVlan(string id, string name, string vlan_id, string interface);
	string getId();
	string getName();
	string getVlanId();
	string getInterface();

	~EndPointVlan();

	bool operator==(const EndPointVlan &other) const;

	Object toJSON();
};

}

#endif //HIGH_LEVEL_GRAPH_ENPOINT_VLAN_H_
