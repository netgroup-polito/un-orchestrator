#ifndef NODE_ORCHESTRATOR_OUTPUT_ACTION_H
#define NODE_ORCHESTRATOR_OUTPUT_ACTION_H

#include <rofl/common/crofbase.h>
#include <rofl/common/logging.h>
#include <rofl/common/openflow/openflow_common.h>
#include <rofl/common/caddress.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "../../../utils/constants.h"

using namespace json_spirit;
using namespace std;

enum output_action_t {ACTION_ON_PORT,ACTION_ON_NETWORK_FUNCTION,ACTION_ON_ENDPOINT_GRE,ACTION_ON_ENDPOINT_INTERNAL};

class OutputAction {

public:
    output_action_t getType();

    virtual ~OutputAction();

    virtual Object toJSON() = 0;

    virtual string toString() = 0;

    virtual string getInfo() = 0;


protected:
    OutputAction(output_action_t type);
    output_action_t type;

};


#endif //NODE_ORCHESTRATOR_OUTPUT_ACTION_H
