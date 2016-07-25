#ifndef ACTION_H_
#define ACTION_H_ 1

#pragma once

#include <rofl/common/crofbase.h>
#include <rofl/common/logging.h>
#include <rofl/common/openflow/openflow_common.h>
#include <rofl/common/caddress.h>

#include <ostream>

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"
#include "../../graph_manager/lsi.h"

#include "../generic_actions/generic_action.h"

using namespace rofl;
using namespace std;

namespace lowlevel
{

class Action
{

private:
	openflow::ofp_action_type type;
	unsigned int port_id;
	bool is_local_port;
	
	/**
	*	@brief: it is true if the output port is defined by MAC learning
	*/
	bool is_normal;

	/**
	*	The outuput action contains a list of generic actions!
	*	The code is organized in this way, because the output action is
	*	mandatory in each rule.
	**/
	list<GenericAction*> genericActions;

public:
	Action(unsigned int port_id);
	Action(bool is_local_port);
	Action(bool is_local_port, bool is_normal);
	openflow::ofp_action_type getActionType();

	bool operator==(const Action &other) const;

	/**
	*	@brief: insert the action into a flowmod message
	*
	*	@param: message		flowmod message
	*/
	void fillFlowmodMessage(rofl::openflow::cofflowmod &message);

	void print();
	string prettyPrint(LSI *lsi0,map<string,LSI *> lsis);

	/**
	*	Associate a generic action with this output action
	*/
	void addGenericAction(GenericAction *ga);
};

}

#endif //ACTION_H_

