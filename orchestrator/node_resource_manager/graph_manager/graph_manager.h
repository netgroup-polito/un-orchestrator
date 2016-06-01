#ifndef GRAPH_MANAGER_H_
#define GRAPH_MANAGER_H_ 1

#pragma once

#include "graph_info.h"
#include "lsi.h"
#include "graph_translator.h"
#include "fileParser.h"
#include "../../network_controller/openflow_controller/controller.h"
#ifdef ENABLE_UNIFY_MONITORING_CONTROLLER
	#include "../../monitoring_controller/monitoring_controller.h"
#endif
#include "../../utils/constants.h"
#include "../graph/high_level_graph/high_level_graph.h"
#include "../graph/high_level_graph/high_level_graph_vnf.h"
#include "../graph/low_level_graph/graph.h"
#include "../graph/high_level_graph/high_level_output_action_nf.h"
#include "../graph/high_level_graph/high_level_output_action_port.h"
#include "../graph/high_level_graph/high_level_output_action_endpoint.h"
//#include "../rest_server/match_parser.h"

#ifdef VSWITCH_IMPLEMENTATION_XDPD
	#include "../../network_controller/switch_manager/plugins/xdpd/xdpd_manager.h"
	#define SWITCH_MANAGER_IMPLEMENTATION XDPDManager
#endif
#ifdef VSWITCH_IMPLEMENTATION_OFCONFIG
	#include "../../network_controller/switch_manager/plugins/ovs-ofconfig/ofconfig_manager.h"
	#define SWITCH_MANAGER_IMPLEMENTATION OVSManager
#endif
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
	#include "../../network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_manager.h"
	#define SWITCH_MANAGER_IMPLEMENTATION OVSDBManager
#endif
#ifdef VSWITCH_IMPLEMENTATION_ERFS
	#include "../../network_controller/switch_manager/plugins/erfs/manager.h"
	#define SWITCH_MANAGER_IMPLEMENTATION ERFSManager
#endif
//[+] Add here other implementations for the virtual switch

#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <pthread.h>

#include <stdexcept>

using namespace std;

typedef struct
	{
		string nf_name;
		unsigned int number_of_ports;
		ComputeController *computeController;

		map<unsigned int, string> namesOfPortsOnTheSwitch;
		map<unsigned int, port_network_config_t > portsConfiguration;
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		list<port_mapping_t > controlConfiguration;
		list<string> environmentVariables;
#endif
	}to_thread_t;

class GraphManager
{
private:
	//FIXME: should I put all the attributes static?

	static pthread_mutex_t graph_manager_mutex;

	/**
	*	Port of the openflow controller of the next graph created
	*/
	static uint32_t nextControllerPort;

	/**
	*	This structure contains all the graph end points which are not
	*	ports, but that must be used to connect many graphs together
	*
	*	map<graph ID, counter>
	*	where graph ID is the identifier of the graph defining the end
	*	points, while counter indicates how many times the endpoint is
	*	used by other graphs
	*/
	map<string, unsigned int > availableEndPoints;

	/**
	*	The LSI in common with all the tenants, which
	*	access to the physical interfaces
	*/
	GraphInfo graphInfoLSI0;
	uint64_t dpid0;
	lowlevel::Graph graphLSI0lowLevel; //FIXME: this is a trick for the log
	lowlevel::Graph graph; //FIXME: this is a trick for default rules

	/**
	*	Local IP of the LSI0
	*/
	string un_address;

	/**
	*	Orchestrator can be in band (true) or out of band (false)
	*/
	bool orchestrator_in_band;

	/**
	*	Control interface of the node
	*/
	string un_interface;

	/**
	*	IPsec certificate
	*/
	string ipsec_certificate;

	/**
	*	Map containing the graph identifier of each tenant-LSI, and its desciption
	*/
	map<string,GraphInfo> tenantLSIs;

	/**
	*	The module that interacts with the virtual switch
	*/
	SWITCH_MANAGER_IMPLEMENTATION switchManager;

	/**
	*	@brief: identify the virtual links required to implement the graph: each action
	*		expressed on a NF port, associated with a match on a physical port, requires a
	*		a virtual link; each action on a port associated with a match on a NF requires
	*		a virtual link; each action expressed on an end point associated with a match
	*		on a NF requires a virtual link.
	*		Two actions on the same NF port requires a single virtual link, as well as two
	*		ouput actions realted to the same physical port or end point requrie just one
	*		virtual link.
	*
	*	@param:  graph	The description of the graph to be implemented
	*	@return: the return value is quite complicated. It is a vector whose fields have the
	*		following meaning:
	*			- vector[0]: NFs requiring a virtual link
	*			- vector[1]: physical ports requiring a virtual link
	*			- vector[2]: endpoints requiring a virtual link
	*			- vector[3]: NFs reached from an endpoint that is defined in the current graph
	*/
	vector<set<string> > identifyVirtualLinksRequired(highlevel::Graph *graph);

	/**
	*	@brief: apply the same rules of the previous functions, but the virtual link is required
	*		only if the NF or the port that would need it are not alredy present in the graph.
	*		In fact, this function must be used during the updating of an existing graph, and not
	*		during the creation of a new one
	*
	*	@param: newPiece	The new part of the graph to be created
	*	@param: lsi		Data structure describing the graph to be updated
	*/
	vector<set<string> > identifyVirtualLinksRequired(highlevel::Graph *newPiece, LSI *lsi);

