#ifndef HIGH_LEVEL_ACTION_OUTPUT_H_
#define HIGH_LEVEL_ACTION_OUTPUT_H_ 1

#include "high_level_output_action.h"
#include "../../../utils/logger.h"

#include <iostream>

using namespace std;

namespace highlevel
{

class ActionPort : public Action
{
private:
	
	/**
	*	@brief: the name of the port (e.g., eth0)
	*/
	string portName;

	/**
	*	@brief: the name of the endpoint (e.g., endpoint:00000001)
	*/
	string endpointName;

public:
	~ActionPort();
	ActionPort(string portName, string endpointName);
	string getInfo();
	string toString();

	bool operator==(const ActionPort &other) const;

	Object toJSON();
};

}

#endif //HIGH_LEVEL_ACTION_OUTPUT_H_
