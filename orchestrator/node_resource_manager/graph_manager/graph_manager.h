#ifndef GRAPH_MANAGER_H_
#define GRAPH_MANAGER_H_ 1

#pragma once

#include "graph_info.h"
#include "lsi.h"
#include "graph_translator.h"
#include "../../network_controller/openflow_controller/controller.h"
#include "../../utils/constants.h"
#include "../graph/high_level_graph/high_level_graph.h"
#include "../graph/low_level_graph/graph.h"
#include "../graph/high_level_graph/high_level_graph_endpoint_interface.h"
#include "../graph/high_level_graph/high_level_graph_vnf.h"
#include "../interface_manager/InterfaceManager.h"

#ifdef VSWITCH_IMPLEMENTATION_XDPD
	#include "../../network_controller/switch_manager/plugins/xdpd/xdpd_manager.h"
	#define SWITCH_MANAGER_IMPLEMENTATION XDPDManager
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
		string nf_id;
		unsigned int number_of_ports;
		ComputeController *computeController;

		map<unsigned int, string> namesOfPortsOnTheSwitch;
		map<unsigned int, port_network_config_t > portsConfiguration;
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		list<port_mapping_t > controlConfiguration;
		list<string> environmentVariables;
#endif
#ifdef ENABLE_KVM
		string user_data;
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
	*	handles L3 ports of the graphs
	*/
	InterfaceManager interfaceManager;

	/**
	*	This structure contains information of the number of graphs that use
	*	an internal endpoint. The first element is the internal group of the
	*	internal endpoint, the counter is the number of times it is used
	*/
	map<string, unsigned int > timesUsedEndPointsInternal;

	/**
	*	This structure says if the LSI represanting an internal endpoint has alrady
	*	been created or not.
	*/
	map<string, bool > internalLSIsCreated;

	/**
	*	Map containing the graph identifier of each internal LSI (i.e., an LSI
	*	that corresponds to and internal endpoint) and its desciption
	*/
	map<string, GraphInfo> internalLSIs;
	
	/**
	*	For each internal endpoint identifier, maps the identifier of a graph using such
	*	an endpoint and the port ID on the LSI-0
	*/
	map<string, map <string, unsigned int> > internalLSIsConnections;

	/**
	*	The LSI-0 is in common with all the tenants, and it is the only one that
	*	accesses to the physical interfaces
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
	*	Netmask of the local IP of the LSI0
	*/
	string un_netmask;

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
	*	Vnf repository configuration parameters
	**/
	string vnfRepoIP;
	int vnfRepoPort;
    string vnfImagePath;

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
	*	@brief: do the operation required to set up an in band control. This requires to set up some rules in the LSI-0.
	*
	*	@param: lsi			contains information related to the LSI-0
	*	@param: controller	Openflow controller used to configure the LSI-0
	*/
	void handleInBandController(LSI *lsi, Controller *controller);
	
	/**
	*	@brief: create the Openflow controller for the LSI representing a specific internal endpoint, in case 
	*			such a controller does not exist yet
	*
	*	@param: graph	graph potentially containing internal endpoints to be handled
	*/
	void handleControllerForInternalEndpoint(highlevel::Graph *graph);

	/**
	*	@brief: for each internal endpoint required by the graph, this function:
	*			- create an new LSI representing such an endpoint, if the LSI does not exist yet
	*			- update the LSI representing such an endpoint, if the LSI already exists
	*/
	void handleGraphForInternalEndpoint(highlevel::Graph *graph);

	/**
	*	@brief: add a new piece to an existing graph with
	*		a specific ID, except the new flowrules.
	*/
	highlevel::Graph *updateGraph_add(string graphID, highlevel::Graph *newGraph);

	/**
	*	@brief: add the new rules to the graph. The rules are currently stored in the graph "diff",
	*			and must be stored in the graph identified by "graphID"
	*/
	bool updateGraph_add_rules(string graphID, highlevel::Graph *diff);

	/**
	*	@brief: remove pieces from an existing graph with a
	*		specific ID.
	*/
	highlevel::Graph *updateGraph_remove(string graphID, highlevel::Graph *newGraph);

	/**
	*	@brief: given a flow ID returns a list of corresponding
	*	rules on LSI0.
	*	Note that the size of the list can be more than 1 due to
	*	the possibility to specify more than one output_to_port in action
	*	(that can involves the splitting of a rule)
	*/
	list<string> getRulesIDForLSI0(highlevel::Rule rule, string graphID);

	/**
	*	@brief: given a flow ID returns a list of corresponding
	*	rules on LSI Tenant.
	*	Note that the size of the list can be more than 1 due to
	*	the possibility to specify more than one output_to_port in action
	*	(that can involves the splitting of a rule)
	*/
	list<string> getRulesIDForLSITenant(highlevel::Rule rule);

public:
	//XXX: Currently I only support rules with a match expressed on a port or on a NF
	//(plus other fields)

	GraphManager(int core_mask,set<string> physical_ports,string un_address,bool control,string un_interface,string ipsec_certificate, string vnf_repo_ip, int vnf_repo_port,string vnf_images_path);
	~GraphManager();

	/**
	*	@brief: check if a certain graph exists
	*/
	bool graphExists(string graphID);

	/**
	*	@brief: given a graph description, implement the graph
	*/
	bool newGraph(highlevel::Graph *graph);

	/**
	*	@brief: remove the graph with a specified graph descriptor.
	*
	*	When the graph is removed, in case it uses internal endpoints, such 
	*	endpoints are removed from the proper internal LSIs.
	*/
	bool deleteGraph(string graphID);

	/**
	*	@brief: update an existing graph
	*/
	bool updateGraph(string graphID, highlevel::Graph *newGraph);

	/**
	*	@brief: create the JSON representation of the graph with the given ID
	*/
	Object toJSON(string graphID);

	/**
	*	@brief: prints information on the graphs deployed
	*/
	void printInfo(bool complete = true);

	void printInfo(lowlevel::Graph graphLSI0, LSI *lsi0);

	static void mutexInit();

	//According to the rules removed from the graph, this function deletes the virtual links that are no
	//longer used by the flows of the graph.
	void removeUselessVlinks(RuleRemovedInfo rri, highlevel::Graph *graph, LSI *lsi);

	void getGraphsNames(std::list<std::string> *l);
	
	//Returns the ip address + port of the vnf repository
	string getVnfRepoEndpoint();
};


class GraphManagerException: public std::exception
{
public:
	virtual const char* what() const throw()
	{
		return "GraphManagerException";
	}
};

#endif //GRAPH_MANAGER_H_
