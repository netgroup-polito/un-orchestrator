#include "rule.h"

namespace lowlevel
{

Rule::Rule(Match match, Action action, string flowID, uint64_t priority) :
	priority(priority), match(match), action(action), flowID(flowID) {};

//XXX: the flowID is not considered. In fact, this operator
//is used to check if two rules have the same match and the
//same action, regardless of their ID
bool Rule::operator==(const Rule &other) const
{
	if((priority == other.priority) && (action == other.action) && (match == other.match))
		//The two rules are identical
		return true;

	return false;
}

void Rule::fillFlowmodMessage(rofl::openflow::cofflowmod &message, uint8_t of_version, commad_t command)
{
	message.set_table_id(0);

	switch (of_version)
	{
		case openflow10::OFP_VERSION:
		{
			if(command == ADD_RULE)
				message.set_command(openflow10::OFPFC_ADD);
			else
				message.set_command(openflow10::OFPFC_DELETE_STRICT);
			break;
		}
		case openflow12::OFP_VERSION:
		{
			if(command == ADD_RULE)
				message.set_command(openflow12::OFPFC_ADD);
			else
				message.set_command(openflow12::OFPFC_DELETE_STRICT);
			break;
		}
		case openflow13::OFP_VERSION:
		{
			if(command == ADD_RULE)
				message.set_command(openflow13::OFPFC_ADD);
			else
				message.set_command(openflow13::OFPFC_DELETE_STRICT);
			break;
		}
		default:
				throw eBadVersion();
	}

	message.set_idle_timeout(0);
	message.set_hard_timeout(0);

	message.set_priority(priority);

	match.fillFlowmodMessage(message);

	if(command == ADD_RULE)
		//The action is only useful when a new entry is added
		action.fillFlowmodMessage(message);
	else
	{
		//Set the out_port and the out_group to any
		message.set_out_group(OF1X_GROUP_ANY);
		message.set_out_port(OF1X_PORT_ANY);
	}
}

string Rule::getID()
{
	return flowID;
}

void Rule::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\trule " << flowID << ": " << endl << "\t{" << endl;
		stringstream prior;
		prior << priority;
		cout << "\t\tpriority : " << prior.str() << endl;
		match.print();
		action.print();
		cout << "\t}" << endl;
	}
}

void Rule::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	int id;
	sscanf(flowID.c_str(),"%d",&id);

	if(id == 2)
		coloredLogger(ANSI_COLOR_BLUE ,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tPriority-> %" PRIu64 " -\t Match-> %s - \t Action-> output to '%s'",priority,match.prettyPrint(lsi0,lsis).c_str(),action.prettyPrint(lsi0,lsis).c_str());
	else if(id == 3)
		coloredLogger(ANSI_COLOR_RED ,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tPriority-> %" PRIu64 " -\t Match-> %s - \t Action-> output to '%s'",priority,match.prettyPrint(lsi0,lsis).c_str(),action.prettyPrint(lsi0,lsis).c_str());
	else
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tPriority-> %" PRIu64 " -\t Match-> %s - \t Action-> %s",priority,match.prettyPrint(lsi0,lsis).c_str(),action.prettyPrint(lsi0,lsis).c_str());
}

}
