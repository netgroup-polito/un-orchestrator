#ifndef HIGH_LEVEL_ACTION_ENDPOIN_INTERNAL_H_
#define HIGH_LEVEL_ACTION_ENDPOINT_INTERNAL_H_ 1

#pragma once

#include "high_level_output_action.h"
#include "../../../utils/logger.h"

#include <iostream>
#include <sstream>
#include <string.h>

using namespace std;

namespace highlevel
{

class ActionEndPointInternal : public Action
{
private:
	
	/**
	*	@brief: identifier of the endpoint group
	*/
	string group;
	
	/**
	*	@brief: the name of the endpoint (e.g., endpoint:00000001)
	*/
	string endpointName;

public:

	ActionEndPointInternal(string group, string endpointName);
	string getInfo();
	unsigned int getGroup();
	string getOutputEndpointID();
	string toString();
	
	bool operator==(const ActionEndPointInternal &other) const;

	Object toJSON();
};

}

#endif //HIGH_LEVEL_ACTION_ENDPOINT_INTERNAL_H_
