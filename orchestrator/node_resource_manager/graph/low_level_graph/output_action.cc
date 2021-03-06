#include "output_action.h"

namespace lowlevel
{

Action::Action(action_t type):type(type)
{
	if (type==ACTION_LOCAL)
		ports_id.push_back(rofl::openflow::OFPP_LOCAL);
	else if(type==ACTION_NORMAL)
		ports_id.push_back(rofl::openflow::OFPP_NORMAL);
}

void Action::addOutputPort(unsigned int port_id)
{
	ports_id.push_back(port_id);
}

bool Action::operator==(const Action &other) const
{
	if((type != other.type) || (ports_id.size() != other.ports_id.size()))
		return false;
	for(list<unsigned int>::const_iterator port_id = this->ports_id.begin(); port_id != this->ports_id.end(); port_id++)
	{
		bool found = false;
		for(list<unsigned int>::const_iterator port_id_other = other.ports_id.begin(); port_id_other != other.ports_id.end(); port_id_other++)
		{
			if(*port_id==*port_id_other)
			{
				found=true;
				break;
			}
		}
		if(!found)
			return false;
	}
	return true;
}

void Action::fillFlowmodMessage(rofl::openflow::cofflowmod &message)
{
	using rofl::cindex;

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
					message.set_actions().add_action_output(cindex(position)).set_port_no(*outputPort);
				break;
			case OFP_12:
			case OFP_13:
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

		if(type==ACTION_LOCAL)
			cout << "\t\t\tOUTPUT: " << "LOCAL" << endl;
		else if(type==ACTION_NORMAL)
			cout << "\t\t\tOUTPUT: " << "NORMAL" << endl;
		else
		{
			for(list<unsigned int>::iterator outputPort = ports_id.begin(); outputPort != ports_id.end(); outputPort++)
				cout << "\t\t\tOUTPUT: " << *outputPort << endl;
		}
		for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
			(*ga)->print();
		cout << "\t\t}" << endl;
	}
}

string Action::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	stringstream ss;

	ss << "output to ";
	bool foundOne=false;

	map<string,unsigned int> pysicalPorts = lsi0->getPhysicalPorts();
	for(map<string,unsigned int>::iterator it = pysicalPorts.begin(); it != pysicalPorts.end(); it++)
	{
		for(list<unsigned int>::iterator outputPort = ports_id.begin(); outputPort != ports_id.end(); outputPort++)
		{
			if(it->second == *outputPort)
			{
				if(foundOne)
					ss << ", output to ";
				else
					foundOne=true;
				ss << it->first;
				break;
			}
		}
	}
	if(foundOne)
		return ss.str();

	//The port corresponds to a virtual link... we search the corresponding graph
	for(map<string,LSI *>::iterator it = lsis.begin(); it != lsis.end(); it++)
	{
		vector<VLink> vlinks = it->second->getVirtualLinks();
		for(vector<VLink>::iterator vl = vlinks.begin(); vl != vlinks.end(); vl++)
		{
			for(list<unsigned int>::iterator outputPort = ports_id.begin(); outputPort != ports_id.end(); outputPort++)
			{
				if(vl->getRemoteID() == *outputPort)
				{
					if(foundOne)
						ss << ", output to ";
					else
						foundOne=true;
					ss << *outputPort;
					break;
				}
			}
		}
	}

	if(!foundOne)
	{
		if(type==ACTION_LOCAL)
			ss << "LOCAL" << " (LOCAL graph)";
		else if(type==ACTION_NORMAL)
			ss << "NORMAL" << " (INTERNAL graph)";
		else
		{
			//The code could be here only when a SIGINT is received and all the graph are going to be removed
			for(list<unsigned int>::iterator outputPort = ports_id.begin(); outputPort != ports_id.end(); outputPort++)
			{
				ss << *outputPort << ", ";
			}
			ss<<" (unknown graph)";
		}
	}

	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		ss << (*ga)->prettyPrint();

	return ss.str();
}

void Action::addGenericAction(GenericAction *ga)
{
	genericActions.push_back(ga);
}

}
