#ifndef GENERIC_ACTION_H_
#define GENERIC_ACTION_H_ 1

#pragma once

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

class GenericAction
{

public:
	virtual ~GenericAction();

	virtual void toJSON(Object &json) = 0;

	/**
	*	@brief: insert the generic action into a flowmod message
	*
	*	@param: message		flowmod message
	*/
	virtual void fillFlowmodMessage(rofl::openflow::cofflowmod &message, unsigned int *position) = 0;

	virtual void print() {}
	virtual string prettyPrint() = 0;

protected:
	GenericAction();
};

#endif //GENERIC_ACTION_H_
