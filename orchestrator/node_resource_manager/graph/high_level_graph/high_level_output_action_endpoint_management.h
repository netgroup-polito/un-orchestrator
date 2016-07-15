#ifndef HIGH_LEVEL_ACTION_ENDPOINT_MANAGEMENT_H_
#define HIGH_LEVEL_ACTION_ENDPOINT_MANAGEMENT_H_ 1

#include "high_level_output_action.h"
#include "../../../utils/logger.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace highlevel
{

	class ActionEndPointManagement : public Action
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

		ActionEndPointManagement(string endpointID, string endpointName);
		string getInfo();
		string getOutputEndpointID();
		string toString();

		bool operator==(const ActionEndPointManagement &other) const;

		Object toJSON();
	};

}

#endif //HIGH_LEVEL_ACTION_ENDPOINT_MANAGEMENT_H_