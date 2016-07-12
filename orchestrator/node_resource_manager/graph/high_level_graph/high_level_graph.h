#ifndef HIGH_LEVEL_GRAPH_H_
#define HIGH_LEVEL_GRAPH_H_ 1

#pragma once

#include <list>
#include <set>
#include <string>
#include <sstream>

#include "high_level_graph_endpoint_interface.h"
#include "high_level_graph_endpoint_internal.h"
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
private:

	//FIXME: what is this?
	/**
	*	@brief: for each end point of the graph, the following structure specifies if that end
	*		point is defined in this graph or not
	*/
	map<string, bool > endpoints;

	/**
	*	@brief: List of rules describing the graph
	*/
	list<Rule> rules;

	/**
	*	@brief: List of "interface" endpoints
	*/
	list<EndPointInterface> endPointsInterface;

	/**
	*	@brief: List of "internal" endpoints
	*/
	list<EndPointInternal> endPointsInternal;

	/**
	*	@brief: List of "GRE" endpoints
	*/
	list<EndPointGre> endPointsGre;

	/**
	*	@brief: List of "vlan" endpoints
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
	
	/**
	*	@brief: Given a graph, returns the rules present in such a graph with respect to the rules
	*			of the graph on which the method is called
	*
	*	@param: other	Graph from which the new rules must be extracted
	*/
	list<Rule> calculateDiffRules(Graph *other);

	/**
	*	@brief: Check if an endpoint interface is still used in the graph.
	*/
	bool stillUsedEndpointInterface(EndPointInterface endpoint);
	
		/**
	*	@brief: Check if a VNF is still used in the graph.
	*/
	bool stillUsedVNF(VNFs vnf);

public:

	/**
	*	@brief: construction
	*
	*	@param: ID	identifier of the graph
	*/
	Graph(string ID);

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
	*	Function that does operations on graphs (diff, sum, subtraction)
	*/

	/**
	*	@brief: Given a graph, calculate the things (e.g., NFs) that are in such a graph and not in the
	*		graph on which the method is called. In practice, it returns "other - this"
	*/
	highlevel::Graph *calculateDiff(highlevel::Graph *other, string graphID);

	/**
	*	@brief: Given a graph, add its components to the called graph
	*
	*	@param: other	graph with the components to be added
	*/
	bool addGraphToGraph(highlevel::Graph *other);

	/**
	*	@brief: Given a graph, rome its components from the called graph
	*
	*	@param: other	graph with the components to be removed
	*/
	list<RuleRemovedInfo> removeGraphFromGraph(highlevel::Graph *other);

	/**
	*	Functions to manage the "interface" endpoints
	*/

	/**
	*	@brief: Add a new "interface" endpoint to the graph
	*/
	bool addEndPointInterface(EndPointInterface endpoint);

	/**
	*	@brief: Return the "interface" endpoints of the graph
	*/
	list<EndPointInterface> getEndPointsInterface();

	/**
	*	@brief: Remove an endpoint "interface" from the graph
	*/
	void removeEndPointInterface(EndPointInterface endpoint);

	/**
	*	Functions to manage the "internal" endpoints
	*/

	/**
	*	@brief: Add a new "internal" endpoint to the graph
	*/
	bool addEndPointInternal(EndPointInternal endpoint);

	/**
	*	@brief: Return the "internal" endpoints of the graph
	*/
	list<EndPointInternal> getEndPointsInternal();

	/**
	*	@brief: Remove an endpoint "internal" from the graph
	*/
	void removeEndPointInternal(EndPointInternal endpoint);

	/**
	*	Functions to manage the "gre-tunnel" endpoints
	*/

	/**
	*	@brief: Add a new "gre-tunnel" endpoint to the graph
	*/
	bool addEndPointGre(EndPointGre endpoint);

	/**
	*	@brief: Return the "gre-tunnel" endpoints of the graph
	*/
	list<EndPointGre> getEndPointsGre();

	/**
	*	@brief: Remove an endpoint "gre-tunnel" from the graph
	*/
	void removeEndPointGre(EndPointGre endpoint);

	/**
	*	Functions to manage the "vlan" endpoints
	*/

	/**
	*	@brief: Add a new "vlan" endpoint to the graph
	*/
	bool addEndPointVlan(EndPointVlan endpoint);

	/**
	*	@brief: Return the "vlan" endpoints of the graph
	*/
	list<EndPointVlan> getEndPointsVlan();
	
	/**
	*	@brief: Remove an endpoint "internal" from the graph
	*/
	void removeEndPointVlan(EndPointVlan endpoint);

	/**
	*	Functions to manage the VNFs
	*/

	/**
	*	@brief: Add a new vnf to the graph. In case the VNF is already part of the
	*			graph but new ports have been specified, the new ports are added to
	*			the graph.
	*/
	void addVNF(VNFs vnf);

	/**
	*	@brief: Return the VNFs of the graph
	*/
	list<VNFs> getVNFs();

	/**
	*	Functions to manage the rules
	*/

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
	*	@param:	ID	Identifier of the rule to be removed
	*/
	RuleRemovedInfo removeRuleFromID(string ID);

	/**
	*	Functions to get the description of the graph
	*/

	/**
	*	@brief: Return the number of rules in the graph
	*/
	int getNumberOfRules();

	/**
	*	@brief: Create a JSON representation of the graph
	*/
	Object toJSON();

	void print();

/********************************************************************************************************************/

	/**
	*	@brief: Checks if an endpoint port still exist in the graph. If it is no
	*		longer used in any rule, but it is part of the "endpoints" set, it
	*		is removed from this set as well.
	*
	*	@param: endpoint	Name of an endpoint
	*/
	bool stillExistEndpoint(string endpoint);

	/**
	*	@brief: Checks if an endpoint GRE still exist in the graph. If it is no
	*		longer used in any rule, but it is part of the "endpoints" set, it
	*		is removed from this set as well.
	*
	*	@param: endpoint	Name of an endpoint
	*/
	bool stillExistEndpointGre(string endpoint);

	/**
	*	@brief: check if a specific flow uses an endpoint (it does not matter if the endpoint is defined in the
	*		current graph or not); in this case, return that endpoint, otherwise return "".
	*
	*	@param: flowID	Identifier of the flow that could contain an endpoint
	*/
	string getEndpointInvolved(string flowID);

	/**
	*	@brief: check if an endpoint is used in some action of the graph
	*
	*	@param: endpoint	Idenfier of the endpoint to be checked
	*/
	bool endpointIsUsedInAction(string endpoint);

	/**
	*	@brief: check if an endpoint is used in some match of the graph
	*
	*	@param: endpoint	Idenfier of the endpoint to be checked
	*/
	bool endpointIsUsedInMatch(string endpoint);
};

}

#endif //HIGH_LEVEL_GRAPH_H_
