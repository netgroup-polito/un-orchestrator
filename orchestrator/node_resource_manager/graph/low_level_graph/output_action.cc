#include "output_action.h"

namespace lowlevel
{

Action::Action()
	: type(openflow::OFPAT_OUTPUT), is_local_port(false), is_normal(false)
{

}

Action::Action(bool is_local_port)
	: type(openflow::OFPAT_OUTPUT), is_local_port(is_local_port), is_normal(false)
{

}

Action::Action(bool is_local_port, bool is_normal)
		: type(openflow::OFPAT_OUTPUT), is_local_port(is_local_port), is_normal(is_normal)
{

}

void Action::addOutputPort(unsigned int port_id)
{
	ports_id.push_back(port_id);
}

bool Action::operator==(const Action &other) const
{
	if((type != other.type) && (ports_id.front() == other.ports_id.front()))
		return false;
	//for(list<unsigned int>::iterator outputPort = ports_id.begin(); outputPort != ports_id.end(); outputPort++)
	// TODO: check if ports_id list are equal!!

	return true;
}

openflow::ofp_action_type Action::getActionType()
{
	return type;
}

void Action::fillFlowmodMessage(rofl::openflow::cofflowmod &message)
{
	//Before inserting the output action, the other actions are considered
	unsigned int position = 0;
	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		(*ga)->fillFlowmodMessage(message,&position);

	//Now we can consider the output actions
	for(list<unsigned int>::iterator outputPort = ports_id.begin(); outputPort != ports_id.end(); outputPort++)
	{
		switch(OFP_VERSION)
		{
			case OFP_10:
				if(is_local_port)
					message.set_actions().add_action_output(cindex(position)).set_port_no(rofl::openflow::OFPP_LOCAL);
				else if(is_normal)
					message.set_actions().add_action_output(cindex(position)).set_port_no(rofl::openflow::OFPP_NORMAL);
				else
					message.set_actions().add_action_output(cindex(position)).set_port_no(*outputPort);
				break;
			case OFP_12:
			case OFP_13:
				if(is_local_port)
					message.set_instructions().set_inst_apply_actions().set_actions().add_action_output(cindex(position)).set_port_no(rofl::openflow::OFPP_LOCAL);
				else if(is_normal)
					message.set_instructions().set_inst_apply_actions().set_actions().add_action_output(cindex(position)).set_port_no(rofl::openflow::OFPP_NORMAL);
				else
					message.set_instructions().set_inst_apply_actions().set_actions().add_action_output(cindex(position)).set_port_no(*outputPort);
				break;
		}
		position++;
	}
}

void Action::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tAction:" << endl << "\t\t{" << endl;
		if(is_local_port)
			cout << "\t\t\tOUTPUT: " << "LOCAL" << endl;
		else if(is_normal)
			cout << "\t\t\tOUTPUT: " << "NORMAL" << endl;
		else
			cout << "\t\t\tOUTPUT: " << ports_id.front() << endl; // TODO: fix this (.front() is not correct!)
		for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
			(*ga)->print();
		cout << "\t\t}" << endl;
	}
}

string Action::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	stringstream ss;

	ss << "output to ";

	map<string,unsigned int> pysicalPorts = lsi0->getPhysicalPorts();
	for(map<string,unsigned int>::iterator it = pysicalPorts.begin(); it != pysicalPorts.end(); it++)
	{
		if(it->second == ports_id.front())// TODO: fix this (.front() is not correct!)
		{
			ss << it->first;
			return ss.str();
		}
	}

	//The port corresponds to a virtual link... we search the corresponding graph

	for(map<string,LSI *>::iterator it = lsis.begin(); it != lsis.end(); it++)
	{
		vector<VLink> vlinks = it->second->getVirtualLinks();
		for(vector<VLink>::iterator vl = vlinks.begin(); vl != vlinks.end(); vl++)
		{
			if(vl->getRemoteID() == ports_id.front())// TODO: fix this (.front() is not correct!)
			{
				ss << ports_id.front() << " (graph: " << it->first << ")";
				goto conclude;
			}
		}
	}

	if(is_local_port)
		ss << "LOCAL" << " (LOCAL graph)";
	else if(is_normal)
		ss << "NORMAL" << " (INTERNAL graph)";
	else
	{
		//The code could be here only when a SIGINT is received and all the graph are going to be removed
		ss << ports_id.front() << " (unknown graph)";// TODO: fix this (.front() is not correct!)
	}

conclude:

	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		ss << (*ga)->prettyPrint();

	return ss.str();
}

void Action::addGenericAction(GenericAction *ga)
{
	genericActions.push_back(ga);
}

}
