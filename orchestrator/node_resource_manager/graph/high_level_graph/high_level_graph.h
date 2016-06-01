#ifndef HIGH_LEVEL_GRAPH_H_
#define HIGH_LEVEL_GRAPH_H_ 1

#pragma once

#include <list>
#include <set>
#include <string>
#include <sstream>

#include "high_level_graph_endpoint_interface.h"
#include "high_level_graph_endpoint_gre.h"
#include "high_level_graph_endpoint_vlan.h"
#include "high_level_graph_vnf.h"
#include "high_level_rule.h"
#include "high_level_output_action_nf.h"
#include "high_level_output_action_port.h"
#include "../../graph_manager/rule_removed_info.h"
#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include "nf_port_configuration.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;
using namespace std;

namespace highlevel
{

/**
*	@brief: describes the graph as it has been required from the extern, through
*		the REST API exported by the un-orchestrator
*/
class Graph
{
public:
	typedef map<string, list<unsigned int> > t_nfs_ports_list;
	typedef map<string, map<unsigned int, port_network_config_t > > t_nfs_configuration;

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	typedef map<string, list<port_mapping_t > > t_nfs_control;
	typedef map<string, list<string> > t_nfs_env_variables;
#endif

private:
	//FIXME: this class contains a lot of information already specified in the class VNFs.

	/**
	*	@brief: for each NF attached to the graph specifies a list of ports. For
	*		instance, if in the graph there is NF:1 and NF:2,
	*		an element of the map is <NF, <1,2> >
	*/
	t_nfs_ports_list networkFunctions;

	/**
	*	@brief: for each NF attached to the graph specifies a list of pair elements
	* 		(mac address, ip address), one for each port
	*/
	t_nfs_configuration networkFunctionsConfiguration;

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	/**
	*	@brief: for each VNF attached to the graph, specifies a list of pair elements
	* 		(host TCP port, VNF TCP port), which are the control connections for the VNF
	*/
	t_nfs_control networkFunctionsControlPorts;

	/**
	*	@brief: for each VNF attached to the graph, specifies a list of environment variables
	*		to be set to the VNF itself. Each element of the list is in the form "variable=value"
	*
	*/
	t_nfs_env_variables networkFunctionsEnvironmentVariables;
#endif

#ifdef ENABLE_UNIFY_MONITORING_CONTROLLER
	string measureString;
#endif

	/**
	*	@brief: for each endpoint attached to the graph specifies a list of params
	* 		(gre key, local ip, remote ip)
	*/
	map<string, vector<string> > endpoints;

	/**
	*	@brief: physical ports to be attached to the graph
	*/
	set<string> ports;

	/**
	*	@brief: List of rules describing the graph
	*/
	list<Rule> rules;

	/**
	*	@brief: List of endPointsInterface describing the graph
	*/
	list<EndPointInterface> endPointsInterface;

	/**
	*	@brief: List of endPointsGRE describing the graph
	*/
	list<EndPointGre> endPointsGre;

	/**
	*	@brief: List of endPointsVlan describing the graph
	*/
	list<EndPointVlan> endPointsVlan;

	/**
	*	@brief: List of VNFs describing the graph
	*/
	list<VNFs> vnfs;

	/**
	*	@brief: Identifier of the graph
	*/
	string ID;

	/**
	*	@brief: Name of the graph
	*/
	string name;

public:

	/**
	*	@brief: construction
	*
	*	@param: ID	identifier of the graph
	*/
	Graph(string ID);

	/**
	*	@brief: Add a physical port to the graph
	*
	*	@param: port	Name of the physical port to be added
	*/
	bool addPort(string port);

	/**
	*	@brief: Return the physical ports of the graph
	*/
	set<string> getPorts();

	/**
	*	@brief: Add a NF to the graph
	*
	*	@param:	nf	Name of the network function to be added
	*/
	bool addNetworkFunction(string nf);

	/**
	*	@brief: Add a NF port description to the graph
	*
	*	@param:	nf	Name of the network function to be added
	*	@param: description a pair of value <mac address, ip address>
	*/
	//FIXME: is the return value useful?
	bool addNetworkFunctionPortConfiguration(string nf, map<unsigned int, port_network_config_t > description);

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	/**
	*	@brief: Add a VNF control port to the graph
	*
	*	@param:	nf	Name of the network function to be added
	*	@param: control_port. It is a pair of value <vnf_tcp_port, host_tcp_port>
	*/
	void addNetworkFunctionControlPort(string nf, port_mapping_t control_port);

	/**
	*	@brief: Add an environment variable for a specific VNF
	*
	*	@param:	nf	Name of the network function to be added
	*	@param: env_vairiable. Environment variable in the for "variable=value"
	*/
	void addNetworkFunctionEnvironmentVariable(string nf, string env_variable);
#endif

#ifdef ENABLE_UNIFY_MONITORING_CONTROLLER
	/**
	*	@brief: Set to the graph a string written in the MEASURE language
	*
	*	@param: measureString	String written according to the MEASURE language
	*/
	void setMeasureString(string measureString);
#endif

