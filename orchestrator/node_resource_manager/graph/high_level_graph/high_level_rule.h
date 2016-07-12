#ifndef HIGH_LEVEL_RULE_H_
#define HIGH_LEVEL_RULE_H_ 1

#pragma once

#include "high_level_output_action.h"
#include "high_level_match.h"
#include "../../../utils/logger.h"

#include <iostream>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace std;
using namespace json_spirit;

namespace highlevel
{

class Rule
{

private:
	/**
	*	@brief: the match of this rule
	*/
	Match match;

	/**
	*	@brief: the action of this rule
	*/
	Action *action;

	/**
	*	@brief: the priority of this rule
	*/
	uint64_t priority;

	/**
	*	@brief: identifier of the rule. 
	*/
	string ruleID;

public:
	Rule(Match match, Action *action,string ruleID, uint64_t priority);

	Object toJSON();

	/**
	*	@brief: return the identifier of the rule
	*/
	string getRuleID();

	/**
	*	@brief: return the priority of the rule
	*/
	uint64_t getPriority();

	/**
	*	@brief: return the match that is part of this rule
	*/
	Match getMatch();

	/**
	*	@brief: return the action that is part of this rule
	*/
	Action *getAction();

	/**
	*	@brief: two rules are equal if they have the same flow ID
	*/
	bool operator==(const Rule &other) const;
};

}

#endif //HIHG_LEVEL_RULE_H_
