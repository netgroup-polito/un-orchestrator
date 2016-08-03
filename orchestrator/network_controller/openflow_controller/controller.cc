#include "controller.h"

static const char LOG_MODULE_NAME[] = "OpenFLow-Controller";

Controller::Controller(rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap,Graph graph, unsigned controllerPort)	:
	thread(this),
	dpt(NULL),
	isOpen(false),
	graph(graph),
	controllerPort(controllerPort)
{
	set_versionbitmap(versionbitmap);
	pthread_mutex_init(&controller_mutex, NULL);
}

void Controller::start()
{
	rofl::csockaddr bind_addr(AF_INET, std::string("0.0.0.0"), controllerPort);
	dpt_sock_listen(bind_addr);
	thread.start();
}

void Controller::handle_dpt_open(crofdpt& dpt)
{
	pthread_mutex_lock(&controller_mutex);

	ULOG_DBG_INFO("Connection with the datapath is open (using the TCP port %u)!",controllerPort);

	dpt.flow_mod_reset();
	switch(OFP_VERSION)
	{
		case OFP_10:
			//Groups does not exist in Openflow 1.0
			break;
		case OFP_12:
		case OFP_13:
			dpt.group_mod_reset();
			break;
	}




	this->dpt = &dpt;
	isOpen = true;

	installNewRulesIntoLSI(graph.getRules());

	pthread_mutex_unlock(&controller_mutex);
}

void Controller::handle_dpt_close(crofdpt& dpt)
{
	isOpen = false;
	this->dpt = NULL;
	ULOG_DBG_INFO("Connection with the datapath is closed (the connection was on the TCP port %u)",controllerPort);
}

/*void Controller::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid,rofl::openflow::cofmsg_packet_in& msg)
{
	dpt.send_packet_out_message(auxid, msg.get_buffer_id(), in_port, actions);
}*/


bool Controller::installNewRule(Rule rule)
{
	pthread_mutex_lock(&controller_mutex);

	list<Rule> rules;
	rules.push_back(rule);

	//Add the rule to the whole graph
	graph.addRule(rule);

	bool retVal = installNewRulesIntoLSI(rules);

	pthread_mutex_unlock(&controller_mutex);

	return retVal;
}

bool Controller::installNewRules(list<Rule> rules)
{
	pthread_mutex_lock(&controller_mutex);

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
		graph.addRule(*r);

	bool retVal = installNewRulesIntoLSI(rules);

	pthread_mutex_unlock(&controller_mutex);

	return retVal;
}

bool Controller::removeRules(list<Rule> rules)
{
	pthread_mutex_lock(&controller_mutex);

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
		graph.removeRule(*r);

	//TODO: removes, from LSI, only rules that do not still appear in the graph
	bool retVal = removeRulesFromLSI(rules);
	pthread_mutex_unlock(&controller_mutex);;
	return retVal;
}

bool Controller::removeRuleFromID(string ID)
{
	//FIXME: is retVal useful?
	bool retVal = false;

	pthread_mutex_lock(&controller_mutex);

	try
	{
		Rule rule = graph.getRule(ID);

		if(!graph.removeRuleFromID(ID))
		{
			//The graph does not contain another rule equal to the
			//one removed. Then the flowmod can be sent
			list<Rule> rules;
			rules.push_back(rule);
			retVal = removeRulesFromLSI(rules);
		}
	}catch(...)
	{
		//No problem.. This means that the rule with ID has not been lowered in this graph.
		//This is ok, since some rules have a lowering just into the LSI-0 or tenant-LSI.
	}
	pthread_mutex_unlock(&controller_mutex);

	return retVal;
}

bool Controller::installNewRulesIntoLSI(list<Rule> rules)
{
	if(rules.size() == 0)
	{
		ULOG_DBG_INFO("No rule to be installed.");
		return true;
	}

	if(isOpen)
	{
		ULOG_DBG_INFO("Installing (%d) new rules!",rules.size());

		list<Rule>::iterator rule = rules.begin();
		for(; rule != rules.end(); rule++)
		{
			ULOG_DBG_INFO("Installing rule %s",rule->getID().c_str());
			rofl::openflow::cofflowmod fe(dpt->get_version());
			rule->fillFlowmodMessage(fe,dpt->get_version(),ADD_RULE);
#ifdef DEBUG_OPENFLOW
			std::cout << "installing new Flow-Mod entry:" << std::endl << fe;
#endif
			dpt->send_flow_mod_message(cauxid(0),fe);
		}
		ULOG_DBG_INFO("%d rules installed!",rules.size());
		return true;
	}

	ULOG_WARN("No datapath connected at the TCP port %u! Cannot install rules!",controllerPort);

	return false;
}

bool Controller::removeRulesFromLSI(list<Rule> rules)
{
	if(isOpen)
	{
		ULOG_DBG_INFO("Removing (%d) rules!",rules.size());

		list<Rule>::iterator rule = rules.begin();
		for(; rule != rules.end(); rule++)
		{
			rofl::openflow::cofflowmod fe(dpt->get_version());
			rule->fillFlowmodMessage(fe,dpt->get_version(),RM_RULE);
#ifdef DEBUG_OPENFLOW
			std::cout << "Removing Flow-Mod entry:" << std::endl << fe;
#endif
			dpt->send_flow_mod_message(cauxid(0),fe);
		}
		return true;
	}

	ULOG_WARN("No datapath connected! Cannot remove rules!");

	return false;
}

unsigned Controller::getControllerPort()
{
	return controllerPort;
}