	/**
	*	@brief: Update a NF by adding a port
	*	//FIXME: is this useful?
	*
	*	@param: nf		Name of the network function to be updated
	*	@param: port	Identifier of the port of the network function
	*/
	bool updateNetworkFunction(string nf, unsigned port);

	/**
	*	@brief: Return the NFs of the graph and the ports they require
	*/
	t_nfs_ports_list getNetworkFunctions();

	/**
	*	@brief: Return the VNFs of the graph and the description of the ports they require
	*/
	t_nfs_configuration getNetworkFunctionsConfiguration();

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	/**
	*	@brief: Return the VNFs of the graph and the control ports they require
	*/
	t_nfs_control getNetworkFunctionsControlPorts();

	/**
	*	@brief: Return the VNFs of the graph and the environment variables they require
	*/
	t_nfs_env_variables getNetworkFunctionsEnvironmentVariables();
#endif

#ifdef ENABLE_UNIFY_MONITORING_CONTROLLER
	/**
	*	@brief: Return a string written according to the MEASURE language
	*/
	string getMeasureString();
#endif

	/**
	*	@brief: Add an end point to the graph, to be used to connect the graph itself with
	*		other graphs.
	*
	*	@param: ep			Name of the endpoint to de added
	*	@param: p			List of five elements (key, local ip, remote ip, interface and safe)
	*
	*	@return: true
	*/
	bool addEndPoint(string ep, vector<string> p);

	/**
	*	@brief: Return the end points of the graph, i.e. the ports to be used to connect
	*		multiple graphs together.
	*/
	map<string, vector<string> > getEndPoints();

	/**
	*	@brief: Return the ID of the graph
	*/
	string getID();

	/**
	*	@brief: Set the name of the graph
	*/
	void setName(string name);

	/**
	*	@brief: Return the name of the graph
	*/
	string getName();

	/**
	*	@brief: Add a new endpointInterface to the graph
	*/
	bool addEndPointInterface(EndPointInterface endpoint);

	/**
	*	@brief: Return the endpointsInterface of the graph
	*/
	//XXX: this functions is not used. 
	list<EndPointInterface> getEndPointsInterface();

	/**
	*	@brief: Add a new endpointGre to the graph
	*/
	bool addEndPointGre(EndPointGre endpoint);

	/**
	*	@brief: Return the endpointsGre of the graph
	*/
	list<EndPointGre> getEndPointsGre();

	/**
	*	@brief: Add a new endpointVlan to the graph
	*/
	bool addEndPointVlan(EndPointVlan endpoint);

	/**
	*	@brief: Return the endpointsVlan of the graph
	*/
	list<EndPointVlan> getEndPointsVlan();

	/**
	*	@brief: Add a new vnf to the graph
	*/
	bool addVNF(VNFs vnf);

	/**
	*	@brief: Return the vnfs of the graph
	*/
	list<VNFs> getVNFs();

	/**
	*	@brief: Return the rules of the graph
	*/
	list<Rule> getRules();

	/**
	*	@brief: Return the rule with a specific ID
	*
	*	@param: ID	Identifier of the rule to be returned
	*/
	Rule getRuleFromID(string ID);

	/**
	*	@brief: Add a new rule to the graph. THe rule is not added if it has an ID already
	*		existing into the graph
	*
	*	@param:	rule	Rule to be added
	*/
	bool addRule(Rule rule);

	/**
	*	@brief: Check if the graph contains a specific flowID
	*
	*	@param:	ID	Identifier of a graph
	*/
	bool ruleExists(string ID);

	/**
	*	@brief: Remove a rule starting from its ID, and return
	*		information on the removed rule
	*
	*	@param:	ID	Identifier of the graph to be removed
	*/
	RuleRemovedInfo removeRuleFromID(string ID);

	/**
	*	@brief: Checks if a NF still exist in the graph. If it is no longer used
	*		in any rule, but it is still part of the "networkFunctions" map,
	*		it is removed from this map as well.
	*
	*	@param:	nf	Name of a network function
	*/
	bool stillExistNF(string nf);

	/**
	*	@brief: Checks if a physical port still exist in the graph. If it is no
	*		longer used in any rule, but it is part of the "ports" set, it
	*		is removed from this set as well.
	*
	*	@param:	port	Name of a physical port
	*/
	bool stillExistPort(string port);

	/**
	*	@brief: Checks if an endpoint port still exist in the graph. If it is no
	*		longer used in any rule, but it is part of the "endpoints" set, it
	*		is removed from this set as well.
	*
	*	@param: endpoint	Name of an endpoint
	*/
	bool stillExistEndpoint(string endpoint);

	/**
	*	@brief: check if a specific flow uses an endpoint (it does not matter if the endpoint is defined in the
	*		current graph or not); in this case, return that endpoint, otherwise return "".
	*
	*	@param: flowID	Identifier of the flow that could contain an endpoint
	*/
	string getEndpointInvolved(string flowID);

	/**
	*	@brief: Return the number of flows in the graph
	*/
	int getNumberOfRules();

	/**
	*	@brief: Create a JSON representation of the graph
	*/
	Object toJSON();

	void print();
};

}

#endif //HIGH_LEVEL_GRAPH_H_
