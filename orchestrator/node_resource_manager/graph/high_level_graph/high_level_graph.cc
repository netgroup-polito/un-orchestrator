#include "high_level_graph.h"

namespace highlevel
{

Graph::Graph(string ID) :
	ID(ID)
{
}

string Graph::getID()
{
	return ID;
}

void Graph::setName(string name)
{
	this->name = name;
}

string Graph::getName()
{
	return name;
}

bool Graph::addEndPointInterface(EndPointInterface endpoint)
{
	for(list<EndPointInterface>::iterator e = endPointsInterface.begin(); e != endPointsInterface.end(); e++)
	{
		if(*e == endpoint)
			return false;
	}

	endPointsInterface.push_back(endpoint);

	return true;
}

list<EndPointInterface> Graph::getEndPointsInterface()
{
	return endPointsInterface;
}

bool Graph::addEndPointGre(EndPointGre endpoint)
{
	for(list<EndPointGre>::iterator e = endPointsGre.begin(); e != endPointsGre.end(); e++)
	{
		if(*e == endpoint)
			return false;
	}

	endPointsGre.push_back(endpoint);

	return true;
}

list<EndPointGre> Graph::getEndPointsGre()
{
	return endPointsGre;
}

bool Graph::addEndPointVlan(EndPointVlan endpoint)
{
	for(list<EndPointVlan>::iterator e = endPointsVlan.begin(); e != endPointsVlan.end(); e++)
	{
		if(*e == endpoint)
			return false;
	}

	endPointsVlan.push_back(endpoint);

	return true;
}

list<EndPointVlan> Graph::getEndPointsVlan()
{
	return endPointsVlan;
}

bool Graph::addVNF(VNFs vnf)
{
	for(list<VNFs>::iterator v = vnfs.begin(); v != vnfs.end(); v++)
	{
		if(*v == vnf)
			return false;
	}

	vnfs.push_back(vnf);

	return true;
}

list<VNFs> Graph::getVNFs()
{
	return vnfs;
}

set<string> Graph::getPorts()
{
	return ports;
}

map<string, list<unsigned int> > Graph::getNetworkFunctions()
{
	return networkFunctions;
}

map<string, map<unsigned int, port_network_config > > Graph::getNetworkFunctionsConfiguration()
{
	return networkFunctionsConfiguration;
}

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
map<string, list<port_mapping_t > > Graph::getNetworkFunctionsControlPorts()
{
	return networkFunctionsControlPorts;
}

map<string, list<string> > Graph::getNetworkFunctionsEnvironmentVariables()
{
	return networkFunctionsEnvironmentVariables;
}
#endif

#ifdef ENABLE_UNIFY_MONITORING_CONTROLLER
string Graph::getMeasureString()
{
	return measureString;
}
#endif

list<Rule> Graph::getRules()
{
	return rules;
}

bool Graph::addPort(string port)
{
	if(ports.count(port) != 0)
		return false;

	ports.insert(port);

	return true;
}

bool Graph::addNetworkFunction(string nf)
{
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "addNetworkFunction(\"%s\")",nf.c_str());

	if(networkFunctions.count(nf) != 0) {
		logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "addNetworkFunction(\"%s\") NF already present!",nf.c_str());
		return false;
	}

	list<unsigned int> ports;
	networkFunctions[nf] = ports;

	return true;
}

bool Graph::addNetworkFunctionPortConfiguration(string nf, map<unsigned int, port_network_config_t > config)
{
	networkFunctionsConfiguration[nf] = config;

	return true;
}

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
void Graph::addNetworkFunctionControlPort(string nf, port_mapping_t control)
{
	networkFunctionsControlPorts[nf].push_back(control);
}

void Graph::addNetworkFunctionEnvironmentVariable(string nf, string env_var)
{
	networkFunctionsEnvironmentVariables[nf].push_back(env_var);
}
#endif

#ifdef ENABLE_UNIFY_MONITORING_CONTROLLER
void Graph::setMeasureString(string measureString)
{
	this->measureString.clear();
	this->measureString = measureString;
}
#endif

bool Graph::updateNetworkFunction(string nf, unsigned int port)
{
	if(networkFunctions.count(nf) == 0)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "NF \"%s\" does not exist",nf.c_str());
		return false;
	}
	list<unsigned int> ports = networkFunctions.find(nf)->second;

	ports.push_back(port);

	networkFunctions[nf] = ports;

	return true;
}

bool Graph::addRule(Rule rule)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		if(*r == rule)
			return false;
	}

	rules.push_back(rule);

	return true;
}

bool Graph::ruleExists(string ID)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		if(r->getFlowID() == ID)
			return true;
	}
	return false;
}

Rule Graph::getRuleFromID(string ID)
{
	list<Rule>::iterator r = rules.begin();

	for(; r != rules.end(); r++)
	{
		if(r->getFlowID() == ID)
			return *r;
	}

	assert(0);

	//This cannot happen; it is just for the compiler.
	return *r;
}

