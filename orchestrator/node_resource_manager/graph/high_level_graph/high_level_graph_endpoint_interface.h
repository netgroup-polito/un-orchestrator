#ifndef HIGH_LEVEL_GRAPH_ENPOINT_INTERFACE_H_
#define HIGH_LEVEL_GRAPH_ENPOINT_INTERFACE_H_ 1

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

class EndPointInterface
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
	*	@brief: the interface of the port (e.g., eth0)
	*/
	string interface;

public:
	EndPointInterface(string id, string name, string interface);
	
	/**
	*	@brief: return the ID of the endpoint
	*/
	string getId();
	
	/**
	*	@brief: return the name of the endpoint
	*/
	string getName();
	
	/**
	*	@brief: return the physical interface corresponding to the endpoint
	*/
	string getInterface();

	~EndPointInterface();

	bool operator==(const EndPointInterface &other) const;

	Object toJSON();
};

}

#endif //HIGH_LEVEL_GRAPH_ENPOINT_INTERFACE_H_
