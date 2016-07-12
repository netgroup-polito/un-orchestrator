#include "high_level_rule.h"

namespace highlevel
{

Rule::Rule(Match match, Action *action,string ruleID, uint64_t priority) :
	match(match), action(action), priority(priority), ruleID(ruleID)
{
}

bool Rule::operator==(const Rule &other) const
{
	if(ruleID == other.ruleID)
		//The two rules have the same ID
		return true;

	return false;
}

string Rule::getRuleID()
{
	return ruleID;
}

uint64_t Rule::getPriority()
{
	return priority;
}

Match Rule::getMatch()
{
	return match;
}

Action *Rule::getAction()
{
	return action;
}

Object Rule::toJSON()
{
	Object rule;
	Array actions;

	rule[_ID] = ruleID.c_str();
	rule[PRIORITY] = priority;
	rule[MATCH] = match.toJSON();
	actions.push_back(action->toJSON());
	rule[ACTIONS] = actions;

	return rule;
}

}
