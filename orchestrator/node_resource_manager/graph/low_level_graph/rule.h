#ifndef RULE_H_
#define RULE_H_ 1

#pragma once

#include "low_level_match.h"
#include "output_action.h"
#include "../../../utils/logger.h"
#include "../../graph_manager/lsi.h"

#include <ostream>

//XXX: the following two constants are not exported by rofl common. Hence they are redefined in this file.
#define OF1X_GROUP_ANY	0xffffffff
#define OF1X_PORT_ANY	0xffffffff

using namespace std;

namespace lowlevel
{

typedef enum{ADD_RULE,RM_RULE}commad_t;

class Rule
{
private:
	uint64_t priority;
	Match match;
	Action action;

	string flowID;

public:
	Rule(Match match, Action action, string flowID, uint64_t priority);

	bool operator==(const Rule &other) const;

	/**
	*	@brief: translate the rule into a flowmod message
	*
	*	@param: message		flowmod message
	*	@param: of_version	openflow version of the flowmod message
	*/
	void fillFlowmodMessage(rofl::openflow::cofflowmod &message, uint8_t of_version, commad_t command);

	/**
	*	@brief: return the identifier of this rule
	*/
	string getID();

	/**
	*	@brief: set the identifier of this rule
	*/
	void setID(string id);

	void print();
	void prettyPrint(LSI *lsi0,map<string,LSI *> lsis);
};

}

#endif //RULE_H_
