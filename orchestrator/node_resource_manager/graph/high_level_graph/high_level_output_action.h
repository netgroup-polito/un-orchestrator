#ifndef HIGH_LEVEL_ACTION_H_
#define HIGH_LEVEL_ACTION_H_ 1

#pragma once

#include "../../../utils/constants.h"
#include "../generic_actions/generic_action.h"
#include "../generic_actions/vlan_action.h"

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

enum action_t {INVALID,ACTION_ON_PORT,ACTION_ON_NETWORK_FUNCTION,ACTION_ON_ENDPOINT_GRE,ACTION_ON_ENDPOINT_INTERNAL};

class Action
{

private:
	action_t type;

public:
	action_t getType();
	virtual string getInfo() = 0;

	virtual Object toJSON() = 0;
	virtual string toString() = 0;

	/**
	*	Associate a generic action with this output action
	*/
	void addGenericAction(GenericAction *ga);

	/**
	*	Return the generic actions associated with this output action
	*/
	list<GenericAction*> getGenericActions();

	virtual ~Action();

protected:
	Action(action_t type);

	/**
	*	The outuput action contains a list of generic actions!
	*	The code is organized in this way, because the output action is
	*	mandatory in each rule.
	**/
	list<GenericAction*> genericActions;
};

}

#endif //HIGH_LEVEL_ACTION_H_
