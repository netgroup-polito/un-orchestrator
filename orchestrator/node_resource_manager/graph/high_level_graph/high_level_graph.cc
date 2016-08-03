#include "high_level_graph.h"

static const char LOG_MODULE_NAME[] = "High-Level-Graph";

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

void Graph::removeEndPointInterface(EndPointInterface endpoint)
{
	for(list<EndPointInterface>::iterator e = endPointsInterface.begin(); e != endPointsInterface.end(); e++)
	{
		if(*e == endpoint)
		{
			endPointsInterface.erase(e);
			return;
		}
	}
	assert(0);
}

bool Graph::addEndPointInternal(EndPointInternal endpoint)
{
	for(list<EndPointInternal>::iterator e = endPointsInternal.begin(); e != endPointsInternal.end(); e++)
	{
		if(*e == endpoint)
			return false;
	}

	endPointsInternal.push_back(endpoint);

	endpoints[endpoint.getGroup()] = true;

	return endpoints[endpoint.getGroup()];
}

list<EndPointInternal> Graph::getEndPointsInternal()
{
	return endPointsInternal;
}

void Graph::removeEndPointInternal(EndPointInternal endpoint)
{
	for(list<EndPointInternal>::iterator e = endPointsInternal.begin(); e != endPointsInternal.end(); e++)
	{
		if(*e == endpoint)
		{
			endPointsInternal.erase(e);
			return;
		}
	}
	assert(0);
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

void Graph::removeEndPointGre(EndPointGre endpoint)
{
	for(list<EndPointGre>::iterator e = endPointsGre.begin(); e != endPointsGre.end(); e++)
	{
		if(*e == endpoint)
		{
			endPointsGre.erase(e);
			return;
		}
	}
	assert(0);
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

void Graph::removeEndPointVlan(EndPointVlan endpoint)
{
	for(list<EndPointVlan>::iterator e = endPointsVlan.begin(); e != endPointsVlan.end(); e++)
	{
		if(*e == endpoint)
		{
			endPointsVlan.erase(e);
			return;
		}
	}
	assert(0);
}

void Graph::addVNF(VNFs vnf)
{
	for(list<VNFs>::iterator v = vnfs.begin(); v != vnfs.end(); v++)
	{
		if(*v == vnf)
		{
			//The vnf is already part of the graph. But we have to check that also the ports are the same.
			//In case new ports are specified, the VNF is updated with the new ports
			list<vnf_port_t> ports = vnf.getPorts();
			for(list<vnf_port_t>::iterator p = ports.begin(); p != ports.end(); p++)
				v->addPort(*p);	//This function adds the port only if it not part of the VNF yet
			return;
		}
	}

	//The VNF is not yet part of the graph
	vnfs.push_back(vnf);
	return;
}

list<VNFs> Graph::getVNFs()
{
	return vnfs;
}


list<Rule> Graph::getRules()
{
	return rules;
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
		if(r->getRuleID() == ID)
			return true;
	}
	return false;
}

Rule Graph::getRuleFromID(string ID)
{
	list<Rule>::iterator r = rules.begin();

	for(; r != rules.end(); r++)
	{
		if(r->getRuleID() == ID)
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
		if(r->getRuleID() == ID)
		{
			Match match = r->getMatch();
			Action *action = r->getAction();
			action_t actionType = action->getType();

			if(actionType == ACTION_ON_PORT)
			{
				//Removed an action on a port. It is possible that a vlink must be removed
				rri.port = ((ActionPort*)action)->getInfo();
				rri.isNFport = false;
				rri.isPort = true;
				rri.isEndpointInternal = false;
				rri.isEndpointGre = false;
			}
			else if(actionType == ACTION_ON_NETWORK_FUNCTION)
			{
				//Removed an action on a NF. It is possible that a vlink must be removed
				stringstream nf_port;
				nf_port << ((ActionNetworkFunction*)action)->getInfo() << "_" << ((ActionNetworkFunction*)action)->getPort();
				rri.nf_port = nf_port.str();
				rri.isNFport = true;
				rri.isPort = false;
				rri.isEndpointInternal = false;
				rri.isEndpointGre = false;
			}
			else if(actionType == ACTION_ON_ENDPOINT_GRE)
			{
				//Removed an action on an endpoint gre
				rri.endpointGre = action->toString();

				rri.isNFport = false;
				rri.isPort = false;
				rri.isEndpointInternal = false;
				rri.isEndpointGre = true;
			}
			else if(actionType == ACTION_ON_ENDPOINT_INTERNAL)
			{
				//Removed an action on an endpoint internal
				rri.endpointInternal = action->toString();

				rri.isNFport = false;
				rri.isPort = false;
				rri.isEndpointInternal = true;
				rri.isEndpointGre = false;
			}

			//finally, remove the rule!
			rules.erase(r);

			ULOG_DBG("The graph still contains the rules: ");
			for(list<Rule>::iterator print = rules.begin(); print != rules.end(); print++)
				ULOG_DBG("\t%s",print->getRuleID().c_str());

			return rri;
		}//end if(r->getRuleID() == ID)
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
		Object json_diff = this->toJSON();
		stringstream ssj;
		write_formatted(json_diff, ssj );
		string sssj = ssj.str();
		cout << sssj.c_str();
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

	for(list<EndPointInternal>::iterator e = endPointsInternal.begin(); e != endPointsInternal.end();e++)
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
	if(end_points.size() != 0)
		forwarding_graph[END_POINTS] = end_points;
	if(vnf.size() != 0)
		forwarding_graph[VNFS] = vnf;
	big_switch[FLOW_RULES] = flow_rules;

	forwarding_graph[BIG_SWITCH] = big_switch;

	return forwarding_graph;
}

bool Graph::stillUsedVNF(VNFs vnf)
{
	list<VNFs>::iterator the_vnfs = vnfs.begin();
	for(; the_vnfs != vnfs.end(); the_vnfs++)
	{
		if(vnf == (*the_vnfs))
			break;
	}
	if(the_vnfs == vnfs.end())
	{
		//This situation shouldn't happen
		assert(0);
		return false;
	}

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();
		Action *action = r->getAction();

		action_t actionType = action->getType();
		bool matchOnNF = match.matchOnNF();

		if(matchOnNF)
		{
			if(match.getNF() == vnf.getName())
				//The VNF is still used into the graph
				return true;
		}

		if(actionType == ACTION_ON_NETWORK_FUNCTION)
		{
			if(((ActionNetworkFunction*)action)->getInfo() == vnf.getName())
				//The VNF is still used into the graph
				return true;
		}
	}

	//The VNF is not longer used into the graph

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

bool Graph::stillExistEndpointGre(string endpoint)
{
	for(list<highlevel::EndPointGre>::iterator e = endPointsGre.begin(); e != endPointsGre.end(); e++)
	{
		if(e->getId().compare(endpoint) == 0)
			return true;
	}

	return false;
}

bool Graph::stillUsedEndpointInterface(EndPointInterface endpoint)
{
	list<EndPointInterface>::iterator interface = endPointsInterface.begin();
	for(; interface != endPointsInterface.end(); interface++)
	{
		if((*interface) == endpoint)
			break;
	}
	if(interface == endPointsInterface.end())
	{
		//This situation shouldn't happen
		assert(0);
		return false;
	}

	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();
		Action *action = r->getAction();

		action_t actionType = action->getType();
		bool matchOnPort = match.matchOnPort();

		if(matchOnPort)
		{
			if(match.getPhysicalPort() == endpoint.getInterface())
				//The endpoint is still used into the graph
				return true;
		}

		if(actionType == ACTION_ON_PORT)
		{
			if(((ActionPort*)action)->getInfo() == endpoint.getInterface())
				//The endpoint is still used into the graph
				return true;
		}
	}

	//The endpoint is no longer used into the graph

	return false;
}

string Graph::getEndpointInvolved(string flowID)
{
	highlevel::Rule r = getRuleFromID(flowID);
	highlevel::Match m = r.getMatch();
	highlevel::Action *a = r.getAction();

	if(a->getType() == highlevel::ACTION_ON_ENDPOINT_INTERNAL)
		return a->toString();

	if(m.matchOnEndPointInternal())
	{
		stringstream ss;
		ss << m.getInputEndpoint();
		return ss.str();
	}

	return "";
}

list<highlevel::Rule> Graph::calculateDiffRules(Graph *other)
{
	list<highlevel::Rule> new_rules;

	list<highlevel::Rule> other_rules = other->getRules();
	//Iterater over the other rules, and save those that are not part of the current graph
	for(list<highlevel::Rule>::iterator ors = other_rules.begin(); ors != other_rules.end(); ors++)
	{
		list<highlevel::Rule>::iterator r = rules.begin();
		for(; r != rules.end(); r++)
		{
			if((*ors) == (*r))
				//the rule is part of the current graph
				break;
		}
		if(r == rules.end())
			//If we are here, it means that we found a rule that is not part in the current graph
			new_rules.push_back(*ors);
	}
	return new_rules;
}

Graph *Graph::calculateDiff(Graph *other, string graphID)
{
	Graph *diff = new Graph(graphID);

	// a) Add the new rules to "diff"

	//Extract the new rules to be instantiated
	list<highlevel::Rule> newrules = this->calculateDiffRules(other);

	for(list<highlevel::Rule>::iterator rule = newrules.begin(); rule != newrules.end(); rule++)
		diff->addRule(*rule);

	// b) Add the new VNFs to "diff"

	//Retrieve the network functions already instantiated (and convert them in a set)
	list<highlevel::VNFs> vnfs_already_there = this->getVNFs();
	//Retrieve the network functions required by the update, and their configuration
	list<highlevel::VNFs> new_vnfs_required = other->getVNFs();
	for(list<highlevel::VNFs>::iterator it = new_vnfs_required.begin(); it != new_vnfs_required.end(); it++)
	{
		//Check if this VNF is already in the graph
		bool alreadyThere = false;
		for(list<highlevel::VNFs>::iterator there = vnfs_already_there.begin(); there != vnfs_already_there.end(); there++)
		{
			if((*there) == (*it))
			{
				alreadyThere = true;
				//we have to check the ports. In fact the VNF may require new ports

				list<vnf_port_t> ports_needed_by_diff;							//this set will contain the ports needed by the diff
				list<vnf_port_t> vnf_ports_already_there = there->getPorts();	//ports of the VNF before the update
				list<vnf_port_t> vnf_ports_required = it->getPorts();		 	//ports of the VNF required by the update
				for(list<vnf_port_t>::iterator p_required = vnf_ports_required.begin(); p_required != vnf_ports_required.end(); p_required++)
				{
					bool port_already_in_graph = false;
					vnf_port_t required_tmp = *p_required;
					for(list<vnf_port_t>::iterator p_there = vnf_ports_already_there.begin(); p_there != vnf_ports_already_there.end(); p_there++)
					{
						vnf_port_t there_tmp = *p_there;
						if(required_tmp.id == there_tmp.id)
						{
							//The port is already part of the graph
							port_already_in_graph = true;
							break;
						}
					}
					if(!port_already_in_graph)
					{
						ULOG_DBG_INFO("\tThe VNF port with id '%s' is needed for VNF '%s'",required_tmp.id.c_str(),(it->getName()).c_str());
						ports_needed_by_diff.push_back(*p_required);
					}
				}

				//If the VNF requires new ports, it must be added to the "diff" graph (only with the new ports)
				if(ports_needed_by_diff.size() !=0)
				{
					ULOG_DBG_INFO("\tUpdate for VNF '%s' is added to the diff graph",(it->getName()).c_str());
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
					highlevel::VNFs the_vnf(it->getId(), it->getName(), it->getGroups(), it->getVnfTemplate(), ports_needed_by_diff, it->getControlPorts(),it->getEnvironmentVariables());
#else
					highlevel::VNFs the_vnf(it->getId(), it->getName(), it->getGroups(), it->getVnfTemplate(), ports_needed_by_diff);
#endif
					diff->addVNF(the_vnf);
				}

				break;
			}
		}//end itearation on the VNFs already deployed
		if(!alreadyThere)
		{
			ULOG_DBG_INFO("A new VNF is required - ID: '%s' - name: '%s'", (it->getId()).c_str(),(it->getName()).c_str());
			diff->addVNF(*it);
		}
	}//end iteration on the VNFs required by the update

	// c) Add the new endpoints

	// c-1) interface endpoints
	//Retrieve the interface endpoints already existing in the graph
	list<highlevel::EndPointInterface> endpointsInterface = this->getEndPointsInterface();
	//Retrieve the interface endpoints required by the update
	list<highlevel::EndPointInterface> new_endpoints_interface = other->getEndPointsInterface();
	for(list<highlevel::EndPointInterface>::iterator new_interface = new_endpoints_interface.begin(); new_interface != new_endpoints_interface.end(); new_interface++)
	{
		bool found = false;
		//string it = nei->getInterface();

		for(list<highlevel::EndPointInterface>::iterator interface = endpointsInterface.begin(); interface != endpointsInterface.end(); interface++)
		{
			if((*new_interface) == (*interface))
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			diff->addEndPointInterface(*new_interface);
			ULOG_DBG_INFO("Interface endpoint %s is added the to the diff graph",(new_interface->getInterface()).c_str());
		}
		else
			ULOG_DBG("Interface endpoint %s is already in the graph",(new_interface->getInterface()).c_str());
	}

	// c-2) gre-tunnel endpoints
	//Retrieve the gre endpoints already existing in the graph
	list<highlevel::EndPointGre> endpointsGre = this->getEndPointsGre();
	//Retrieve the gre endpoints required by the update
	list<highlevel::EndPointGre> new_endpoints_gre = other->getEndPointsGre();
	for(list<highlevel::EndPointGre>::iterator new_gre = new_endpoints_gre.begin(); new_gre != new_endpoints_gre.end(); new_gre++)
	{
		bool found = false;

		for(list<highlevel::EndPointGre>::iterator gre = endpointsGre.begin(); gre != endpointsGre.end(); gre++)
		{
			if((*new_gre) == (*gre))
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			ULOG_DBG_INFO("GRE endpoint %s is added to the diff graph",new_gre->getId().c_str());
			diff->addEndPointGre(*new_gre);
		}
		else
			ULOG_DBG("GRE endpoint %s is already in the graph",new_gre->getId().c_str());
	}

	// c-3) internal endpoints
	//Retrieve the internal endpoints already existing in the graph
	list<highlevel::EndPointInternal> endpoint_internal = this->getEndPointsInternal();
	//Retrieve the internal endpoints required by the update
	list<highlevel::EndPointInternal> new_endpoints_internal = other->getEndPointsInternal();
	for(list<highlevel::EndPointInternal>::iterator new_internal = new_endpoints_internal.begin(); new_internal != new_endpoints_internal.end(); new_internal++)
	{
		bool found = false;

		for(list<highlevel::EndPointInternal>::iterator internal = endpoint_internal.begin(); internal != endpoint_internal.end(); internal++)
		{
			if((*new_internal) == (*internal))
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			ULOG_DBG_INFO("Internal endpoint %s is added to the diff graph",(new_internal->getGroup()).c_str());
			diff->addEndPointInternal(*new_internal);
		}
		else
			ULOG_DBG("Internal endpoint %s is already in the graph",(new_internal->getGroup()).c_str());
	}

	// c-4) vlan endpoints
	//Retrieve the vlan endpoints already existing in the graph
	list<highlevel::EndPointVlan> endpointsVlan = this->getEndPointsVlan();
	//Retrieve the vlan endpoints required by the update
	list<highlevel::EndPointVlan> new_endpoints_vlan = other->getEndPointsVlan();
	for(list<highlevel::EndPointVlan>::iterator new_vlan = new_endpoints_vlan.begin(); new_vlan != new_endpoints_vlan.end(); new_vlan++)
	{
		bool found = false;

		for(list<highlevel::EndPointVlan>::iterator vlan = endpointsVlan.begin(); vlan != endpointsVlan.end(); vlan++)
		{
			if((*new_vlan) == (*vlan))
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			diff->addEndPointVlan(*new_vlan);
			ULOG_DBG_INFO("Vlan endpoint %s is added to the diff graph",(new_vlan->getVlanId()).c_str());
		}
		else
			ULOG_DBG_INFO("Vlan endpoint %s is already in the graph",(new_vlan)->getVlanId().c_str());
	}

	return diff;
}

bool Graph::addGraphToGraph(highlevel::Graph *other)
{
	//Update the rules
	list<highlevel::Rule> newRules = other->getRules();
	for(list<highlevel::Rule>::iterator rule = newRules.begin(); rule != newRules.end(); rule++)
	{
		if(!this->addRule(*rule))
		{
			ULOG_INFO("The graph has at least two rules with the same ID: %s",rule->getRuleID().c_str());
			return false;
		}
	}

	//Update the interface endpoints
	list<highlevel::EndPointInterface> iep = other->getEndPointsInterface();
	for(list<highlevel::EndPointInterface>::iterator ep = iep.begin(); ep != iep.end(); ep++)
	{
		//The interface endpoint is not part of the graph
		this->addEndPointInterface(*ep);
	}

	//Update the gre-tunnel endpoints
	list<highlevel::EndPointGre> nepp = other->getEndPointsGre();
	for(list<highlevel::EndPointGre>::iterator ep = nepp.begin(); ep != nepp.end(); ep++)
	{
		//The gre endpoint is not part of the graph
		this->addEndPointGre(*ep);
	}

	//Update the internal endpoints
	list<highlevel::EndPointInternal> inep = other->getEndPointsInternal();
	for(list<highlevel::EndPointInternal>::iterator ep = inep.begin(); ep != inep.end(); ep++)
	{
		//The internal endpoint is not part of the graph
		this->addEndPointInternal(*ep);
	}

	//Update the vlan endpoints
	list<highlevel::EndPointVlan> vep = other->getEndPointsVlan();
	for(list<highlevel::EndPointVlan>::iterator ep = vep.begin(); ep != vep.end(); ep++)
	{
		this->addEndPointVlan(*ep);
	}

	//Update the network functions
	list<highlevel::VNFs> vnfs_tobe_added = other->getVNFs();
	//Iterates on the VNFs to be added (i.e., the VNFs that are in "other")
	for(list<highlevel::VNFs>::iterator vtba = vnfs_tobe_added.begin(); vtba != vnfs_tobe_added.end(); vtba++)
		this->addVNF(*vtba); //In case the VNF is already in the graph but now it has new ports, the new ports are added to the graph

	return true;
}

list<RuleRemovedInfo> Graph::removeGraphFromGraph(highlevel::Graph *other)
{
	list<RuleRemovedInfo> toberemoved; //this information will be used to destroy the virtual links on the LSI

	//Update the rules
	list<highlevel::Rule> oldRules = other->getRules();
	for(list<highlevel::Rule>::iterator rule = oldRules.begin(); rule != oldRules.end(); rule++)
	{
		RuleRemovedInfo rule_removed_info = removeRuleFromID(rule->getRuleID());
		toberemoved.push_back(rule_removed_info);
	}

	//Update the interface endpoints
	list<highlevel::EndPointInterface> iep = other->getEndPointsInterface();
	for(list<highlevel::EndPointInterface>::iterator ep = iep.begin(); ep != iep.end(); ep++)
	{
		if(!stillUsedEndpointInterface(*ep))
			//The endpoint is no longer used by the graph
			this->removeEndPointInterface(*ep);
		else
		{
			assert(0);
			//TODO: how to manage properly this situation?
		}
	}

	//Update the gre-tunnel endpoints
	list<highlevel::EndPointGre> nepp = other->getEndPointsGre();
	for(list<highlevel::EndPointGre>::iterator ep = nepp.begin(); ep != nepp.end(); ep++)
	{
		//TODO If a gre-tunnel endpoint is still used in a rule, it cannot
		//be removed! In this case the update is not valid!
		this->removeEndPointGre(*ep);
	}

	//Update the internal endpoints
	list<highlevel::EndPointInternal> inep = other->getEndPointsInternal();
	for(list<highlevel::EndPointInternal>::iterator ep = inep.begin(); ep != inep.end(); ep++)
	{
		//TODO If an internal endpoint is still used in a rule, it cannot
		//be removed! In this case the update is not valid!
		this->removeEndPointInternal(*ep);
	}

	//Update the vlan endpoints
	list<highlevel::EndPointVlan> vep = other->getEndPointsVlan();
	for(list<highlevel::EndPointVlan>::iterator ep = vep.begin(); ep != vep.end(); ep++)
	{
		//TODO If a vlan endpoint is still used in a rule, it cannot
		//be removed! In this case the update is not valid!
		this->removeEndPointVlan(*ep);
	}

	//Update the network functions
	list<highlevel::VNFs> vnfs_tobe_removed = other->getVNFs();
	//Iterates on the VNFs to be removed (i.e., the VNFs that are in "other")
	for(list<highlevel::VNFs>::iterator vtbr = vnfs_tobe_removed.begin(); vtbr != vnfs_tobe_removed.end(); vtbr++)
	{
		//TODO: Here it is important to distinguish the case in which a VNF must be removed and the case in which only the ports of the VNF must be removed.
		//Currently I'm only considering the case in which the entire VNF must be removed
		if(!stillUsedVNF(*vtbr))
		{
			list<VNFs>::iterator vnf = vnfs.begin();
			for(; vnf != vnfs.end(); vnf++)
			{
				if((*vtbr) == (*vnf))
				{
					vnfs.erase(vnf);
					break;
				}
			}
			assert(vnf != vnfs.end());
		}
		else
		{
			assert(0);
			//TODO: how to manage properly this situation?
		}
	}

	return toberemoved;
}

bool Graph::endpointIsUsedInMatch(string endpoint)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Match match = r->getMatch();
		if(match.matchOnEndPointInternal())
		{
			stringstream ss;
			ss << match.getGraphID() << ":" << match.getEndPointInternal();
			if(ss.str() == endpoint)
				//The endpoint is used in a match
				return true;
		}
	}
	return false;
}

bool Graph::endpointIsUsedInAction(string endpoint)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
	{
		Action *action = r->getAction();
		action_t actionType = action->getType();

		if(actionType == ACTION_ON_ENDPOINT_INTERNAL)
		{
			if(action->toString() == endpoint)
				//The port is used in an action
				return true;
		}
	}
	return false;
}

}
