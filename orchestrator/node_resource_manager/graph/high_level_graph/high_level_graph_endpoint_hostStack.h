#ifndef HIGH_LEVEL_GRAPH_ENPOINT_HOSTSTACK_H_
#define HIGH_LEVEL_GRAPH_ENPOINT_HOSTSTACK_H_ 1

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"
#include "high_level_element_position.h"

#include <iostream>
#include <sstream>

#include <string.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;

using namespace std;

enum hoststack_conf_t { NONE, DHCP, STATIC, PPPOE};

namespace highlevel
{

class EndPointHostStack
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
	*	@brief: configuration type of the endpoint (e.g., 00000003)
	*/
	hoststack_conf_t configuration;
	
	/**
	*	@brief: the ip address of the port (e.g., 192.168.1.1/24)
	*/
	string ipAddress;

	/**
	*	@brief: the mac address of the port (e.g., aa:bb:cc:dd:ee:ff)
	*/
	string macAddress;

	/**
	*	@brief: coordinates of the endpoint (used and setted by the GUI)
	*/
	Position *position = NULL;

public:

	EndPointHostStack(string id, string name, hoststack_conf_t configuration, string ipAddress, string macAddress);

	/**
	*	@brief: return the ID of the endpoint
	*/
	string getId();
	
	/**
	*	@brief: return the name of the endpoint
	*/
	string getName();
	
	/**
	*	@brief: return the ip address (with netmask) of the endpoint
	*/
	string getIpAddress();

	/**
	*	@brief: return the mac address of the endpoint
	*/
	string getMacAddress();

	/**
	*	@brief: return the configuration type
	*/
	hoststack_conf_t getConfiguration();

	void setPosition(Position *position);

	~EndPointHostStack();

	bool operator==(const EndPointHostStack &other) const;

	Object toJSON();

};

}

#endif //HIGH_LEVEL_GRAPH_ENPOINT_HOSTSTACK_H_
