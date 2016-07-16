#ifndef HIGH_LEVEL_ACTION_ENDPOINT_HOSTSTACK_H_
#define HIGH_LEVEL_ACTION_ENDPOINT_HOSTSTACK_H_ 1

#include "high_level_output_action.h"
#include "../../../utils/logger.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace highlevel
{

	class ActionEndPointHostStack : public Action
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

}

#endif //HIGH_LEVEL_ACTION_ENDPOINT_HOSTSTACK_H_