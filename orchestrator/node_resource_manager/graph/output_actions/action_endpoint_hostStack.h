#ifndef NODE_ORCHESTRATOR_ACTION_ENDPOINT_HOSTSTACK_H_
#define NODE_ORCHESTRATOR_ACTION_ENDPOINT_HOSTSTACK_H_ 1

#include "output_action.h"
#include "../../../utils/logger.h"

#include <iostream>
#include <sstream>

using namespace std;

class ActionEndPointHostStack : public OutputAction
{
private:

	/**
	*	@brief: endpoint identifier
	*/
	string endpointID;

	/**
	*	@brief: the name of the endpoint (e.g., endpoint:00000001)
	*/
	string endpointName;

public:

	ActionEndPointHostStack(string endpointID, string endpointName);
	string getInfo();
	string getOutputEndpointID();
	string toString();

	bool operator==(const ActionEndPointHostStack &other) const;

	Object toJSON();
};

#endif //NODE_ORCHESTRATOR_ACTION_ENDPOINT_HOSTSTACK_H_