	/**
	*	@brief: given a graph description, check if the ports and the NFs required by the
	*		graph exist
	*
	*	@param: graph				Graph description to be validated
	*	@param: computeController	Compute controller used to validate the graph
	*/
	bool checkGraphValidity(highlevel::Graph *graph, ComputeController *computeController);

	/**
	*	@brief: check if
	*		- a NF no longer requires a vlink in a specific graph
	*		- a physical port no longer requires a vlink in a specific graph
	*		- a graph endpoint no longer requires a vlink in a specific graph
	*			(it is not necessary that the endpoint is defined by the graph itself)
	*		- NFs are no longer used
	*		- physical ports are no longer used
	*		- endpoints are no longer used
	*	and then remove the useles things from the LSI
	*/
	void removeUselessPorts_NFs_Endpoints_VirtualLinks(RuleRemovedInfo tbr, ComputeController *computeController,highlevel::Graph *graph, LSI * lsi);

	/**
	*	@brief: given a NF of the graph (in the form NF_port), return the endpoint expressed in the match of a rule
	*		whose action is expressed on the function.
	*
	*	@param: graph	Graph in which the information must be seatched
	*	@param: nf		Involved NF
	*/
	string findEndPointTowardsNF(highlevel::Graph *graph, string nf);

	/**
	*	@brief: check if a specific flow can be removed from a graph. The flow cannot be removed if it defines
	*		an endpoint currently used by other graphs.
	*
	*	@param: graph	Graph containing the flow to be removed
	*	@param: flowID	Identifier of the flow to be removed
	*
	*	FIXME: this function could be improved as follows: if the flow defines an endpoint in a match, it could
	*	be removed if it is not used in actions of other graphs.
	*/
	bool canDeleteFlow(highlevel::Graph *graph, string flowID);

public:
	//XXX: Currently I only support rules with a match expressed on a port or on a NF
	//(plus other fields)

	GraphManager(int core_mask,string portsFileName,string un_address,bool control,string un_interface,string ipsec_certificate);
	~GraphManager();

	/**
	*	@brief: check if a certain graph exists
	*/
	bool graphExists(string graphID);

	/**
	*	@brief: check if a flow exists in a graph
	*/
	bool flowExists(string graphID, string flowID);

	/**
	*	@brief: given a graph description, implement the graph
	*/
	bool newGraph(highlevel::Graph *graph);

	/**
	*	@brief: remove the graph with a specified graph descriptor. The graph cannot be
	*		removed if it defines endpoints currently used by other graphs and
	*		shutdown is false.
	*
	*	When the graph is removed, the endpoints it defines are removed as well, and the
	*	counter for the endpoints it uses are decreased.
	*/
	bool deleteGraph(string graphID, bool shutdown = false);

	/**
	*	@brief: add a new piece to an existing graph with
	*		a specific ID.
	*
	*	XXX: note that an existing NF does not change: if a new port is required, the update
	*	of the graph fails
	*/
	bool updateGraph(string graphID, highlevel::Graph *newFlow);

	/**
	*	@brief: remove the flow with a specified ID, from a specified graph
	*
	*	This method is quite complex, and it works as follows
	*		* rule: port -> NF:port
	*			This means that there is a vlink associated with NF:port. In case
	*			NF:port does not appear in other actions, then the vlink is removed
	*		* rule: NF:port -> port
	*			This means that there is vlink associated with port. In case port
	*			does not appear in other actions, then the vlink is removed
	*		* rule: NF:port -> port
	*				port -> NF:port
	*				NF1:port -> NF2:port
	*			If NF (NF1, NF2) does not appear in any other rule, the NF is stopped,
	*			and its ports are destroyed
	*	In both the LSI-0 and tenant-LSI, a flowmod to remove a flow is sent just in case
	*	the related (low level) graph does not have other identical rules (the lowering
	*	of the graph may generate several identical rules in the same low level graph).
	*	In any case, the rule is removed from the highlevel graph, from the lowlevel graph
	*	of the tenant-LSI, and from the lowlevel graph of the LSI-0.
	*
	*	XXX: note that an existing NF does not change: if a port of that NF is no longer used,
	*	it does not matter. The port is neither destroyed, nor removed from the virtual switch
	*
	*	TODO: describe what happens in case of endpoint
	*/
	bool deleteFlow(string graphID, string flowID);

#if 0
	/**
	*	@brief: deletes a NF from the graph
	*
	*	@param: graphID	Identifier of the graph to which the NF belongs to
	*	@param: nf_name	Name of the NF to be removed from the graph
	*/
	bool stopNetworkFunction(string graphID, string nf_name);
#endif

	/**
	*	@brief: checks if a specific NF is part of a specific graph
	*/
	bool graphContainsNF(string graphID,string nf);

	/**
	*	@brief: create the JSON representation of the graph with the given ID
	*/
	Object toJSON(string graphID);

	/**
	*	@brief: create the JSON representation of the physical interfaces that can be connected
	*		to the graphs (both ethernet and wifi)
	*/
	Object toJSONPhysicalInterfaces();

	/**
	*	@brief: prints information on the graphs deployed
	*/
	void printInfo(bool complete = true);

	void printInfo(lowlevel::Graph graphLSI0, LSI *lsi0);

	static void mutexInit();
};


class GraphManagerException: public exception
{
public:
	virtual const char* what() const throw()
	{
		return "GraphManagerException";
	}
};

#endif //GRAPH_MANAGER_H_
