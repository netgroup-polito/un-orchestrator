#ifndef HIGH_LEVEL_GRAPH_ENPOINT_MANAGEMENT_H_
#define HIGH_LEVEL_GRAPH_ENPOINT_MANAGEMENT_H_ 1

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

class EndPointHostStack
{
private:
	typedef enum { DHCP, STATIC, PPPOE} configurationType;

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
	configurationType configuration;
	
	/**
	*	@brief: the ip address of the port (e.g., 192.168.1.1/24)
	*/
	string ipAddress;

public:

	EndPointHostStack(string id, string name, string configuration, string ipAddress);

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

	~EndPointHostStack();

	bool operator==(const EndPointHostStack &other) const;

	Object toJSON();

};

}

#endif //HIGH_LEVEL_GRAPH_ENPOINT_MANAGEMENT_H_