RuleRemovedInfo Graph::removeRuleFromID(string ID)
{
	RuleRemovedInfo rri;

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		if(r->getFlowID() == ID)
		{
			Match match = r->getMatch();
			Action *action = r->getAction();

			action_t actionType = action->getType();
			bool matchOnPort = match.matchOnPort();
			bool matchOnNF = match.matchOnNF();

			if(actionType == ACTION_ON_PORT)
			{
				//Removed an action on a port. It is possible that a vlink must be removed
				rri.port = ((ActionPort*)action)->getInfo();
				rri.isNFport = false;
				rri.isPort = true;
				rri.isEndpoint = false;

				rri.ports.push_back(rri.port);
			}
			else if(actionType == ACTION_ON_NETWORK_FUNCTION)
			{
				//Removed an action on a NF. It is possible that a vlink must be removed
				stringstream nf_port;
				nf_port << ((ActionNetworkFunction*)action)->getInfo() << "_" << ((ActionNetworkFunction*)action)->getPort();
				rri.nf_port = nf_port.str();

				//Potentially, the NF is useless in the graph
				rri.nfs.push_back(((ActionNetworkFunction*)action)->getInfo());
				rri.isNFport = true;
				rri.isPort = false;
				rri.isEndpoint = false;
			}
			else
			{
				//Removed an action on an endpoint
				assert(actionType == ACTION_ON_ENDPOINT);
				rri.endpoint = action->toString();

				rri.isNFport = false;
				rri.isPort = false;
				rri.isEndpoint = true;
			}

			if(matchOnNF)
				//Potentially, the NF is useless in the graph
				rri.nfs.push_back(match.getNF());
			else if(matchOnPort)
				//Potentially, the port is useless in the graph
				rri.ports.push_back(match.getPhysicalPort());
			else
			{
				stringstream ss;
				ss << match.getEndPoint();
				rri.endpoint = ss.str();
			}

			//finally, remove the rule!
			rules.erase(r);

			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "The graph still contains the rules: ");
			for(list<Rule>::iterator print = rules.begin(); print != rules.end(); print++)
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t%s",print->getFlowID().c_str());

			return rri;
		}//end if(r->getFlowID() == ID)
	}

	assert(0);

	//Just for the compiler
	return rri;
}

int Graph::getNumberOfRules()
{
	return rules.size();
}

void Graph::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "Graph :" << endl << "{" << endl;
		for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
			r->print();
		cout << "}" << endl;
	}
}

Object Graph::toJSON()
{
	Object forwarding_graph, big_switch;

	Array flow_rules, end_points, vnf;
	for(list<Rule>::iterator r = rules.begin(); r != rules.end();r++)
	{
		flow_rules.push_back(r->toJSON());
	}

	for(list<EndPointInterface>::iterator e = endPointsInterface.begin(); e != endPointsInterface.end();e++)
	{
		end_points.push_back(e->toJSON());
	}

	for(list<EndPointGre>::iterator e = endPointsGre.begin(); e != endPointsGre.end();e++)
	{
		end_points.push_back(e->toJSON());
	}

	for(list<EndPointVlan>::iterator e = endPointsVlan.begin(); e != endPointsVlan.end();e++)
	{
		end_points.push_back(e->toJSON());
	}

	for(list<VNFs>::iterator v = vnfs.begin(); v != vnfs.end();v++)
	{
		vnf.push_back(v->toJSON());
	}

	forwarding_graph[_ID] = ID;
	forwarding_graph[_NAME] = name;
#ifdef ENABLE_UNIFY_MONITORING_CONTROLLER
	forwarding_graph[UNIFY_MONITORING] = measureString;
#endif
	if(end_points.size() != 0)
		forwarding_graph[END_POINTS] = end_points;
	if(vnf.size() != 0)
		forwarding_graph[VNFS] = vnf;
	big_switch[FLOW_RULES] = flow_rules;

	forwarding_graph[BIG_SWITCH] = big_switch;

	return forwarding_graph;
}

bool Graph::stillExistNF(string nf)
{
	if(networkFunctions.count(nf) == 0)
		return false;

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();
		Action *action = r->getAction();

		action_t actionType = action->getType();
		bool matchOnNF = match.matchOnNF();

		if(matchOnNF)
		{
			if(match.getNF() == nf)
				//The NF still exists into the graph
				return true;
		}

		if(actionType == ACTION_ON_NETWORK_FUNCTION)
		{
			if(((ActionNetworkFunction*)action)->getInfo() == nf)
				//The NF still exist into the graph
				return true;
		}
	}

	networkFunctions.erase(nf);

	list<VNFs>::iterator vnf = vnfs.begin();
	for(; vnf != vnfs.end(); vnf++)
	{
		if(nf == vnf->getName())
		{
			vnfs.erase(vnf);
			break;
		}
	}

	assert(vnf != vnfs.end());

	return false;
}

bool Graph::stillExistEndpoint(string endpoint)
{
	if(endpoints.count(endpoint) == 0)
		return false;
	return true;

	endpoints.erase(endpoint);
	return false;
}

bool Graph::stillExistPort(string port)
{
	if(ports.count(port) == 0)
		return false;

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();
		Action *action = r->getAction();

		action_t actionType = action->getType();
		bool matchOnPort = match.matchOnPort();

		if(matchOnPort)
		{
			if(match.getPhysicalPort() == port)
				//The port still exists into the graph
				return true;
		}

		if(actionType == ACTION_ON_PORT)
		{
			if(((ActionPort*)action)->getInfo() == port)
				//The port still exist into the graph
				return true;
		}
	}

	ports.erase(port);

	return false;
}

bool Graph::addEndPoint(string ep, vector<string> p)
{
	if(endpoints.count(ep) != 0)
		return false;

	endpoints[ep] = p;

	return true;
}

map<string, vector<string> > Graph::getEndPoints()
{
	return endpoints;
}

string Graph::getEndpointInvolved(string flowID)
{
	highlevel::Rule r = getRuleFromID(flowID);
	highlevel::Match m = r.getMatch();
	highlevel::Action *a = r.getAction();

	if(a->getType() == highlevel::ACTION_ON_ENDPOINT)
		return a->toString();

	if(m.matchOnEndPoint())
	{
		stringstream ss;
		ss << m.getEndPoint();
		return ss.str();
	}

	return "";
}

}
