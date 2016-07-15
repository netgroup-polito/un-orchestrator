#include "controller.h"

Controller::Controller(rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap,Graph graph,string controllerPort)	:
	crofbase(versionbitmap, rofl::cioloop::add_thread()),
	dpt(NULL),
	isOpen(false),
	graph(graph),
	controllerPort(controllerPort)
{
	pthread_mutex_init(&controller_mutex, NULL);
}

void Controller::start()
{
	pthread_t thread[1];
	pthread_create(&thread[0],NULL,loop,this);
#ifdef __x86_64__
	//the following function is not available on all platforms
	pthread_setname_np(thread[0],"Openflow-Controller");
#endif
}

void Controller::handle_dpt_open(crofdpt& dpt)
{
	pthread_mutex_lock(&controller_mutex);

	logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "Connection with the datapath is open (using the TCP port %s)!",controllerPort.c_str());

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
	logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "Connection with the datapath is closed (the connection was on the TCP port %s)",controllerPort.c_str());
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
		logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "No rule to be installed.");
		return true;
	}

	if(isOpen)
	{
		logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "Installing (%d) new rules!",rules.size());

		list<Rule>::iterator rule = rules.begin();
		for(; rule != rules.end(); rule++)
		{
			logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "Installing rule %s",rule->getID().c_str());
			rofl::openflow::cofflowmod fe(dpt->get_version_negotiated());
			rule->fillFlowmodMessage(fe,dpt->get_version_negotiated(),ADD_RULE);
#ifdef DEBUG_OPENFLOW
			std::cout << "installing new Flow-Mod entry:" << std::endl << fe;
#endif
			dpt->send_flow_mod_message(cauxid(0),fe);
		}
		logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "%d rules installed!",rules.size());
		return true;
	}

	logger(ORCH_WARNING, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "No datapath connected at the TCP port %s! Cannot install rules!",controllerPort.c_str());

	return false;
}

bool Controller::removeRulesFromLSI(list<Rule> rules)
{
	if(isOpen)
	{
		logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "Removing (%d) rules!",rules.size());

		list<Rule>::iterator rule = rules.begin();
		for(; rule != rules.end(); rule++)
		{
			rofl::openflow::cofflowmod fe(dpt->get_version_negotiated());
			rule->fillFlowmodMessage(fe,dpt->get_version_negotiated(),RM_RULE);
#ifdef DEBUG_OPENFLOW
			std::cout << "Removing Flow-Mod entry:" << std::endl << fe;
#endif
			dpt->send_flow_mod_message(cauxid(0),fe);
		}
		return true;
	}

	logger(ORCH_WARNING, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "No datapath connected! Cannot remove rules!");

	return false;
}

void *Controller::loop(void *param)
{
	Controller *controller = (Controller*)param;

	rofl::cparams socket_params = csocket::get_default_params(rofl::csocket::SOCKET_TYPE_PLAIN);
	socket_params.set_param(csocket::PARAM_KEY_LOCAL_PORT).set_string() = controller->controllerPort;

	controller->add_dpt_listening(0,rofl::csocket::SOCKET_TYPE_PLAIN, socket_params);

	if(LOGGING_LEVEL <= ORCH_DEBUG)
		rofl::logging::set_debug_level(7);

	logger(ORCH_DEBUG_INFO, OFCONTROLLER_MODULE_NAME, __FILE__, __LINE__, "Openflow controller is going to start on the TCP port %s...",controller->controllerPort.c_str());

	rofl::cioloop::get_loop().run();

	assert(0 && "Cannot be here!");

	rofl::cioloop::get_loop().shutdown();

	return NULL;
}

string Controller::getControllerPort()
{
	return controllerPort;
}

