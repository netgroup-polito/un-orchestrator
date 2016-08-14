#ifndef NODE_ORCHESTRATOR_PORT_ACTION_H
#define NODE_ORCHESTRATOR_PORT_ACTION_H

#include "../../../utils/logger.h"
#include "output_action.h"

#include <iostream>

using namespace std;

class ActionPort: public OutputAction {

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


#endif //NODE_ORCHESTRATOR_PORT_ACTION_H
