#ifndef HIGH_LEVEL_ACTION_NF_H_
#define HIGH_LEVEL_ACTION_NF_H_ 1

#include "high_level_output_action.h"
#include "../../../utils/logger.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace highlevel
{

class ActionNetworkFunction : public Action
{
private:
	/**
	*	@brief: the id of the NF (e.g., 0001)
	*/
	string nfId;

	/**
	*	@brief: the name of the endpoint port (e.g., vnf:0001:inout:0)
	*/
	string endpointPortName;

	/**
	*	@brief: the port of the NF (e.g., 1)
	*/
	unsigned int port;

public:

	ActionNetworkFunction(string nfId, string endpointPortName, unsigned int port = 1);
	string getInfo();
	unsigned int getPort();
	string toString();

	bool operator==(const ActionNetworkFunction &other) const;

	Object toJSON();
};

}

#endif //HIGH_LEVEL_ACTION_NF_H_
