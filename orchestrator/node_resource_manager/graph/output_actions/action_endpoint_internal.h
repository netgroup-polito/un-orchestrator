#ifndef NODE_ORCHESTRATOR_ACTION_ENDPOINT_INTERNAL_H
#define NODE_ORCHESTRATOR_ACTION_ENDPOINT_INTERNAL_H

#include "../../../utils/logger.h"
#include "output_action.h"
#include <iostream>
#include <sstream>

class TempActionEndPointInternal: public OutputAction {

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

    TempActionEndPointInternal(string group, string endpointName);
    string getInfo();
    unsigned int getGroup();
    string getOutputEndpointID();
    string toString();

    bool operator==(const TempActionEndPointInternal &other) const;

    Object toJSON();
};


#endif //NODE_ORCHESTRATOR_ACTION_ENDPOINT_INTERNAL_H
