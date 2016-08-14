#ifndef HIGH_LEVEL_ACTION_H_
#define HIGH_LEVEL_ACTION_H_ 1

#pragma once

#include "../../../utils/constants.h"
#include "../generic_actions/generic_action.h"
#include "../generic_actions/vlan_action.h"
#include "../output_actions/output_action.h"
#include "../output_actions/action_port.h"
#include "../output_actions/action_network_function.h"
#include "../output_actions/action_endpoint_internal.h"
#include "../output_actions/action_endpoint_gre.h"

#include <string>
#include <iostream>
#include <list>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;
using namespace std;

namespace highlevel
{

class Action
{

public:

	Array toJSON();

	/**
	*	Associate a generic action with this action
	*/
	void addGenericAction(GenericAction *ga);

	/**
	*	Return the generic actions associated with this action
	*/
	list<GenericAction*> getGenericActions();

	/**
	*	Associate a output action with this action
	*/
	void addOutputAction(OutputAction *oa);

	/**
	*	Return the output actions associated with this action
	*/
	list<OutputAction*> getOutputActions();

	~Action();

	Action();
protected:
	/**
	*	The action contains a list of generic actions!
	**/
	list<GenericAction*> genericActions;

	/**
	*	The action contains a list of output actions!
	*	Have almost one outputAction is mandatory!
	**/
	list<OutputAction*> outputActions;
};

}

#endif //HIGH_LEVEL_ACTION_H_
