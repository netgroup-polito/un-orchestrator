#ifndef ACTION_H_
#define ACTION_H_ 1

#pragma once

#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/openflow_common.h>
#include <rofl/common/caddress.h>

#include <ostream>

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"
#include "../../graph_manager/lsi.h"

#include "../generic_actions/generic_action.h"
#include "../output_actions/output_action.h"

using namespace std;

namespace lowlevel
{
enum action_t {LOCAL,NORMAL,TO_PORT};

class Action
{

private:
	action_t type;
	/**
	*	The action contains a list of generic actions!
	**/
	list<GenericAction*> genericActions;

	/**
	*	list of ports id where the traffic must exit!
	**/
	list<unsigned int> ports_id;

public:

	Action(action_t type = TO_PORT);

	void addOutputPort(unsigned int port_id);

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
	*	Associate a generic action with this action
	*/
	void addGenericAction(GenericAction *ga);

};

}

#endif //ACTION_H_

