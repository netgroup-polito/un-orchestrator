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

class EndPointManagement
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
	*	@brief: true (static ip address), false (dynamic ip address)
	*/
	bool is_static;
	
	/**
	*	@brief: the ip address of the port (e.g., 192.168.1.1)
	*/
	string ipAddress;

	/**
	*	@brief: the netmask of the ip address of the port (e.g., 255.255.255.0)
	*/
	string netmask;

public:
	EndPointManagement(string id, string name, bool isStatic, string ipAddress, string netmask);
	
	/**
	*	@brief: return the ID of the endpoint
	*/
	string getId();
	
	/**
	*	@brief: return the name of the endpoint
	*/
	string getName();
	
	/**
	*	@brief: return if the ip address of the endpoint is static or not
	*/
	bool isStatic();
	
	/**
	*	@brief: return the ip address of the endpoint
	*/
	string getIpAddress();

	/**
*	@brief: return the netmask of the ip address of the endpoint
*/
	string getIpNetmask();

	~EndPointManagement();

	bool operator==(const EndPointManagement &other) const;

	Object toJSON();

};

}

#endif //HIGH_LEVEL_GRAPH_ENPOINT_MANAGEMENT_H_
