#ifndef NODE_ORCHESTRATOR_ACTION_ENDPOINT_GRE_H
#define NODE_ORCHESTRATOR_ACTION_ENDPOINT_GRE_H

#include "../../../utils/logger.h"
#include "output_action.h"
#include <iostream>
#include <sstream>


class TempActionEndPointGre: public OutputAction {

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

    TempActionEndPointGre(string endpointID, string endpointName);
    string getInfo();
    string getOutputEndpointID();
    string toString();

    bool operator==(const TempActionEndPointGre &other) const;

    Object toJSON();
};


#endif //NODE_ORCHESTRATOR_ACTION_ENDPOINT_GRE_H
