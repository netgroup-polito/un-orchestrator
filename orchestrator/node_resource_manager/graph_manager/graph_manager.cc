#include "graph_manager.h"

static const char LOG_MODULE_NAME[] = "Graph-Manager";

pthread_mutex_t GraphManager::graph_manager_mutex;
uint32_t GraphManager::nextControllerPort = FIRTS_OF_CONTROLLER_PORT;

void GraphManager::mutexInit()
{
	pthread_mutex_init(&graph_manager_mutex, NULL);
}

GraphManager::GraphManager(int core_mask) : switchManager()
{
	//TODO: this code can be simplified. Why don't providing the set<string> to the switch manager?
	list<string> phisicalPorts = Configuration::instance()->getPhisicalPorts();
	set<CheckPhysicalPortsIn> phyPortsRequired;
	for(list<string>::iterator pp = phisicalPorts.begin(); pp != phisicalPorts.end(); pp++)
	{
		CheckPhysicalPortsIn cppi(*pp);
		phyPortsRequired.insert(cppi);
	}

	set<string> phyPorts;//maps the name into the side
	for(set<CheckPhysicalPortsIn>::iterator pp = phyPortsRequired.begin(); pp != phyPortsRequired.end(); pp++)
		phyPorts.insert(pp->getPortName());

	//Create the openflow controller for the LSI-0

	pthread_mutex_lock(&graph_manager_mutex);
	uint32_t controllerPort = nextControllerPort;
	nextControllerPort++;
	pthread_mutex_unlock(&graph_manager_mutex);

	ULOG_INFO("Checking the available physical interfaces...");
	try
	{
		//Check the available physical ports
		switchManager.checkPhysicalInterfaces(phyPortsRequired);
	} catch (...)
	{
		throw GraphManagerException();
	}

	ULOG_INFO("\t%d physical interfaces under the control of the un-orchestrator:",phyPorts.size());
	for(set<string>::iterator p = phyPorts.begin(); p != phyPorts.end(); p++)
		ULOG_INFO("\t\t%s",(*p).c_str());

	//Create the openflow controller for the lsi-0
	ULOG_INFO("Creating the openflow controller for LSI-0...");

	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	switch(OFP_VERSION)
	{
		case OFP_10:
			ULOG_DBG_INFO("\tUsing Openflow 1.0");
			versionbitmap.add_ofp_version(rofl::openflow10::OFP_VERSION);
			break;
		case OFP_12:
			ULOG_DBG_INFO("\tUsing Openflow 1.2");
			versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
			break;
		case OFP_13:
			ULOG_DBG_INFO("\tUsing Openflow 1.3");
			versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
			break;
	}

	Controller *controller = new Controller(versionbitmap,graph, controllerPort);
	controller->start();

	ULOG_INFO("Creating the LSI-0...");

	//The following structures are empty. No network function, virtual link gre-tunnel endpoint is attached.
	list<highlevel::VNFs> dummy_network_functions;
	map<string, map<unsigned int, PortTechnology> > dummy_nfs_ports_type;
	list<highlevel::EndPointGre> dummy_gre_endpoints;
	list<highlevel::EndPointHostStack> dummy_hoststack_endpoints;
	vector<VLink> dummy_virtual_links;

	LSI *lsi = new LSI(string(OF_CONTROLLER_ADDRESS), controllerPort, phyPorts, dummy_network_functions,
	dummy_gre_endpoints,dummy_virtual_links,dummy_nfs_ports_type, dummy_hoststack_endpoints);

	try
	{
		//Create a new LSI, which is the LSI-0 of the node
		assert(lsi->getGreEndpointsPorts().size() == 0);
		map<string,nf_t>  nf_types;
		map<string,list<nf_port_info> > netFunctionsPortsInfo;
		CreateLsiIn cli(string(OF_CONTROLLER_ADDRESS),controllerPort,lsi->getPhysicalPortsName(),lsi->getHostackEndpointID(), nf_types,netFunctionsPortsInfo,lsi->getGreEndpointsDescription(),lsi->getVirtualLinksRemoteLSI(), Configuration::instance()->getUnAddress(), Configuration::instance()->getIpsecCertificate());

		CreateLsiOut *clo = switchManager.createLsi(cli);

		lsi->setDpid(clo->getDpid());
		map<string,unsigned int> physicalPorts = clo->getPhysicalPorts();
		//TODO check that the physical ports returned are the same provided to the switch manager
		for(map<string,unsigned int>::iterator it = physicalPorts.begin(); it != physicalPorts.end(); it++)
		{
			if(!lsi->setPhysicalPortID(it->first,it->second))
			{
				ULOG_ERR("An unknown physical interface \"%s\" has been attached to the lsi-0",it->first.c_str());
				delete(clo);
				throw GraphManagerException();
			}
		}

		map<string,map<string, unsigned int> > nfsports = clo->getNetworkFunctionsPorts();
		if(!nfsports.empty())
		{
			ULOG_ERR("Non required NFs ports have been attached to the lsi-0");
			delete(clo);
			throw GraphManagerException();
		}

		map<string, unsigned int> epsports = clo->getGreEndpointsPorts();
		if(!epsports.empty())
		{
			ULOG_ERR("Non required endpoints ports have been attached to the lsi-0");
			delete(clo);
			throw GraphManagerException();
		}

		list<pair<unsigned int, unsigned int> > vl = clo->getVirtualLinks();
		if(!vl.empty())
		{
			ULOG_WARN("Non required connections have been created between the lsi-0 and other(?) lsis");
			delete(clo);
			throw GraphManagerException();
		}

		delete(clo);
	} catch (SwitchManagerException e)
	{
		ULOG_ERR("%s",e.what());
		throw GraphManagerException();
	}

	dpid0 = lsi->getDpid();
	map<string,unsigned int> lsi_ports = lsi->getPhysicalPorts();

	ULOG_DBG_INFO("LSI ID: %d",dpid0);
	ULOG_DBG_INFO("Physical ports:",lsi_ports.size());
	for(map<string,unsigned int>::iterator p = lsi_ports.begin(); p != lsi_ports.end(); p++)
		ULOG_DBG_INFO("\t%s -> %d",(p->first).c_str(),p->second);

	graphInfoLSI0.setLSI(lsi);
	graphInfoLSI0.setController(controller);

	ULOG_INFO("LSI-0 and its controller are created");

	ComputeController::setCoreMask(core_mask);
#if 0
	//if control is in band install the default rules on LSI-0 otherwise skip this code
	if(Configuration::instance()->getOrchestratorInBand() && !Configuration::instance()->getUnInterface().empty() && !Configuration::instance()->getUnAddress().empty())
		handleInBandController(lsi,controller);
#endif
}

#if 0
void GraphManager::handleInBandController(LSI *lsi, Controller *controller)
{
	ULOG_DBG_INFO("Handling in band controller");

	unsigned int i = 0;

	//remove first " character
	un_interface.erase(0,1);
	//remove last " character
	un_interface.erase(un_interface.size()-1,1);

	//Install the default rules on LSI-0
	map<string,unsigned int> lsi_ports = lsi->getPhysicalPorts();
	lowlevel::Match lsi0Match, lsi0Match0, lsi0Match1, lsi0Match2;
	if(lsi_ports.count((char *)un_interface.c_str()) == 0)
	{
		ULOG_ERR("Control interface does not exist in a list of available plysical ports.");
		throw GraphManagerException();
	}

	map<string,unsigned int>::iterator translation = lsi_ports.find((char *)un_interface.c_str());

	lsi0Match0.setArpTpa((char *)un_address.c_str());
	lsi0Match0.setEthType(2054 & 0xFFFF);
	lsi0Match0.setInputPort(translation->second);

	lsi0Match1.setIpv4Dst((char *)un_address.c_str());
	lsi0Match1.setEthType(2048 & 0xFFFF);
	lsi0Match1.setInputPort(translation->second);

	lowlevel::Action lsi0Action(lowlevel::ACTION_LOCAL);

	//Create the rule and add it to the graph
	//The rule ID is created as follows DEFAULT-GRAPH_ID
	stringstream newRuleID;
	newRuleID << IN_BAND_GRAPH << "_" << i;
	lowlevel::Rule lsi0Rule0(lsi0Match0,lsi0Action,newRuleID.str(),HIGH_PRIORITY);
	graphLSI0lowLevel.addRule(lsi0Rule0);

	i++;

	//Create the rule and add it to the graph
	//The rule ID is created as follows DEFAULT-GRAPH_ID
	newRuleID.str("");
	newRuleID << IN_BAND_GRAPH << "_" << i;
	lowlevel::Rule lsi0Rule1(lsi0Match1,lsi0Action,newRuleID.str(),HIGH_PRIORITY);
	graphLSI0lowLevel.addRule(lsi0Rule1);

	i++;

	lowlevel::Match lsi0Match3(lowlevel::MATCH_LOCAL);

	lowlevel::Action lsi0Action1;
	lsi0Action1.addOutputPort(translation->second);

	//Create the rule and add it to the graph
	//The rule ID is created as follows DEFAULT-GRAPH_ID
	newRuleID.str("");
	newRuleID << IN_BAND_GRAPH << "_" << i;
	lowlevel::Rule lsi0Rule3(lsi0Match3,lsi0Action1,newRuleID.str(),HIGH_PRIORITY);
	graphLSI0lowLevel.addRule(lsi0Rule3);

	graphLSI0lowLevel.print();

	//Insert new rules into the LSI-0
	ULOG_DBG_INFO("Adding the new rules to the LSI-0");
	controller->installNewRules(graphLSI0lowLevel.getRules());

	printInfo(graphLSI0lowLevel,graphInfoLSI0.getLSI());
}
#endif
GraphManager::~GraphManager()
{
	//Deleting tenants LSIs
	for(map<string,GraphInfo>::iterator lsi = tenantLSIs.begin(); lsi != tenantLSIs.end();)
	{
		map<string,GraphInfo>::iterator tmp = lsi;
		lsi++;
		try
		{
			deleteGraph(tmp->first);
		}catch(...)
		{
			assert(0);
			/*nothing to do, since the node orchestrator is terminating*/
		}
	}

	//Deleting LSI-0
	ULOG_INFO("Deleting the graph for the LSI-0...");
	LSI *lsi0 = graphInfoLSI0.getLSI();

	try
	{
		switchManager.destroyLsi(lsi0->getDpid());
	} catch (SwitchManagerException e)
	{
		ULOG_WARN("%s",e.what());
		//we don't throw any exception here, since the graph manager is terminating
	}

	Controller *controller = graphInfoLSI0.getController();
	delete(controller);
	controller = NULL;
}

bool GraphManager::graphExists(string graphID)
{
	if(tenantLSIs.count(graphID) == 0)
		return false;

	return true;
}

void GraphManager::getGraphsNames(std::list<std::string> *l)
{
	for (std::map<string,GraphInfo>::iterator it=tenantLSIs.begin(); it!=tenantLSIs.end(); ++it)
	l->push_back(it->first);
}

Object GraphManager::toJSON(string graphID)
{
	if(tenantLSIs.count(graphID) == 0)
	{
		ULOG_DBG_INFO("The graph \"%s\" does not exist",graphID.c_str());
		assert(0);
		throw GraphManagerException();
	}
	highlevel::Graph *graph = (tenantLSIs.find(graphID))->second.getGraph();
	assert(graph != NULL);

	Object flow_graph;

	try
	{
		flow_graph[FORWARDING_GRAPH] = graph->toJSON();
	}catch(...)
	{
		assert(0);
		throw GraphManagerException();
	}

	return flow_graph;
}

bool GraphManager::deleteGraph(string graphID)
{
	if(tenantLSIs.count(graphID) == 0)
	{
		ULOG_DBG_INFO("The graph \"%s\" does not exist",graphID.c_str());
		return false;
	}

	/**
	*	@outline:
	*
	*		0) remove the rules from the LSI0
	*		1) stop the NFs
	*		2) delete the LSI, the virtual links and the
	*			ports related to NFs
	*		3) handle the internal LSI associated with the internal endpoints that are part of the graph
	*       4) if present, remove temp dir which contains VNF's file (used for the datadisk)
	*/

	ULOG_INFO("Deleting graph '%s'...",graphID.c_str());

	LSI *tenantLSI = (tenantLSIs.find(graphID))->second.getLSI();
	highlevel::Graph *highLevelGraph = (tenantLSIs.find(graphID))->second.getGraph();

	/**
	*		0) remove the rules from the LSI-0
	*/
	ULOG_DBG_INFO("0) Remove the rules from the LSI-0");

	lowlevel::Graph graphLSI0 = GraphTranslator::lowerGraphToLSI0(highLevelGraph,tenantLSI,graphInfoLSI0.getLSI(),internalLSIsConnections,false);
	graphLSI0lowLevel.removeRules(graphLSI0.getRules());

	//Remove rules from the LSI-0
	graphInfoLSI0.getController()->removeRules(graphLSI0.getRules());

	/**
	*		1) stop the NFs
	*/
	ComputeController *computeController = (tenantLSIs.find(graphID))->second.getComputeController();
#ifdef RUN_NFS
	ULOG_DBG_INFO("1) Stop the NFs");
	computeController->stopAll();
#else
	ULOG_DBG_INFO("1) Flag RUN_NFS disabled. No NF to be stopped");
#endif

	/**
	*		2) delete the LSI, the internal LSIs, the virtual links, the
	*			ports related to NFs and the ports related to GRE tunnel
	*/
	ULOG_DBG_INFO("2) Delete the LSI, the vlinks, and the ports used by NFs");

	try
	{
		//delete the LSI
		switchManager.destroyLsi(tenantLSI->getDpid());
	} catch (SwitchManagerException e)
	{
		ULOG_WARN("%s",e.what());
		throw GraphManagerException();
	}

	tenantLSIs.erase(tenantLSIs.find(highLevelGraph->getID()));

	delete(tenantLSI);
	delete(computeController);

	tenantLSI = NULL;
	computeController = NULL;

	ULOG_INFO("Tenant LSI (ID: %s) and its controller have been destroyed!",graphID.c_str());
	printInfo(graphLSI0lowLevel,graphInfoLSI0.getLSI());

	/**
	* 	3) If no other graph is still connected to an internal graph, such an internal graph can be deleted.
	*	Otherwise, only the flow and the virtual link related to the current graph are deleted.
	*/
	list<highlevel::EndPointInternal> internalEndpoints = highLevelGraph->getEndPointsInternal();
	for(list<highlevel::EndPointInternal>::iterator iep = internalEndpoints.begin(); iep != internalEndpoints.end(); iep++)
	{
		string internal_group(iep->getGroup());
		ULOG_DBG_INFO("3) Considering the internal graph associated with the internal group '%s'",internal_group.c_str());

		LSI *internalLSI = (internalLSIs.find(internal_group))->second.getLSI();
		//The number of virtual links of the internal LSI is equal to the number of graph attached to the internal endpoint represented by such an LSI
		assert((internalLSI->getVirtualLinks()).size() == timesUsedEndPointsInternal[internal_group]);

		if(timesUsedEndPointsInternal[internal_group] == 1)
		{
			//The internal graph representing this internal endpoint can be removed. In fact to other graph is connected with it :)
			ULOG_DBG_INFO("The internal graph associated with the internal group '%s' must be deleted",internal_group.c_str());

			try
			{
				//delete the LSI
				switchManager.destroyLsi(internalLSI->getDpid());
			} catch (SwitchManagerException e)
			{
				ULOG_WARN("%s",e.what());
				throw GraphManagerException();
			}

			internalLSIs.erase(internalLSIs.find(internal_group));
			internalLSIsConnections.erase(internalLSIsConnections.find(internal_group));
			assert(internalLSIsConnections.count(internal_group) == 0);

			delete(internalLSI);
			internalLSI = NULL;

			internalLSIsCreated[internal_group] = false;
		}
		else
		{
			//In this case, only the parts related to the current graph (i.e., a flow and a virtual link) must be deleted.
			ULOG_DBG_INFO("A virtual link and a rule must be deleted from the internal graph associated with the internal group '%s'",internal_group.c_str());

			//Remove the flow from the internal LSI
			GraphInfo internalGraphInfo = (internalLSIs.find(internal_group))->second;
			Controller *internalController = internalGraphInfo.getController();
			assert(internalController != NULL);

			stringstream ruleID;
			ruleID << "INTERNAL-GRAPH" << "-" << internal_group << "_" << highLevelGraph->getID(); //This guarantees that the ID of the rule is unique
			internalController->removeRuleFromID(ruleID.str());

			//Remove the virtual link connecting the internal LSI with the LSI-0

			assert(internalLSIsConnections.count(internal_group) != 0);
			map <string, unsigned int> internalLSIsConnectionsOfGroup = internalLSIsConnections[internal_group];
			assert(internalLSIsConnectionsOfGroup.count(graphID) != 0);
			unsigned int vlink_port_on_lsi_0 = internalLSIsConnectionsOfGroup[graphID];

			//We have to identify the virtual link to be removed
			vector<VLink> vlinks = internalLSI->getVirtualLinks();
			ULOG_DBG_INFO("The internal LSI has currently the following virtual links");
			vector<VLink>::iterator vl = vlinks.begin();
			for(; vl != vlinks.end(); vl++)
			{
				ULOG_DBG_INFO("\t\t(ID: %x) %x:%d -> %x:%d",vl->getID(),internalLSI->getDpid(),vl->getLocalID(),vl->getRemoteDpid(),vl->getRemoteID());
				if(vl->getRemoteID() == vlink_port_on_lsi_0)
				{
					ULOG_DBG_INFO("\t\t\tTHIS VLINK MUST BE REMOVED");
					DestroyVirtualLinkIn dvli(internalLSI->getDpid(), vl->getLocalID(), vl->getRemoteDpid(), vl->getRemoteID());
					switchManager.destroyVirtualLink(dvli);
					internalLSI->removeVlink(vl->getID());
					break;
				}
			}
			assert(vl != vlinks.end());
			if(vl == vlinks.end())
				return false;

			internalLSIsConnectionsOfGroup.erase(internalLSIsConnectionsOfGroup.find(graphID));
			internalLSIsConnections[internal_group] = internalLSIsConnectionsOfGroup;
			//The following two instruction check that everything has been removed correctly
			internalLSIsConnectionsOfGroup = internalLSIsConnections[internal_group];
			assert(internalLSIsConnectionsOfGroup.count(graphID) == 0);
		}

		timesUsedEndPointsInternal[internal_group]--;
		ULOG_DBG_INFO("The internal graph associated with the internal group '%s' is still used by %d other graphs",internal_group.c_str(),timesUsedEndPointsInternal[internal_group]);
	}//end iteration on the internal endpoints

	/**
	*    4) if present, remove temp dir which contains VNF's file (used for the datadisk)
    */
	string current_path = fs::current_path().string();
    string path = current_path+"/"+DIR_TO_SAVE_VNF_FILE;
    if(fs::exists(path) && fs::is_directory(path)){
        fs::path p = path;
        fs::remove_all(p);
        ULOG_DBG_INFO("Deleting dir %s", path.c_str());
    }


	delete(highLevelGraph);
	highLevelGraph = NULL;

	return true;
}

bool GraphManager::checkGraphValidity(highlevel::Graph *graph, ComputeController *computeController)
{
	list<highlevel::EndPointInterface> phyPorts = graph->getEndPointsInterface();
	list<highlevel::EndPointInternal> endPointsInternal = graph->getEndPointsInternal();
	list<highlevel::EndPointGre> endPointsGre = graph->getEndPointsGre();
	list<highlevel::EndPointVlan> endPointsVlan = graph->getEndPointsVlan();
	list<highlevel::EndPointHostStack> endPointsHoststack = graph->getEndPointsHostStack();

	string graphID = graph->getID();

	/**
	*	Check if the required interface endpoints (i.e., physical ports) are under the control of the un-orchestrator
	*/
	ULOG_DBG_INFO("The command requires %d new 'interface' endpoints",phyPorts.size());
	LSI *lsi0 = graphInfoLSI0.getLSI();
	map<string,unsigned int> physicalPorts = lsi0->getPhysicalPorts();
	for(list<highlevel::EndPointInterface>::iterator p = phyPorts.begin(); p != phyPorts.end(); p++)
	{
		string interfaceName = p->getInterface();
		ULOG_DBG_INFO("* %s",interfaceName.c_str());
		if((physicalPorts.count(interfaceName)) == 0)
		{
			ULOG_WARN("Physical port \"%s\" does not exist",interfaceName.c_str());
			return false;
		}
	}

	// TODO: check the validity of hoststack endpoints.
	for(list<highlevel::EndPointHostStack>::iterator e = endPointsHoststack.begin(); e != endPointsHoststack.end(); e++)
	{

	}

	/**
	*	No check is required for an internal endpoint
	*/
	ULOG_DBG_INFO("The command requires %d 'internal' endpoints",endPointsInternal.size());
	for(list<highlevel::EndPointInternal>::iterator i = endPointsInternal.begin(); i != endPointsInternal.end(); i++)
	{
		string group = i->getGroup();
		ULOG_DBG_INFO("* group: %s",group.c_str());
	}

	ULOG_DBG_INFO("The command requires %d 'hoststack' endpoints",endPointsHoststack.size());

	/**
	*	No check is required for a GRE endpoint
	*/
	ULOG_DBG_INFO("The command requires %d 'gre-tunnel' endpoints",endPointsGre.size());

	/**
	*	No check is required for a VLAN endpoint
	*/
	ULOG_DBG_INFO("The command requires %d 'vlan' endpoints",endPointsVlan.size());

	/**
	*	Check if the required network functions are available
	*/
	list<highlevel::VNFs> network_functions = graph->getVNFs();
	ULOG_DBG_INFO("The command requires to retrieve %d new NFs",network_functions.size());
	//The description must be actually retrieved only for new VNFs, and not for VNFs whose number of ports is changed
	for(list<highlevel::VNFs>::iterator nf = network_functions.begin(); nf != network_functions.end(); nf++)
	{
		nf_manager_ret_t retVal;
		//FIXME: not sure that this check is necessary
		if(computeController->getNFSelectedImplementation(nf->getId()))
		{
			ULOG_DBG_INFO("\t* NF with id \"%s\" is already part of the graph; it is not retrieved again",nf->getId().c_str());
			continue;
		}

		retVal = computeController->retrieveDescription(*nf);

		if(retVal == NFManager_NO_NF)
		{
			ULOG_WARN("NF \"%s\" cannot be retrieved",nf->getName().c_str());
			return false;
		}
		else if(retVal == NFManager_SERVER_ERROR)
		{
			throw GraphManagerException();
		}

	}

	/**
	*	The graph is valid!
	*/
	return true;
}

void *startNF(void *arguments)
{
	to_thread_t *args = (to_thread_t *)arguments;
	assert(args->computeController != NULL);

	if(!args->computeController->startNF(args->nf_id, args->namesOfPortsOnTheSwitch, args->portsConfiguration
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		, args->controlConfiguration, args->environmentVariables
#endif
        ,args->dir_to_mount, args->dst_path
	))
		return (void*) 0;
	else
		return (void*) 1;
}


bool GraphManager::newGraph(highlevel::Graph *graph)
{
	ULOG_INFO("Creating a new graph '%s'...",graph->getID().c_str());

	assert(tenantLSIs.count(graph->getID()) == 0);

	/**
*	@outline:
	*
	*		0) check the validity of the graph
	*		1) create the Openflow controller for the tenant LSI
	*		2) select an implementation for each NF of the graph
	*		3) create the LSI, with the proper ports
	*		4) start the network functions
	*		5) create the OpenFlow controller for the internal LSIs (if it does not exist yet)
	*		6) create the internal LSI (if it does not exist yet), with the proper vlinks and download the rules in internal-LSI
	*		7) download the rules in LSI-0, tenant-LSI
	*		8) assign ip address to host-stack endpoint
	*		9) set the configuration of the node
	*/

	/**
	*	@Considerations:
	*		- the number of ports of a VNF described in the graph are immediately created (also those ports not used in any flow)
	*		- the GRE-tunnel endpoints described in the graph are immediately created (also those not used in any flow)
	*		- the internal endpoints described in the graph are immediately created (also those not used in any flow)
	*/

	/**
	*	@Endpoint internal limitation:
	*		- connection from physical port to internal endpoint is not supported
	*		- connection from internal endpoint to physical port is not supported
	*/

	/**
	*	0) Check the validity of the graph
	*/
	ULOG_DBG_INFO("0) Checking the validity of the graph");
	ComputeController *computeController = new ComputeController();

	if(!checkGraphValidity(graph,computeController))
	{
		//This is an error in the request
		delete(computeController);
		computeController = NULL;
		return false;
	}

	/**
	*	1) Create the Openflow controller for the tenant LSI
	*/
	ULOG_DBG_INFO("1) Create the Openflow controller for the tenant LSI");

	pthread_mutex_lock(&graph_manager_mutex);
	uint32_t controllerPort = nextControllerPort;
	nextControllerPort++;
	pthread_mutex_unlock(&graph_manager_mutex);

	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	switch(OFP_VERSION)
	{
		case OFP_10:
			ULOG_DBG_INFO("\tUsing Openflow 1.0");
			versionbitmap.add_ofp_version(rofl::openflow10::OFP_VERSION);
			break;
		case OFP_12:
			ULOG_DBG_INFO("\tUsing Openflow 1.2");
			versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
			break;
		case OFP_13:
			ULOG_DBG_INFO("\tUsing Openflow 1.3");
			versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
			break;
	}

	lowlevel::Graph graphTmp ;
	Controller *controller = new Controller(versionbitmap,graphTmp, controllerPort);
	controller->start();

	/**
	*	2) Select an implementation for each network function of the graph
	*/
	ULOG_DBG_INFO("2) Select an implementation for each NF of the graph");
	if(!computeController->selectImplementation())
	{
		//This is an internal error
		delete(computeController);
		delete(controller);
		computeController = NULL;
		controller = NULL;
		throw GraphManagerException();
	}

	/**
	*	3) Create the LSI
	*/
	ULOG_DBG_INFO("3) Create the LSI");
	list<highlevel::VNFs> network_functions = graph->getVNFs();
	list<highlevel::EndPointInternal> endpointsInternal = graph->getEndPointsInternal();
	list<highlevel::EndPointGre> endpointsGre = graph->getEndPointsGre();
	list<highlevel::EndPointHostStack> endpointsHoststack = graph->getEndPointsHostStack();


	vector<set<string> > vlVector = identifyVirtualLinksRequired(graph);
	set<string> vlNFs = vlVector[0];
	set<string> vlPhyPorts = vlVector[1];
	set<string> vlEndPointsGre = vlVector[2];
	set<string> vlEndPointsInternal = vlVector[3];
	set<string> vlEndPointsHoststack = vlVector[4];

	/**
	*	A virtual link can be used in two direction, hence it can be shared between a NF port and a physical port.
	*	In principle a virtual link could also be shared between a NF port and an endpoint but, for simplicity, we
	*	use separated virtual links in case of endpoint.
	*/
	unsigned int toUp = vlNFs.size() + vlEndPointsGre.size() + vlEndPointsHoststack.size();
	unsigned int toDown = vlPhyPorts.size() + vlEndPointsInternal.size();
	unsigned int numberOfVLrequired = (toUp > toDown)? toUp : toDown;

	ULOG_DBG_INFO("%d virtual links are required to connect the new LSI with LSI-0",numberOfVLrequired);

	vector<VLink> virtual_links;
	for(unsigned int i = 0; i < numberOfVLrequired; i++)
		virtual_links.push_back(VLink(dpid0));

	//The tenant-LSI is not connected to physical ports, but just the LSI-0
	//through virtual links, to network functions through virtual ports, and to gre tunnels and hoststack endpoint through proper ports
	set<string> dummyPhyPorts;

	//Check the types of the VNFs ports
	map<string, nf_t>  nf_types;
	map<string, map<unsigned int, PortTechnology> > nfs_ports_type;
	for(list<highlevel::VNFs>::iterator nf_it = network_functions.begin(); nf_it != network_functions.end(); nf_it++)
	{
		const string& nf_id = nf_it->getId();
		list<unsigned int> nf_ports = nf_it->getPortsId();

		nf_types[nf_id] = computeController->getNFType(nf_id);

		//Gather VNF ports types
		Description* descr = computeController->getNFSelectedImplementation(nf_id);
		map<unsigned int, PortTechnology> nf_ports_type = descr->getPortTechnologies();  // Port types as specified by the retrieved and selected NF implementation

		ULOG_DBG_INFO("NF with id \"%s\" selected implementation (type %d) defines type for %d ports", nf_id.c_str(), nf_types[nf_id], nf_ports_type.size());
		// Fill in incomplete port type specifications (unless we make it mandatory input from name-resolver)
		for (list<unsigned int>::iterator p_it = nf_ports.begin(); p_it != nf_ports.end(); p_it++) {
			map<unsigned int, PortTechnology>::iterator pt_it = nf_ports_type.find(*p_it);
			if(pt_it->second == UNDEFINED_PORT) {
				ULOG_WARN("\tNF Port \"%s\":%d has no type defined", nf_id.c_str(), (*p_it));
				ULOG_WARN("\tThe ports ID used in the graph must correspond to those specified in the network function template...");
				//This is an error of the client, which specified a wrong NF-FG (wrong ports towards a VNF)
				delete(computeController);
				delete(controller);
				computeController = NULL;
				controller = NULL;
				return false;
			}
			ULOG_DBG_INFO("\tNF Port \"%s\":%d is of type '%s'", nf_id.c_str(), (*p_it), portTechnologyToString(pt_it->second).c_str());
		}
		nfs_ports_type[nf_id] = nf_ports_type;
	}

	//Prepare the structure representing the new tenant-LSI
	LSI *lsi = new LSI(string(OF_CONTROLLER_ADDRESS), controllerPort, dummyPhyPorts, network_functions,
		endpointsGre,virtual_links,nfs_ports_type,endpointsHoststack);

	CreateLsiOut *clo = NULL;
	try
	{
		//Create a new tenant-LSI

		CreateLsiIn cli(string(OF_CONTROLLER_ADDRESS),controllerPort, lsi->getPhysicalPortsName(), lsi->getHostackEndpointID(), nf_types, lsi->getNetworkFunctionsPortsInfo(), lsi->getGreEndpointsDescription(), lsi->getVirtualLinksRemoteLSI(), string(OF_CONTROLLER_ADDRESS), Configuration::instance()->getIpsecCertificate());

		clo = switchManager.createLsi(cli);

		lsi->setDpid(clo->getDpid());

		map<string,unsigned int> physicalPorts = clo->getPhysicalPorts();
		if(physicalPorts.size() > 1)
		{
			ULOG_ERR("Non required physical ports have been attached to the tenant-lsi");
			delete(clo);
			throw GraphManagerException();
		}

		map<string,map<string, unsigned int> > nfsports = clo->getNetworkFunctionsPorts();
		//TODO: check if the number of vnfs and ports is the same required
		for(map<string,map<string, unsigned int> >::iterator nfp = nfsports.begin(); nfp != nfsports.end(); nfp++)
		{
			if(!lsi->setNfSwitchPortsID(nfp->first,nfp->second))
			{
				ULOG_ERR("A non-required network function port related to the network function with id \"%s\" has been attached to the tenant-lsi",nfp->first.c_str());
				delete(clo);
				throw GraphManagerException();
			}
		}

		map<string,unsigned int > epsports = clo->getGreEndpointsPorts();
		for(map<string,unsigned int >::iterator ep = epsports.begin(); ep != epsports.end(); ep++)
		{
			if(!lsi->setGreEndpointPortID(ep->first, ep->second))
			{
				ULOG_ERR("A non-required gre end point port \"%s\" has been attached to the tenant-lsi",ep->first.c_str());
				delete(clo);
				throw GraphManagerException();
			}
		}

		map<string,unsigned int > hoststackEPsports = clo->getHoststackEndpointPorts();
		for(map<string,unsigned int >::iterator hs = hoststackEPsports.begin(); hs != hoststackEPsports.end(); hs++)
		{
			if(!lsi->setHoststackEndpointPortID(hs->first, hs->second))
			{
				ULOG_ERR("A non-required hoststack end point port \"%s\" has been attached to the tenant-lsi",hs->first.c_str());
				delete(clo);
				throw GraphManagerException();
			}
		}

		map<string,string> hoststackEPsPortName = clo->getHoststackPortsName();
		for(map<string,string >::iterator hs = hoststackEPsPortName.begin(); hs != hoststackEPsPortName.end(); hs++)
		{
			if(!lsi->setHoststackEndpointPortName(hs->first, hs->second))
			{
				ULOG_ERR("A non-required hoststack end point port \"%s\" has been attached to the tenant-lsi",hs->first.c_str());
				delete(clo);
				throw GraphManagerException();
			}
		}

		map<string,map<string, unsigned int> > networkFunctionsPortsNameOnSwitch = clo->getNetworkFunctionsPortsNameAndID();

		for(map<string,map<string, unsigned int> >::iterator nfpnos = networkFunctionsPortsNameOnSwitch.begin(); nfpnos != networkFunctionsPortsNameOnSwitch.end(); nfpnos++)
			lsi->setNetworkFunctionsPortsNameOnSwitch(nfpnos->first, nfpnos->second);

		list<pair<unsigned int, unsigned int> > vl = clo->getVirtualLinks();
		//TODO: check if the number of vlinks is the same required
		unsigned int currentTranslation = 0;
		for(list<pair<unsigned int, unsigned int> >::iterator it = vl.begin(); it != vl.end(); it++)
		{
			lsi->setVLinkIDs(currentTranslation,it->first,it->second);
			currentTranslation++;
		}
		delete(clo);

	} catch (SwitchManagerException e)
	{
		ULOG_ERR("%s",e.what());
		switchManager.destroyLsi(lsi->getDpid());
		if(clo != NULL)
			delete(clo);
		delete(graph);
		delete(lsi);
		delete(computeController);
		delete(controller);
		graph = NULL;
		lsi = NULL;
		computeController = NULL;
		controller = NULL;
		throw GraphManagerException();
	}

	uint64_t dpid = lsi->getDpid();

	map<string,unsigned int> lsi_ports = lsi->getPhysicalPorts();
	set<string> nfs = lsi->getNetworkFunctionsId();
	list<highlevel::EndPointGre > eps = lsi->getGreEndpointsPorts();
	vector<VLink> vls = lsi->getVirtualLinks();

	//The following code just prints information on the VNFs that are part of the graph
	ULOG_DBG_INFO("LSI ID: %d",dpid);
	for(list<highlevel::VNFs>::iterator nf = network_functions.begin(); nf != network_functions.end(); nf++)
	{
		ULOG_DBG_INFO("\t\tNF id %s:",(nf->getId()).c_str());
		list<highlevel::vnf_port_t> nf_ports = nf->getPorts();
		ULOG_DBG_INFO("\t\t\tPorts (%d):",nf_ports.size());

		for(list<highlevel::vnf_port_t>::iterator n = nf_ports.begin(); n != nf_ports.end(); n++)
		{
			ULOG_DBG_INFO("\t\t\t\t* %s - %s",(n->name).c_str(),(n->id).c_str());
			port_network_config config = n->configuration;

			if(!(config.mac_address).empty())
				ULOG_DBG_INFO("\t\t\t\t\tMac address -> %s",(config.mac_address).c_str());
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
			if(!(config.ip_address).empty())
				ULOG_DBG_INFO("\t\t\t\t\tIP address -> %s",(config.ip_address).c_str());
#endif
		}

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		list<string> nf_environment_variables = nf->getEnvironmentVariables();
		if(nf_environment_variables.size() != 0)
		{
			ULOG_DBG_INFO("\t\t\tEnvironment variables (%d):",nf_environment_variables.size());
			for(list<string>::iterator ev = nf_environment_variables.begin(); ev != nf_environment_variables.end(); ev++)
				ULOG_DBG_INFO("\t\t\t\t* %s",ev->c_str());
		}

		list<port_mapping_t> control_ports = nf->getControlPorts();
		ULOG_DBG_INFO("\t\t\tControl interfaces (%d):",control_ports.size());
		for(list<port_mapping_t >::iterator n = control_ports.begin(); n != control_ports.end(); n++)
		{
			ULOG_DBG_INFO("\t\t\t\tHost TCP port -> %s",(n->host_port).c_str());
			ULOG_DBG_INFO("\t\t\t\tVNF TCP port -> %s",(n->guest_port).c_str());
		}
#endif
	}

	//The following code just prints information on gre-tunnel endpoints
	ULOG_DBG_INFO("Gre end points (%d):",eps.size());
	for(list<highlevel::EndPointGre>::iterator it = eps.begin(); it != eps.end(); it++){
		int id = 0;
		sscanf(it->getId().c_str(), "%d", &id);
		ULOG_DBG_INFO("\t\tID %d:", id);
		ULOG_DBG_INFO("\t\t\t\tKey: %s", it->getGreKey().c_str());
		ULOG_DBG_INFO("\t\t\t\tLocal ip: %s", it->getLocalIp().c_str());
		ULOG_DBG_INFO("\t\t\t\tRemote_ip: %s", it->getRemoteIp().c_str());
	}

	//The following code prints information on the virtual link just created and that are necessary to interconnect the tenant LSI to the LSI-0
	ULOG_DBG_INFO("Virtual links (%u): ",vls.size());
	for(vector<VLink>::iterator v = vls.begin(); v != vls.end(); v++)
		//localID is the identifier of the virtual link on the tenant LSI
		//remoteID is the identifier of the virtual link on the LSI-0
		ULOG_DBG_INFO("\t\t(ID: %x) %x:%d -> %x:%d",v->getID(),dpid,v->getLocalID(),v->getRemoteDpid(),v->getRemoteID());

	//vlToUp is used to associate a virtual link on an action to VNF or gre-tunnels
	//vlToDown is used to associate a virtual link on an action to phisical ports or internal endpoints
	if(vls.size() != 0)
	{
		//Virtual links are required to implement the graph

		ULOG_DBG_INFO("NF port is virtual link ID (up):");
		map<string, uint64_t> nfs_vlinks;
		vector<VLink>::iterator vlToUp = vls.begin();
		//Associate a virtual link to the network functions ports that needed it
		for(set<string>::iterator nf = vlNFs.begin(); nf != vlNFs.end(); nf++, vlToUp++)
		{
			nfs_vlinks[*nf] = vlToUp->getID();
			ULOG_DBG_INFO("\t\t%s -> %x",(*nf).c_str(),vlToUp->getID());
		}
		lsi->setNFsVLinks(nfs_vlinks);

		//associate the vlinks to the physical ports
		//Note that the same virtual link can be used both for a VNF port and for a physical interface
		ULOG_DBG_INFO("Physical port is virtual link ID (down):");
		map<string, uint64_t> ports_vlinks;
		vector<VLink>::iterator vlToDown = vls.begin();
		for(set<string>::iterator p = vlPhyPorts.begin(); p != vlPhyPorts.end(); p++, vlToDown++)
		{
			ports_vlinks[*p] = vlToDown->getID();
			ULOG_DBG_INFO("\t\t%s -> %x",(*p).c_str(),vlToDown->getID());
		}
		lsi->setPortsVLinks(ports_vlinks);

		//associate the vlinks to the gre end points
		ULOG_DBG_INFO("Gre endpoint is virtual link ID (up):");
		map<string, uint64_t> gre_endpoints_vlinks;

		for(set<string>::iterator ep = vlEndPointsGre.begin(); ep != vlEndPointsGre.end(); ep++, vlToUp++)
		{
			gre_endpoints_vlinks[*ep] = vlToUp->getID();
			ULOG_DBG_INFO("\t\t%s -> %x",(*ep).c_str(),vlToUp->getID());
		}			

		lsi->setEndPointsGreVLinks(gre_endpoints_vlinks);

		//associate the vlinks to the internal end points
		ULOG_DBG_INFO("Internal end point is virtual link ID (down):");
		map<string, uint64_t> endpoints_internal_vlinks;

		for(set<string>::iterator ep = vlEndPointsInternal.begin(); ep != vlEndPointsInternal.end(); ep++, vlToDown++)
		{
			endpoints_internal_vlinks[*ep] = vlToDown->getID();
			ULOG_DBG_INFO("\t\t%s -> %x",(*ep).c_str(),vlToDown->getID());
		}
		lsi->setEndPointsVLinks(endpoints_internal_vlinks);

		//associate the vlinks to the hoststack end points
		ULOG_DBG_INFO("Hoststack endpoint is virtual link ID (up):");
		map<string, uint64_t> endpoints_hoststack_vlinks;

		for(set<string>::iterator ep = vlEndPointsHoststack.begin(); ep != vlEndPointsHoststack.end(); ep++, vlToUp++)
		{
			endpoints_hoststack_vlinks[*ep] = vlToUp->getID();
			ULOG_DBG_INFO("\t\t%s -> %x",(*ep).c_str(),vlToUp->getID());
		}

		lsi->setEndPointsHoststackVLinks(endpoints_hoststack_vlinks);
	}

	/**
	*	4) Start the network functions
	*/
#ifdef RUN_NFS
	ULOG_DBG_INFO("4) start the network functions");

	computeController->setLsiID(dpid);
	#ifndef STARTVNF_SINGLE_THREAD
	pthread_t some_thread[network_functions.size()];
	#endif
	to_thread_t thr[network_functions.size()];
	int i = 0;

	string current_path = fs::current_path().string();
	string vnf_file_root_path = current_path+"/"+DIR_TO_SAVE_VNF_FILE;
	if(!(fs::exists(vnf_file_root_path) && fs::is_directory(vnf_file_root_path)))
	    fs::create_directory(vnf_file_root_path);
	string vnf_file_path = "";

    // Iterate for each network functions
	for(list<highlevel::VNFs>::iterator nf = network_functions.begin(); nf != network_functions.end(); nf++)
	{
	    //Ask to Config Service if there are file associated to NF
	    string tenant_id = "33";
	    list<string> fileList;
	    string dir_to_mount = "";
	    string dst_path = "";
	    try{
	        ULOG_WARN("Trying to retrieve VNF's file list of: %s...", nf->getId().c_str());
	        fileList = computeController->retrieveFileList(tenant_id, graph->getID().c_str(), nf->getId().c_str());
            ULOG_WARN("Trying to retrieve VNF's file list of: %s...Done! Found %d file!", nf->getId().c_str(), fileList.size());
            if(fileList.size()>0){
                vnf_file_path = vnf_file_root_path+"/"+tenant_id+"_"+graph->getID()+"_"+nf->getId();
                fs::create_directory(vnf_file_path);
                dir_to_mount = vnf_file_path+"/";
                dst_path = Configuration::instance()->getDatadiskDestPath();
                string path_file = "";
                for(string filename : fileList){
                    ULOG_WARN("\tTrying to retrieve file: %s...", filename.c_str());
                    try{
                        path_file = computeController->retrieveFile(tenant_id, graph->getID().c_str(), nf->getId(), filename, vnf_file_path);
                    }catch(const std::exception &e){
                        ULOG_DBG_INFO("\tTrying to retrieve file: %s...Error! Exception: %s", filename.c_str(), e.what());
                        throw;
                    }
                    ULOG_WARN("\tTrying to retrieve file: %s...Done! Saved in: %s", filename.c_str(), path_file.c_str());
                }
                ULOG_DBG_INFO("NF %s will be start with a datadisk", nf->getId().c_str());
                ULOG_DBG_INFO("\tHost dir to mount: %s", dir_to_mount.c_str());
	            ULOG_DBG_INFO("\tDestination path: %s", dst_path.c_str());
            }
	    }catch(const std::exception &e){
	        ULOG_WARN("Trying to retrieve VNF's file of: %s...Error! Exception: %s", nf->getId().c_str(), e.what());
	        dir_to_mount = "";
	        dst_path = "";
	        ULOG_DBG_INFO("NF %s will be start without datadisk", nf->getId().c_str());
	    }

		thr[i].nf_id = nf->getId();
		thr[i].computeController = computeController;
		thr[i].namesOfPortsOnTheSwitch = lsi->getNetworkFunctionsPortsNameOnSwitchMap(nf->getId());
		thr[i].portsConfiguration = nf->getPortsID_configuration();
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		thr[i].controlConfiguration = nf->getControlPorts();
		thr[i].environmentVariables = nf->getEnvironmentVariables();
#endif
        thr[i].dir_to_mount = dir_to_mount;
        thr[i].dst_path = dst_path;

#ifdef STARTVNF_SINGLE_THREAD
		startNF((void *) &thr[i]);
#else
		if (pthread_create(&some_thread[i], NULL, &startNF, (void *)&thr[i]) != 0)
		{
			assert(0);
			ULOG_ERR("An error occurred while creating a new thread");
			throw GraphManagerException();
		}
#endif
		i++;
	}

	bool ok = true;
#ifndef STARTVNF_SINGLE_THREAD
	for(int j = 0; j < i;j++)
	{
		void *returnValue;
		pthread_join(some_thread[j], &returnValue);
		int *c = (int*)returnValue;

		if(c == 0)
			ok = false;
	}
#endif

	if(!ok)
	{
		for(list<highlevel::VNFs>::iterator nf = network_functions.begin(); nf != network_functions.end(); nf++)
			computeController->stopNF(nf->getId());

		switchManager.destroyLsi(lsi->getDpid());

		delete(graph);
		delete(lsi);
		delete(computeController);
		delete(controller);

		graph = NULL;;
		lsi = NULL;
		computeController = NULL;
		controller = NULL;

		throw GraphManagerException();
	}

#else
	ULOG_DBG_INFO("3) Flag RUN_NFS disabled. NFs will not start");
#endif

	/**
	*	5) Create the Openflow controller for each internal LSI
	*/
	handleControllerForInternalEndpoint(graph);

	/**
	*	6) Handle the Internal controller graph
	*/
	try
	{
		handleGraphForInternalEndpoint(graph);
	}
	catch(GraphManagerException e)
	{
		for(list<highlevel::VNFs>::iterator nf = network_functions.begin(); nf != network_functions.end(); nf++)
			computeController->stopNF(nf->getId());

		switchManager.destroyLsi(lsi->getDpid());

		delete(graph);
		delete(lsi);
		delete(computeController);
		delete(controller);

		graph = NULL;;
		lsi = NULL;
		computeController = NULL;
		controller = NULL;
		throw GraphManagerException();
	}

	/**
	*	7) Create the rules and download them in LSI-0, tenant-LSI
	*/
	ULOG_DBG_INFO("7) Create the rules and download them in LSI-0, tenant-LSI");
	try
	{
		//creates the rules for LSI-0 and for the tenant-LSI
		lowlevel::Graph graphLSI0 = GraphTranslator::lowerGraphToLSI0(graph, lsi, graphInfoLSI0.getLSI(), internalLSIsConnections);
		ULOG_DBG_INFO("New graph for LSI-0:");
		graphLSI0.print();

		graphLSI0lowLevel.addRules(graphLSI0.getRules());
		list<lowlevel::Rule> llrules = graphLSI0.getRules();
		ULOG_DBG_INFO("Graph for LSI-0 (%d new rules added):",llrules.size());
		graphLSI0.print();

		lowlevel::Graph graphTenant =  GraphTranslator::lowerGraphToTenantLSI(graph,lsi,graphInfoLSI0.getLSI());
		ULOG_DBG_INFO("Graph for tenant LSI:");
		graphTenant.print();

		controller->installNewRules(graphTenant.getRules());

		GraphInfo graphInfoTenantLSI;
		graphInfoTenantLSI.setGraph(graph);
		graphInfoTenantLSI.setComputeController(computeController);
		graphInfoTenantLSI.setLSI(lsi);
		graphInfoTenantLSI.setController(controller);

		//Save the graph information
		tenantLSIs[graph->getID()] = graphInfoTenantLSI;

		ULOG_INFO("Tenant LSI (ID: %s) and its controller are created",graph->getID().c_str());

		ULOG_DBG_INFO("Adding the new rules to the LSI-0");
		(graphInfoLSI0.getController())->installNewRules(graphLSI0.getRules());

		printInfo(graphLSI0lowLevel,graphInfoLSI0.getLSI());

	} catch (SwitchManagerException e)
	{
#ifdef RUN_NFS
		for(list<highlevel::VNFs>::iterator nf = network_functions.begin(); nf != network_functions.end(); nf++)
			computeController->stopNF(nf->getId());
#endif

		switchManager.destroyLsi(lsi->getDpid());

		if(tenantLSIs.count(graph->getID()) != 0)
			tenantLSIs.erase(tenantLSIs.find(graph->getID()));

		delete(graph);
		delete(lsi);
		delete(computeController);
		delete(controller);

		graph = NULL;
		lsi = NULL;
		computeController = NULL;
		controller = NULL;

		//TODO: also delete things related to the internal endpoint

		ULOG_ERR("%s",e.what());
		throw GraphManagerException();
	}

	// 8) assign ip address to host-stack endpoint
	map<string,string> hsPortsName = lsi->getHoststackEndpointPortName();
	for(list<highlevel::EndPointHostStack>::iterator hs = endpointsHoststack.begin(); hs!=endpointsHoststack.end();hs++)
	{
		if (hs->getConfiguration()==DHCP)
			interfaceManager.getIpAddressFromDhcp(hsPortsName[hs->getId()]);
		else if (hs->getConfiguration()==PPPOE)
			interfaceManager.getIpAddressFromPppoe(hsPortsName[hs->getId()]);
		else
			interfaceManager.setStaticIpAddress(hsPortsName[hs->getId()],hs->getIpAddress());

		string macAddress= hs->getMacAddress();
		if(macAddress!="")
			interfaceManager.setMacAddress(hsPortsName[hs->getId()],macAddress);
	}
#ifdef ENABLE_NODE_CONFIG
    // 9) set the configuration of the node
	bool setted = configurationAgent.setNodeConfiguration(graph);
	if (!setted)
	{
		ULOG_ERR("An error occurred setting the node configuration");
		throw GraphManagerException();
	}
	//configurationAgent.setNodeConfiguration(graph);
#endif

	return true;
}

void GraphManager::handleControllerForInternalEndpoint(highlevel::Graph *graph)
{
	list<highlevel::EndPointInternal> internalEPs = graph->getEndPointsInternal();

	for(list<highlevel::EndPointInternal>::iterator iep = internalEPs.begin(); iep != internalEPs.end(); iep++)
	{
		string internal_group(iep->getGroup());

		//In case the internal LSI representing the current internal endpoints has not been yet created, let's create its controller
		if(!internalLSIsCreated[internal_group])
		{
			ULOG_DBG_INFO("Create the Openflow controller for the internal endpoint '%s'",internal_group.c_str());

			pthread_mutex_lock(&graph_manager_mutex);
			uint32_t controllerPort = nextControllerPort;
			nextControllerPort++;
			pthread_mutex_unlock(&graph_manager_mutex);

			rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
			switch(OFP_VERSION)
			{
				case OFP_10:
					ULOG_DBG_INFO("\tUsing Openflow 1.0");
					versionbitmap.add_ofp_version(rofl::openflow10::OFP_VERSION);
					break;
				case OFP_12:
					ULOG_DBG_INFO("\tUsing Openflow 1.2");
					versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
					break;
				case OFP_13:
					ULOG_DBG_INFO("\tUsing Openflow 1.3");
					versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
					break;
			}

			//create a new OF controller associated with the internal LSI
			lowlevel::Graph graphTmp;
			Controller *controller = new Controller(versionbitmap,graphTmp, controllerPort);
			controller->start();

			//store the information related to the OpenFlow controller associated with the internal LSI
			GraphInfo graphInfoInternalLSI;
			graphInfoInternalLSI.setController(controller);
			internalLSIs[internal_group] = graphInfoInternalLSI;
		}
	}
}

void GraphManager::handleGraphForInternalEndpoint(highlevel::Graph *graph)
{
	list<highlevel::EndPointInternal> internalEPs = graph->getEndPointsInternal();

	//Iterate on the internal endpoints of the graph
	for(list<highlevel::EndPointInternal>::iterator iep = internalEPs.begin(); iep != internalEPs.end(); iep++)
	{
		string internal_group(iep->getGroup());
		ULOG_DBG_INFO("handling the internal LSI representing the internal-group: \"%s\"", internal_group.c_str());

		GraphInfo graphInfoInternalLSI = internalLSIs[internal_group];
		Controller *controller = graphInfoInternalLSI.getController();

		//create a new internal LSI if does not exists yet
		//this LSI is created with a virtual link connected to the LSI-0. The port on the LSI-0 corresponding to the virtual link will be used in the creation of the rules
		//to implement the tenant-LSI
		if(!internalLSIsCreated[internal_group])
		{
			ULOG_DBG_INFO("Create the internal LSI related to internal-group: \"%s\"", internal_group.c_str());

			unsigned controllerPort = controller->getControllerPort();

			vector<VLink> virtual_links;
			virtual_links.push_back(VLink(dpid0));

			//The internal-LSI is neither connected to physical ports, nor to VNFs. It is in fact just
			//connected to the LSI-0 through virtual links
			set<string> dummyPhyPorts;
			list<highlevel::VNFs> dummyNetworkFunctions;
			list<highlevel::EndPointGre> dummyEndpointsGre;
			list<highlevel::EndPointHostStack> dummyEndpointHoststack;
			map<string, map<unsigned int, PortTechnology> > dummyNfsPortsType;

			//Prepare the structure representing the new internal-LSI
			LSI *lsi = new LSI(string(OF_CONTROLLER_ADDRESS),
							   controllerPort, dummyPhyPorts, dummyNetworkFunctions,
							   dummyEndpointsGre, virtual_links,
							   dummyNfsPortsType,
							   dummyEndpointHoststack);

			CreateLsiOut *clo = NULL;
			try
			{
				map<string, nf_t> dummyNfsPortsTypeForCli;
				//Create a new internal-LSI

				CreateLsiIn cli(string(OF_CONTROLLER_ADDRESS),controllerPort, lsi->getPhysicalPortsName(),
								lsi->getHostackEndpointID() , dummyNfsPortsTypeForCli, lsi->getNetworkFunctionsPortsInfo(), lsi->getGreEndpointsDescription(), lsi->getVirtualLinksRemoteLSI(), string(OF_CONTROLLER_ADDRESS), Configuration::instance()->getIpsecCertificate());
				
				clo = switchManager.createLsi(cli);

				lsi->setDpid(clo->getDpid());

				map<string,unsigned int> physicalPorts = clo->getPhysicalPorts();
				if(physicalPorts.size() > 0)
				{
					ULOG_ERR("Non required physical ports have been attached to the internal-lsi");
					delete(lsi);
					delete(controller);
					lsi = NULL;
					controller = NULL;
					delete(clo);
					throw GraphManagerException();
				}

				map<string,map<string, unsigned int> > nfsports = clo->getNetworkFunctionsPorts();
				if(nfsports.size() > 0)
				{
					ULOG_ERR("A non-required network function has been attached to the internal-lsi");
					delete(lsi);
					delete(controller);
					lsi = NULL;
					controller = NULL;
					delete(clo);
					throw GraphManagerException();
				}

				map<string,unsigned int > epsports = clo->getGreEndpointsPorts();//FIXME this is gre endpoint
				if(epsports.size() > 0)
				{
					ULOG_ERR("A non-required gre-tunnel endpoint has been attached to the internal-lsi");
					delete(lsi);
					delete(controller);
					lsi = NULL;
					controller = NULL;
					delete(clo);
					throw GraphManagerException();
				}

				list<pair<unsigned int, unsigned int> > vl = clo->getVirtualLinks();
				//TODO: check if the number of vlinks is the same required
				unsigned int currentTranslation = 0;
				for(list<pair<unsigned int, unsigned int> >::iterator it = vl.begin(); it != vl.end(); it++)
				{
					lsi->setVLinkIDs(currentTranslation,it->first,it->second);
					currentTranslation++;
				}

				//the internal LSI now created
				internalLSIsCreated[internal_group] = true;
				timesUsedEndPointsInternal[internal_group] = 1;

				delete(clo);
			} catch (SwitchManagerException e)
			{
				ULOG_ERR("%s",e.what());
				switchManager.destroyLsi(lsi->getDpid());
				if(clo != NULL)
					delete(clo);
				delete(lsi);
				delete(controller);
				lsi = NULL;
				controller = NULL;
				throw GraphManagerException();
			}

			/**
			*	At this point the internal LSI is created
			*/

			uint64_t dpid = lsi->getDpid();
			vector<VLink> vls = lsi->getVirtualLinks();

			ULOG_DBG_INFO("LSI ID: %d",dpid);

			assert(vls.size() == 1);

			//associate the vlinks to the internal end points
			ULOG_DBG_INFO("Internal end point '%s' is virtual link ID:",internal_group.c_str());
			map<string, uint64_t> endpoints_internal_vlinks;

			for(vector<VLink>::iterator v = vls.begin(); v != vls.end(); v++)
			{
				//localID is the identifier of the virtual link on the internal LSI
				//remoteID is the identifier of the virtual link on the LSI-0
				endpoints_internal_vlinks[internal_group] = v->getID();
				ULOG_DBG_INFO("\t\t(ID: %x) %x:%d -> %x:%d",v->getID(),dpid,v->getLocalID(),v->getRemoteDpid(),v->getRemoteID());

				//Save the port of the LSI-0 that will be used to match and send packets through the internal endpoint
				map<string, unsigned int> internalLSIsConnectionOfAGroup = internalLSIsConnections[internal_group];
				internalLSIsConnectionOfAGroup[graph->getID()] = v->getRemoteID();
				internalLSIsConnections[internal_group] = internalLSIsConnectionOfAGroup;
			}

			lsi->setEndPointsVLinks(endpoints_internal_vlinks);

			graphInfoInternalLSI.setLSI(lsi);
			internalLSIs[internal_group] = graphInfoInternalLSI;

			/**
			*	[ivanofrancesco] maybe it must be moved after the 'else'
			*/
			/**
			*	Insert a rule on the internal LSI, which matches on the virtual link and that has NORMAL as action
			*/
			lowlevel::Graph internalLSIlowLevel;
			vector<VLink>::iterator vl = vls.begin();

			lowlevel::Match match;
			unsigned int vlink_local_id = vl->getLocalID();
			match.setInputPort(vlink_local_id);
			lowlevel::Action action(lowlevel::ACTION_NORMAL);

			stringstream newRuleID;
			newRuleID << "INTERNAL-GRAPH" << "-" << internal_group << "_" << graph->getID(); //This guarantees that the ID of the rule is unique
			lowlevel::Rule lsiRule(match,action,newRuleID.str(),HIGH_PRIORITY);
			internalLSIlowLevel.addRule(lsiRule);

			ULOG_DBG_INFO("Graph for internal LSI related to internal group \"%s\":", internal_group.c_str());
			internalLSIlowLevel.print();

			//Insert new rules into the internal-LSI
			ULOG_DBG_INFO("Adding the new rules to the internal-LSI");
			controller->installNewRules(internalLSIlowLevel.getRules());

		}
		else
		{
			ULOG_DBG_INFO("The internal LSI related to internal-group \"%s\" already exists and must be updated!", internal_group.c_str());

			LSI *lsi =  graphInfoInternalLSI.getLSI();

			VLink newLink(graphInfoLSI0.getLSI()->getDpid());
			int vlinkPosition = lsi->addVlink(newLink);

			AddVirtualLinkIn avli(lsi->getDpid(), graphInfoLSI0.getLSI()->getDpid());
			AddVirtualLinkOut *avlo = switchManager.addVirtualLink(avli);

			assert(avlo != NULL);
			lsi->setVLinkIDs(vlinkPosition,avlo->getIdA(),avlo->getIdB());

			uint64_t vlinkID = newLink.getID();
			VLink vlink = lsi->getVirtualLink(vlinkID); //FIXME: vlink is the same of newLink
			ULOG_DBG_INFO("Virtual link: (ID: %x) %x:%d -> %x:%d",vlink.getID(),lsi->getDpid(),vlink.getLocalID(),vlink.getRemoteDpid(),vlink.getRemoteID());

			//Save the port of the LSI-0 that will be used to match and send packets through the internal endpoint
			map<string, unsigned int> internalLSIsConnectionOfAGroup = internalLSIsConnections[internal_group];
			internalLSIsConnectionOfAGroup[graph->getID()] = vlink.getRemoteID();
			internalLSIsConnections[internal_group] = internalLSIsConnectionOfAGroup;

			graphInfoInternalLSI.setLSI(lsi);//FIXME: is it needed to store again the LSI, since it is a pointer?
			internalLSIs[internal_group] = graphInfoInternalLSI;

			timesUsedEndPointsInternal[internal_group]++;

			delete avlo;

			/**
			*	Insert a rule on the internal LSI, which matches on the virtual link and that has NORMAL as action
			*/
			lowlevel::Graph internalLSIlowLevel;

			lowlevel::Match match;
			unsigned int vlink_local_id = vlink.getLocalID();
			match.setInputPort(vlink_local_id);
			lowlevel::Action action(lowlevel::ACTION_NORMAL);

			stringstream newRuleID;
			newRuleID << "INTERNAL-GRAPH" << "-" << internal_group << "_" << graph->getID(); //This guarantees that the ID of the rule is unique
			lowlevel::Rule lsiRule(match,action,newRuleID.str(),HIGH_PRIORITY);
			internalLSIlowLevel.addRule(lsiRule);

			ULOG_DBG_INFO("Graph for internal LSI related to internal group \"%s\":", internal_group.c_str());
			internalLSIlowLevel.print();

			//Insert new rules into the internal-LSI
			ULOG_DBG_INFO("Adding the new rules to the internal-LSI");
			controller->installNewRules(internalLSIlowLevel.getRules());

		}
	}//end iteration on the internal endpoints of the graph
}

bool GraphManager::updateGraph(string graphID, highlevel::Graph *newGraph)
{
	/**
	*	The update of the graph is split in three parts:
	*	-	create the new VNFs, VNF ports, gre tunnels and virtual links
	*	-	deleted the proper VNFs, VNF ports, gre tunnels, rules from the LSI-0 and tenant LSI, as required by the update
	*	-	insert in the LSI-0 and tenant LSI the proper rules, as required by the update
	*	-	assigne ip address to host-stack endpoints
	*	-   update the node settings
	*/

	highlevel::Graph *diff_to_add = NULL;
	highlevel::Graph *diff_to_del = NULL;
	try
	{
		diff_to_add = updateGraph_add(graphID,newGraph);
		diff_to_del = updateGraph_remove(graphID,newGraph);
	}
	catch(GraphManagerException e)
	{
		if(diff_to_add!=NULL)
		{
			delete(diff_to_add);
			diff_to_add = NULL;
		}
		if(diff_to_del!=NULL)
		{
			delete(diff_to_del);
			diff_to_del = NULL;
		}
		return false;
	}

	if(!updateGraph_add_rules(graphID,diff_to_add))
	{
		delete(diff_to_add);
		delete(diff_to_del);
		return false;
	}

	// assigne ip address to host-stack endpoints
	LSI *lsi = tenantLSIs[graphID].getLSI();
	map<string,string> hsPortsName = lsi->getHoststackEndpointPortName();
	list<highlevel::EndPointHostStack> newHoststackEndpoints = diff_to_add->getEndPointsHostStack();
	for(list<highlevel::EndPointHostStack>::iterator hs = newHoststackEndpoints.begin(); hs!=newHoststackEndpoints.end();hs++)
	{
		if (hs->getConfiguration()==DHCP)
			interfaceManager.getIpAddressFromDhcp(hsPortsName[hs->getId()]);
		else if (hs->getConfiguration()==PPPOE)
			interfaceManager.getIpAddressFromPppoe(hsPortsName[hs->getId()]);
		else
			interfaceManager.setStaticIpAddress(hsPortsName[hs->getId()],hs->getIpAddress());

		string macAddress= hs->getMacAddress();
		if(macAddress!="")
			interfaceManager.setMacAddress(hsPortsName[hs->getId()],macAddress);
	}

	//update the node settings
#ifdef ENABLE_NODE_CONFIG
	bool setted = configurationAgent.setNodeConfiguration(newGraph);
	if (!setted)
	{
		ULOG_ERR("An error occurred setting the node configuration");
		throw GraphManagerException();
	}
#endif

	delete(diff_to_add);
	delete(diff_to_del);

	printInfo(graphLSI0lowLevel,graphInfoLSI0.getLSI());
	return true;
}

bool GraphManager::updateGraph_add_rules(string graphID, highlevel::Graph *diff)
{
	//Retrieve the information already stored for the graph
	GraphInfo graphInfo = (tenantLSIs.find(graphID))->second;
	LSI *lsi = graphInfo.getLSI();
	Controller *tenantController = graphInfo.getController();

	ULOG_DBG_INFO("Create the new rules and download them in LSI-0 and tenant-LSI");

	try
	{
		//creates the new rules for LSI-0 and for the tenant-LSI

		lowlevel::Graph graphLSI0 = GraphTranslator::lowerGraphToLSI0(diff,lsi,graphInfoLSI0.getLSI(),internalLSIsConnections);

		ULOG_DBG_INFO("New piece of graph for LSI-0:");
		graphLSI0.print();
		graphLSI0lowLevel.addRules(graphLSI0.getRules());

		lowlevel::Graph graphTenant =  GraphTranslator::lowerGraphToTenantLSI(diff,lsi,graphInfoLSI0.getLSI());
		ULOG_DBG_INFO("New piece of graph for tenant LSI:");
		graphTenant.print();

		//Insert new rules into the LSI-0
		ULOG_DBG_INFO("Adding the new rules to the LSI-0");
		(graphInfoLSI0.getController())->installNewRules(graphLSI0.getRules());

		//Insert new rules into the tenant-LSI
		ULOG_DBG_INFO("Adding the new rules to the tenant-LSI");
		tenantController->installNewRules(graphTenant.getRules());

	} catch (SwitchManagerException e)
	{
		//TODO: no idea on what I have to do at this point
		assert(0);
		delete(diff);
		diff = NULL;
		ULOG_ERR("%s",e.what());
		return false;
	}

	return true;
}

highlevel::Graph *GraphManager::updateGraph_add(string graphID, highlevel::Graph *newGraph)
{
	/**
	*	Limitations:
	*	- only new ports (and the related configuration) can be added to VNFs
	*		It is instead not possible to add new:
	*		- environment variables
	*		- control connections
	*	- internal endpoints not supported
	*	- hoststack endpoints not supported
	**/

	ULOG_INFO("Updating the graph '%s' with new 'pieces'...",graphID.c_str());

	assert(tenantLSIs.count(graphID) != 0);

	//Retrieve the information already stored for the graph (i.e., retrieve the as it is
	//currently implemented, without the update)
	GraphInfo graphInfo = (tenantLSIs.find(graphID))->second;
	ComputeController *computeController = graphInfo.getComputeController();
	highlevel::Graph *graph = graphInfo.getGraph();
	LSI *lsi = graphInfo.getLSI();

	uint64_t dpid = lsi->getDpid();

	/**
	*	Outline:
	*
	*	0) calculate the diff with respect to the graph already deployed (and check its validity)
	*	1) update the high level graph
	*	2) select an implementation for the new NFs
	*	3) update the lsi (in case of new interface/NFs/gre/vlan endpoints are required)
	*	4) start the new NFs
	*/

	/**
	*	The three following variables will be used in the following and that contain
	*	an high level graph:
	*		* graph -> the original graph to be updated
	*		* newGraph -> graph containing the update
	*		* diff -> graph that will contain the parts in newGraph that are not part of graph
	*/

	/**
	*	0) Calculate the diff ad check the validity of the update
	*/
	ULOG_DBG_INFO("0) Calculate the new parts of the graph");
	highlevel::Graph *diff = graph->calculateDiff(newGraph, graphID);

	// Save list of new network functions created
	list<string> new_network_functions;
	list<highlevel::VNFs> network_functions_diff = diff->getVNFs();
	list<highlevel::VNFs> network_functions_before = graph->getVNFs();
	for(list<highlevel::VNFs>::iterator nf_diff = network_functions_diff.begin(); nf_diff != network_functions_diff.end(); ++nf_diff)
	{
		string nf_diff_id = nf_diff->getId();
		bool find = false;
		for(list<highlevel::VNFs>::iterator nf_before = network_functions_before.begin(); nf_before != network_functions_before.end(); ++nf_before)
		{
			string nf_before_id = nf_before->getId();
			if(nf_diff_id==nf_before_id) // this is not a new Network Function!
			{
				find = true;
				break;
			}
		}
		if(!find)
			new_network_functions.push_back(nf_diff_id);
	}

	ULOG_DBG_INFO("The diff graph is:");
	diff->print();

	if(!checkGraphValidity(diff,computeController))
	{
		//This is an error in the request
		delete(diff);
		diff = NULL;
		throw GraphManagerException();
	}
	//The required graph update is valid

	/**
	*	1) Update the high level graph
	*/
	ULOG_DBG_INFO("1) Update the high level graph");

	graph->addGraphToGraph(diff);
	ULOG_DBG_INFO("The deployed graph + the new new parts is:");
	graph->print();

	/**
	*	2) Select an implementation for the new NFs
	*/
	ULOG_DBG_INFO("2) Select an implementation for the new NFs");
	if(!computeController->selectImplementation())
	{
		//This is an internal error
		delete(computeController);
		computeController = NULL;
		throw GraphManagerException();
	}

	/**
	*	3) Update the lsi (in case of new interface/VNFs/gre/vlan endpoints are required)
	*/
	ULOG_DBG_INFO("3) update the lsi (in case of new ports/VNFs/gre/vlan endpoints are required)");

	/**
	*	3-a) condider the virtual links
	*/
	ULOG_DBG_INFO("3-a) considering the new virtual links");

	//Since the NFs cannot specify new ports, new virtual links can be required only by the new NFs and the physical ports

	vector<set<string> > vlVector = identifyVirtualLinksRequired(diff,lsi);
	set<string> vlNFs = vlVector[0];
	set<string> vlPhyPorts = vlVector[1];
	set<string> vlEndPointsGre = vlVector[2];
	set<string> vlEndPointsInternal = vlVector[3];
	set<string> vlEndPointsHoststack = vlVector[4];

	//TODO: check if a virtual link is already available and can be used (because it is currently used only in one direction)
	//FIXME: I think the part that calculates the number of vlink required should be rewritten. It should be based on the code of new graph, which is very optimized
	unsigned int numberOfVLrequiredBeforeEndPoints = (vlNFs.size() > vlPhyPorts.size())? vlNFs.size() : vlPhyPorts.size(); //The same virtual link can be exploited both towards VNFs and towards physical ports
	unsigned int numberOfVLrequired = numberOfVLrequiredBeforeEndPoints + vlEndPointsInternal.size()  + vlEndPointsGre.size(); //TODO: optimize this; in fact, the same virtual link could be exploited both towards GRE endpoints and towards internal endpoints

	ULOG_DBG_INFO("%d virtual links are required to connect the new part of the LSI with LSI-0",numberOfVLrequired);

	set<string>::iterator nf = vlNFs.begin();
	set<string>::iterator p = vlPhyPorts.begin();
	for(; nf != vlNFs.end() || p != vlPhyPorts.end() ;)
	{
		//FIXME: here I am referring to a vlink through its position. It would be really better to use its ID
		AddVirtualLinkOut *avlo = NULL;
		try
		{
			ULOG_DBG_INFO(" Adding vlink required to a interface endpoint (i.e., physical port) and/or to a VNF port");
			VLink newLink(dpid0);
			int vlinkPosition = lsi->addVlink(newLink);

			AddVirtualLinkIn avli(dpid,dpid0);
			avlo = switchManager.addVirtualLink(avli);

			assert(avlo != NULL);
			lsi->setVLinkIDs(vlinkPosition,avlo->getIdA(),avlo->getIdB());

			delete(avlo);

			uint64_t vlinkID = newLink.getID();

			VLink vlink = lsi->getVirtualLink(vlinkID); //FIXME: vlink is the same of newLink
			ULOG_DBG_INFO("Virtual link: (ID: %x) %x:%d -> %x:%d",vlink.getID(),dpid,vlink.getLocalID(),vlink.getRemoteDpid(),vlink.getRemoteID());
			assert(vlinkID == vlink.getID());

			if(nf != vlNFs.end())
			{
				lsi->addNFvlink(*nf,vlinkID);
				ULOG_DBG_INFO("NF '%s' uses the vlink '%x'",(*nf).c_str(),vlink.getID());
				nf++;
			}
			if(p != vlPhyPorts.end())
			{
				lsi->addPortvlink(*p,vlinkID);
				ULOG_DBG_INFO("Physical port '%s' uses the vlink '%x'",(*p).c_str(),vlink.getID());
				p++;
			}
		}catch(SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			if(avlo != NULL)
				delete(avlo);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
	}

	for(set<string>::iterator ep = vlEndPointsGre.begin(); ep != vlEndPointsGre.end(); ep++)
	{
		//FIXME: here I am referring to a vlink through its position. It would be really better to use its ID
		AddVirtualLinkOut *avlo = NULL;
		try
		{
			ULOG_DBG_INFO(" Adding vlink required to a gre-tunnel endpoint");

			VLink newLink(dpid0);
			int vlinkPosition = lsi->addVlink(newLink);

			AddVirtualLinkIn avli(dpid,dpid0);
			avlo = switchManager.addVirtualLink(avli);

			lsi->setVLinkIDs(vlinkPosition,avlo->getIdA(),avlo->getIdB());

			delete(avlo);

			uint64_t vlinkID = newLink.getID();

			VLink vlink = lsi->getVirtualLink(vlinkID);
			ULOG_DBG_INFO("Virtual link: (ID: %x) %x:%d -> %x:%d",vlink.getID(),dpid,vlink.getLocalID(),vlink.getRemoteDpid(),vlink.getRemoteID());
			assert(vlinkID == vlink.getID());

			lsi->addEndpointGrevlink(*ep,vlinkID);
			ULOG_DBG_INFO("Gre endpoint '%s' uses the vlink '%x'",(*ep).c_str(),vlink.getID());
		}catch(SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			if(avlo != NULL)
				delete(avlo);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
	}

	for(set<string>::iterator ep = vlEndPointsHoststack.begin(); ep != vlEndPointsHoststack.end(); ep++)
	{
		//FIXME: here I am referring to a vlink through its position. It would be really better to use its ID
		AddVirtualLinkOut *avlo = NULL;
		try
		{
			ULOG_DBG_INFO(" Adding vlink required to a hoststack endpoint");

			VLink newLink(dpid0);
			int vlinkPosition = lsi->addVlink(newLink);

			AddVirtualLinkIn avli(dpid,dpid0);
			avlo = switchManager.addVirtualLink(avli);

			lsi->setVLinkIDs(vlinkPosition,avlo->getIdA(),avlo->getIdB());

			delete(avlo);

			uint64_t vlinkID = newLink.getID();

			VLink vlink = lsi->getVirtualLink(vlinkID);
			ULOG_DBG_INFO("Virtual link: (ID: %x) %x:%d -> %x:%d",vlink.getID(),dpid,vlink.getLocalID(),vlink.getRemoteDpid(),vlink.getRemoteID());
			assert(vlinkID == vlink.getID());

			lsi->addEndpointHoststackvlink(*ep,vlinkID);
			ULOG_DBG_INFO("Hoststack endpoint '%s' uses the vlink '%x'",(*ep).c_str(),vlink.getID());
		}catch(SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			if(avlo != NULL)
				delete(avlo);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
	}

	for(set<string>::iterator ep = vlEndPointsInternal.begin(); ep != vlEndPointsInternal.end(); ep++)
	{
		//FIXME: here I am referring to a vlink through its position. It would be really better to use its ID
		AddVirtualLinkOut *avlo = NULL;
		try
		{
			ULOG_DBG_INFO(" Adding vlink required to an internal endpoint");

			VLink newLink(dpid0);
			int vlinkPosition = lsi->addVlink(newLink);

			AddVirtualLinkIn avli(dpid,dpid0);
			avlo = switchManager.addVirtualLink(avli);

			lsi->setVLinkIDs(vlinkPosition,avlo->getIdA(),avlo->getIdB());

			delete(avlo);

			uint64_t vlinkID = newLink.getID();

			VLink vlink = lsi->getVirtualLink(vlinkID);
			ULOG_DBG_INFO("Virtual link: (ID: %x) %x:%d -> %x:%d",vlink.getID(),dpid,vlink.getLocalID(),vlink.getRemoteDpid(),vlink.getRemoteID());

			lsi->addEndpointInternalvlink(*ep,vlinkID);
			ULOG_DBG_INFO("Internal endpoint '%s' uses the vlink '%x'",(*ep).c_str(),vlink.getID());

		}catch(SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			if(avlo != NULL)
				delete(avlo);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
	}

	/**
	*	3-b) condider the virtual network functions
	*/
	ULOG_DBG_INFO("3-b) considering the new virtual network functions (%d)",network_functions_diff.size());

	//Itarate on all the new network functions
	for(list<highlevel::VNFs>::iterator nf = network_functions_diff.begin(); nf != network_functions_diff.end(); nf++)
	{
		AddNFportsOut *anpo = NULL;
		try
		{
			string nf_id = nf->getId();
			list<highlevel::vnf_port_t> nf_ports = nf->getPorts(); // nf_it->second;
			list<unsigned int> nf_ports_id_list = nf->getPortsId();

			//before
			map<string, list<struct nf_port_info> >pi_map_before = lsi->getNetworkFunctionsPortsInfo();
			map<string, list<struct nf_port_info> >::iterator pi_it_before = pi_map_before.find(nf->getId()); //pi_it_before==pi_map.end() in case of a new NF

			lsi->addNF(nf->getId()/*first*/, /*nf->second*/ nf_ports_id_list, computeController->getNFSelectedImplementation(nf->getId()/*first*/)->getPortTechnologies());

			//after
			map<string, list<struct nf_port_info> >pi_map = lsi->getNetworkFunctionsPortsInfo();//for each network function, retrieve a list of "port name, port type"
			map<string, list<struct nf_port_info> >::iterator pi_it = pi_map.find(nf->getId()/*first*/); //select the info related to the network function currently considered

			assert(pi_it != pi_map.end());
			list<struct nf_port_info> newPortList;
			bool is_new_nf=false;
			for(list<string>::iterator new_nf = new_network_functions.begin(); new_nf != new_network_functions.end(); ++new_nf)
				if(nf_id==*new_nf)
				{
					is_new_nf=true;
					break;
				}
			if(is_new_nf)
			{
				ULOG_INFO("Adding new ports on the switch to create the NF with id %s", nf->getId().c_str());
				newPortList = pi_it->second;
			}
			else
			{
				ULOG_INFO("Adding new ports on the switch to update the NF with id %s", nf->getId().c_str());
				for(list<struct nf_port_info>::iterator port_it = pi_it->second.begin(); port_it != pi_it->second.end(); ++port_it)
				{
					struct nf_port_info port_id = (*port_it);
					bool find = false;
					for(list<struct nf_port_info>::iterator lsiPort_it = pi_it_before->second.begin(); lsiPort_it != pi_it_before->second.end(); ++lsiPort_it)
					{
						struct nf_port_info portData = (*lsiPort_it);
						if(portData.port_name.compare(port_id.port_name) == 0)
						{
							find = true;
							break;
						}
					}
					if(!find)
						newPortList.push_back(port_id);
				}
			}
			AddNFportsIn anpi(dpid, nf->getId(), computeController->getNFType(nf->getId()), newPortList); //prepare the input for the switch manager
			//We add, with a single call, all the ports of a single network function
			anpo = switchManager.addNFPorts(anpi);

			//anpo->getPorts() returns the map "ports name, identifier within the lsi"
			if(!lsi->setNfSwitchPortsID(anpo->getNfId(), anpo->getPorts()))
			{
				ULOG_ERR("A non-required network function port related to the network function with id \"%s\" has been attached to the tenant-lsi",nf->getId().c_str()/*first.c_str()*/);
				lsi->removeNF(nf->getId()/*first*/);
				delete(anpo);
				throw GraphManagerException();
			}

			//FIXME: useful? Probably no!
			//map such names with ports names calculated before (or with the port id).
			//I think that it should be done something similar to anpo->getPorts(), which maps the identifier on the switch to the port name.
			uint64_t lsiID = lsi->getDpid();
			map<string, unsigned int> newPortsNamesAndID;
			for(list<struct nf_port_info>::iterator port_it = newPortList.begin(); port_it != newPortList.end(); port_it++)
			{
				stringstream ss;
				ss << lsiID << "_" << port_it->port_name;
				newPortsNamesAndID[ss.str()] = port_it->port_id;
			}
			lsi->setNetworkFunctionsPortsNameOnSwitch(anpo->getNfId(), newPortsNamesAndID);


			delete(anpo);
		}catch(SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			lsi->removeNF(nf->getId()/*first*/);
			if(anpo != NULL)
				delete(anpo);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
	}

	/**
	*	3-c) condider the gre-tunnel endpoints
	*/
	list<highlevel::EndPointGre> tmp_gre_endpoints = diff->getEndPointsGre();
	ULOG_DBG_INFO("3-c) considering the new gre-tunnel endpoints (%d)",tmp_gre_endpoints.size());

	for(list<highlevel::EndPointGre>::iterator ep = tmp_gre_endpoints.begin(); ep != tmp_gre_endpoints.end(); ep++)
	{
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
		//fill the vector related to the endpoint params [gre key, local-ip, remote-ip, interface, isSafe]
		vector<string> ep_param(4);
		ep_param[0] = ep->getLocalIp();
		ep_param[1] = ep->getGreKey();
		ep_param[2] = ep->getRemoteIp();
		if(ep->isSafe())
			ep_param[3] = "true";
		else
			ep_param[3] = "false";

		ULOG_DBG_INFO("\t%s",(ep->getId()).c_str());

		AddEndpointOut *aepo = NULL;
		try
		{
			lsi->addGreEndpoint(*ep);
			AddEndpointIn aepi(dpid,ep->getId(),ep_param);

			aepo = switchManager.addGreEndpoint(aepi);

			if(!lsi->setGreEndpointPortID(aepo->getEpId(), aepo->getOfId()))
			{
				ULOG_ERR("A non-required gre endpoint \"%s\" has been attached to the tenant-lsi",ep->getId().c_str());
				lsi->removeGreEndpoint(ep->getId());
				delete(aepo);
				throw GraphManagerException();
			}

			delete(aepo);
		}catch(SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			lsi->removeGreEndpoint(ep->getId());
			if(aepo != NULL)
				delete(aepo);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
#else
		ULOG_ERR("GRE tunnel unavailable with the selected virtual switch");
		throw GraphManagerException();
#endif
	}

	/**
	*	3-d) condider the internal endpoints
	*/
	ULOG_DBG_INFO("3-d) considering the new internal endpoints (%d)",diff->getEndPointsInternal().size());
	handleControllerForInternalEndpoint(diff);
	handleGraphForInternalEndpoint(diff);


	/**
	*	3-e) condider the hoststack endpoints
	*/
	list<highlevel::EndPointHostStack> tmp_hoststack_endpoint = diff->getEndPointsHostStack();
	ULOG_DBG_INFO("3-d) considering the new hoststack endpoints (%d)",tmp_hoststack_endpoint.size());

	for(list<highlevel::EndPointHostStack>::iterator ep = tmp_hoststack_endpoint.begin(); ep != tmp_hoststack_endpoint.end(); ep++)
	{
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
		AddEndpointHoststackOut *aepo = NULL;
		try
		{
			lsi->addHoststackEndpoint(*ep);
			AddEndpointHoststackIn aepi(dpid,ep->getId());

			aepo = switchManager.addHoststackEndpoint(aepi);

			if(!lsi->setHoststackEndpointPortID(aepo->getEpId(), aepo->getOfId()))
			{
				ULOG_ERR("A non-required hoststack endpoint \"%s\" has been attached to the tenant-lsi",ep->getId().c_str());
				// TODO: lsi->removeHoststackEndpoint(ep->getId());
				assert(0);
				delete(aepo);
				throw GraphManagerException();
			}
			lsi->setHoststackEndpointPortName(aepo->getEpId(), aepo->getEpPortName());
			delete(aepo);
		}catch(SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			// TODO: lsi->removeHoststackEndpoint(ep->getId());
			if(aepo != NULL)
				delete(aepo);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
#else
		ULOG_ERR("Hoststack endpoints unavailable with the selected virtual switch");
		throw GraphManagerException();
#endif
	}

	/**
	*	4) Start or update the new NFs
	*/
#ifdef RUN_NFS
	ULOG_DBG_INFO("4) start or update the new NFs");

	computeController->setLsiID(dpid);


#ifndef STARTVNF_SINGLE_THREAD
	pthread_t some_thread[network_functions_diff.size()];
#endif
	to_thread_t thr[network_functions_diff.size()];
	int i = 0;

	for(list<highlevel::VNFs>::iterator nf = network_functions_diff.begin(); nf != network_functions_diff.end(); nf++)
	{
		string nf_id=nf->getId();
		map<unsigned int, string> nfPortIdToNameOnSwitch = lsi->getNetworkFunctionsPortsNameOnSwitchMap(nf->getId()/*first*/); //Returns the map <port ID, port name on switch>
		//TODO: the following information should be retrieved through the highlevel graph
		map<unsigned int, port_network_config_t > nfs_ports_configuration = nf->getPortsID_configuration();
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		list<port_mapping_t > nfs_control_configuration = nf->getControlPorts();
		list<string> environment_variables_tmp = nf->getEnvironmentVariables();
#endif
		bool is_new_nf=false;
		for(list<string>::iterator new_nf = new_network_functions.begin(); new_nf != new_network_functions.end(); ++new_nf)
			if(nf_id==*new_nf)
			{
				is_new_nf=true;
				break;
			}
		if(!is_new_nf)
		{
			if(!computeController->updateNF(nf->getId(), nfPortIdToNameOnSwitch, nfs_ports_configuration, nf->getPortsId()))
			{
				//TODO: no idea on what I have to do at this point
				assert(0);
				delete(diff);
				diff = NULL;
				throw GraphManagerException();
			}
		}
#if 0
		else if(!computeController->startNF(nf->getId()/*first*/, nfPortIdToNameOnSwitch, nfs_ports_configuration
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
			, nfs_control_configuration, environment_variables_tmp
#endif
		))
		{
			//TODO: no idea on what I have to do at this point
			assert(0);
			delete(diff);
			diff = NULL;
			throw GraphManagerException();
		}
#endif
		else
		{
			thr[i].nf_id = nf->getId();
			thr[i].computeController = computeController;
			thr[i].namesOfPortsOnTheSwitch = lsi->getNetworkFunctionsPortsNameOnSwitchMap(nf->getId());
			thr[i].portsConfiguration = nf->getPortsID_configuration();
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
			thr[i].controlConfiguration = nf->getControlPorts();
			thr[i].environmentVariables = nf->getEnvironmentVariables();
#endif

#ifdef STARTVNF_SINGLE_THREAD
			startNF((void *) &thr[i]);
#else
			if (pthread_create(&some_thread[i], NULL, &startNF, (void *)&thr[i]) != 0)
			{
				assert(0);
				delete(diff);
				diff = NULL;
				ULOG_ERR("An error occurred while creating a new thread");
				throw GraphManagerException();
			}
#endif
			i++;
		}
	} // end iteration on network functions
	bool ok = true;
#ifndef STARTVNF_SINGLE_THREAD
	for(int j = 0; j < i;j++)
	{
		void *returnValue;
		pthread_join(some_thread[j], &returnValue);
		int *c = (int*)returnValue;

		if(c == 0)
			ok = false;
	}
#endif
	if(!ok)
	{
		for(list<highlevel::VNFs>::iterator nf = network_functions_diff.begin(); nf != network_functions_diff.end(); nf++)
			computeController->stopNF(nf->getId());

		switchManager.destroyLsi(lsi->getDpid());

/*		delete(graph);
		delete(lsi);
		delete(computeController);
		delete(controller);

		graph = NULL;;
		lsi = NULL;
		computeController = NULL;
		controller = NULL;*/
		delete(diff);
		diff = NULL;
		throw GraphManagerException();
	}
#else
	ULOG_DBG_INFO("4) Flag RUN_NFS disabled. New NFs will not start");
#endif //end ifdef RUN_NFS

	//The new endpoints, VNFs, VNF ports and virtual links have been added to the graph!

	return diff;
}

highlevel::Graph *GraphManager::updateGraph_remove(string graphID, highlevel::Graph *newGraph)
{
	ULOG_INFO("Updating the graph '%s' by removing parts...",graphID.c_str());

	assert(tenantLSIs.count(graphID) != 0);

	//Retrieve the information already stored for the graph (i.e., retrieve the as it is
	//currently implemented, without the update)
	GraphInfo graphInfo = (tenantLSIs.find(graphID))->second;
	highlevel::Graph *graph = graphInfo.getGraph();
	LSI *lsi = graphInfo.getLSI();

	/**
	*	Outline:
	*
	*	0) calculate the diff with respect to the graph already deployed (and check its validity)
	*	1) update the high level graph
	*	2) remove the rules from LSI-0 and tenant-LSI
	*	3) remove the virtual links that are no longer used
	*	4) delete the gre-tunnel endpoints from the LSI, if required by the update
	*	5) delete the VNFs and the related ports on the LSI, if required by the update
	*/

	/**
	*	Limitations:
	*	- only ports (and the related configuration) can be removed from VNFs
	*		It is instead not possible to remove:
	*		- environment variables
	*		- control connections
	*	- internal endpoints not supported
	*	- hoststack endpoints not supported
	**/

	/**
	*	The three following variables will be used in the following and that contain
	*	an high level graph:
	*		* graph -> the original graph to be updated
	*		* newGraph -> graph containing the update
	*		* diff -> graph that will contain the parts in "graph" and that are not in "newGraph",
	*				and then that must be removed from "newGraph"
	*/

	/**
	*	0) Calculate the diff
	*/
	ULOG_DBG_INFO("0) Calculate the parts to be removed from the graph");
	highlevel::Graph *diff = newGraph->calculateDiff(graph, graphID);

	ULOG_DBG_INFO("The diff graph is:");
	diff->print();

	/**
	*	1) Update the high level graph
	*/
	ULOG_DBG_INFO("1) Update the high level graph");

	list<RuleRemovedInfo> removeRuleInfo = graph->removeGraphFromGraph(diff);
	ULOG_DBG_INFO("The final graph is:");
	graph->print();

	/**
	*	The following things must be done
	*	- 2) delete the flowrules
	*	- 3) delete the virtual links no longer used. We support virtual link related to each type of endpoint
	*	- 4) delete the gre-tunnel and hoststack ports
	*	- 5) delete the interface endpoints (i.e., physical ports)
	*	- 6) delete the VNFs
	*/

	/**
	*	2) Remove the rules from LSI-0 and tenant-LSI
	*/

	ULOG_DBG_INFO("2) Removing the flow rules");
	Controller *lsi0Controller = graphInfoLSI0.getController();
/* vecchia
	list<highlevel::Rule> rulesToBeRemoved = diff->getRules();
	for(list<highlevel::Rule>::iterator rule = rulesToBeRemoved.begin(); rule != rulesToBeRemoved.end(); rule++)
	{
		string ruleID = rule->getRuleID();
		ULOG_DBG_INFO("Removing the flow rule with id '%s'",ruleID.c_str());

		// Considering the LSI-0
		stringstream lsi0RuleID;
		lsi0RuleID << graph->getID() << "_" << ruleID;
		lsi0Controller->removeRuleFromID(lsi0RuleID.str());
		graphLSI0lowLevel.removeRuleFromID(lsi0RuleID.str());

		// Considering the tenant-LSI
		ULOG_DBG_INFO("Removing the flow from the tenant-LSI graph");
		Controller *tenantController = graphInfo.getController();
		tenantController->removeRuleFromID(ruleID);
	}*/

	list<highlevel::Rule> rulesToBeRemoved = diff->getRules();
	for(list<highlevel::Rule>::iterator rule = rulesToBeRemoved.begin(); rule != rulesToBeRemoved.end(); rule++)
	{
		string generalRuleID = rule->getRuleID();
		ULOG_DBG_INFO("Removing the flow rule with id '%s'",generalRuleID.c_str());

		// Considering the LSI-0
		list<string> rulesIDLSI0 = getRulesIDForLSI0(*rule,graph->getID());
		for(list<string>::iterator ruleID = rulesIDLSI0.begin(); ruleID!=rulesIDLSI0.end(); ruleID++)
		{
			lsi0Controller->removeRuleFromID(*ruleID);
			graphLSI0lowLevel.removeRuleFromID(*ruleID);
		}

		// Considering the tenant-LSI
		ULOG_DBG_INFO("Removing the flow from the tenant-LSI graph");
		Controller *tenantController = graphInfo.getController();
		list<string> rulesIDLSITenant = getRulesIDForLSITenant(*rule);
		for(list<string>::iterator ruleID = rulesIDLSITenant.begin(); ruleID !=rulesIDLSITenant.end(); ruleID++)
			tenantController->removeRuleFromID(*ruleID);

	}

	/**
	*	3)	Remove the virtual links that are no longer used.
	*		The fact that such endpoints (of whatever type) are removed or not from the graph is not important; in fact, the vlink are created/destroyed depending on the rules.
	*/

	ULOG_DBG_INFO("3) Removing the virtual links");

	assert(removeRuleInfo.size() >= rulesToBeRemoved.size());
	for(list<RuleRemovedInfo>::iterator tbr = removeRuleInfo.begin(); tbr != removeRuleInfo.end(); tbr++)
		removeUselessVlinks(*tbr,graph,lsi);

	/**
	*	4a) Delete the gre-tunnel endpoints, as required by the diff
	*/

	ULOG_DBG_INFO("4a) Removing the gre-tunnel endpoints");

	list<highlevel::EndPointGre> greEndpoints = diff->getEndPointsGre();
	for(list<highlevel::EndPointGre>::iterator gep = greEndpoints.begin(); gep != greEndpoints.end(); gep++)
	{
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
		ULOG_DBG_INFO("The gre endpoint '%s' is no longer part of the graph",(gep->getId()).c_str());

		try
		{
			DestroyEndpointIn depi(lsi->getDpid(),gep->getId());
			switchManager.destroyEndpoint(depi);
			lsi->removeGreEndpoint((gep->getId()));
		} catch (SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			throw GraphManagerException();
		}
#else
		ULOG_WARN("GRE tunnel not supported with the selected virtual switch");
#endif
	}

	/**
	*	4b) Delete the hoststack endpoints, as required by the diff
	*/

	ULOG_DBG_INFO("4b) Removing the hoststack endpoints");

	list<highlevel::EndPointHostStack> hoststackEndpoints = diff->getEndPointsHostStack();
	for(list<highlevel::EndPointHostStack>::iterator hsep = hoststackEndpoints.begin(); hsep != hoststackEndpoints.end(); hsep++)
	{
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
		ULOG_DBG_INFO("The hoststack '%s' is no longer part of the graph",(hsep->getId()).c_str());

		try
		{
			DestroyHoststackEndpointIn dhepi(lsi->getDpid(),hsep->getId());
			switchManager.destroyHoststackEndpoint(dhepi);
			lsi->removeHoststackEndpoint((hsep->getId()));
		} catch (SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			throw GraphManagerException();
		}
#else
		ULOG_WARN("Hostack endpoints not supported with the selected virtual switch");
#endif
	}

	/**
	*	5) Delete the VNFs and the related ports on the LSI, as required by the diff
	*/

	ULOG_DBG_INFO("5) Removing the VNFs and the related ports on the LSI");

#ifdef RUN_NFS
	ComputeController *computeController = graphInfo.getComputeController();
#endif
	list<highlevel::VNFs> vnfs = diff->getVNFs();
	for(list<highlevel::VNFs>::iterator vnf = vnfs.begin(); vnf != vnfs.end(); vnf++)
	{

		ULOG_DBG_INFO("The VNF with id '%s' is no longer part of the graph",(vnf->getId()).c_str());

		//Stop the NF
#ifdef RUN_NFS
		computeController->stopNF(vnf->getId());
#else
		ULOG_DBG_INFO("Flag RUN_NFS disabled. No NF to be stopped");
#endif

		set<string> nf_ports;
		map<unsigned int, string>lsi_nf_ports = lsi->getNetworkFunctionsPortsNameOnSwitchMap(vnf->getId());
		for (map<unsigned int,string>::iterator lsi_nfp_it = lsi_nf_ports.begin(); lsi_nfp_it != lsi_nf_ports.end(); ++lsi_nfp_it) {
			nf_ports.insert(lsi_nfp_it->second);
		}
		try
		{
			DestroyNFportsIn dnpi(lsi->getDpid(), vnf->getId(), nf_ports);
			switchManager.destroyNFPorts(dnpi);
			lsi->removeNF(vnf->getId());
		} catch (SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			throw GraphManagerException();
		}
	}

	//TODO: , phy ports- add an error in case internal endpoints have to be removed

	return diff;
}

void GraphManager::removeUselessVlinks(RuleRemovedInfo rri, highlevel::Graph *graph, LSI *lsi)
{
	// TODO: Deal also with hoststack!
	map<string, uint64_t> nfs_vlinks = lsi->getNFsVlinks();
	map<string, uint64_t> ports_vlinks = lsi->getPortsVlinks();
	map<string, uint64_t> endpoints_internal_vlinks = lsi->getEndPointsVlinks();
	map<string, uint64_t> endpoints_gre_vlinks = lsi->getEndPointsGreVlinks();

	list<highlevel::Rule> rules = graph->getRules();

	/**
	*	Consider the network function ports
	*/

	ULOG_DBG_INFO("Checking if some virtual links can be removed...");
	if(rri.isNFport)
		ULOG_DBG_INFO("Check if the vlink associated with the NF port '%s' must be removed (if this vlink exists)",rri.nf_port.c_str());

	if(rri.isNFport && nfs_vlinks.count(rri.nf_port) != 0)
	{
		ULOG_DBG_INFO("The NF port '%s' is associated with a vlink",rri.nf_port.c_str());

		/**
		*	In case NF:port does not appear in other actions, and it is not use for any physical port or internal endpoint, then the vlink must be removed
		*/
		for(list<highlevel::Rule>::iterator again = rules.begin(); again != rules.end(); again++)
		{
			highlevel::Action *a = again->getAction();
			highlevel::Match m = again->getMatch();

			list<OutputAction*> outputActions = a->getOutputActions();
			for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
			{
				if((*outputAction)->getType() == ACTION_ON_NETWORK_FUNCTION && (m.matchOnEndPointInternal() || m.matchOnPort() ))
				{
					//The network function port can still be used in an action, but the match should be on something not requiring the vlink: another VNF port or a gre tunnel

					stringstream nf_port;
					nf_port << ((ActionNetworkFunction*)(*outputAction))->getInfo() << "_" << ((ActionNetworkFunction*)(*outputAction))->getPort();
					string nf_port_string = nf_port.str();

					if(nf_port_string == rri.nf_port)
					{
						//The action is on the same VNF port of the removed one, hence the vlink must not be removed
						ULOG_DBG_INFO("The vlink cannot be removed, since there are other actions expressed on the NF port '%s'",rri.nf_port.c_str());
						return;
					}
				}
			}
		}//end of again iterator on the rules of the graph

		//We just know that the vlink is no longer used for a NF port. However, it might be used in the opposite
		//direction: for a physical port or for an internal endpoint

		ULOG_DBG_INFO("Virtual link no longer required for NF port: %s",rri.nf_port.c_str());
		uint64_t tobeRemovedID = nfs_vlinks.find(rri.nf_port)->second;

		//The virtual link is no longer associated with the network function port
		lsi->removeNFvlink(rri.nf_port);

		for(map<string, uint64_t>::iterator pvl = ports_vlinks.begin(); pvl != ports_vlinks.end(); pvl++)
		{
			if(pvl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the interface: %s",pvl->first.c_str());
				return;
			}
		}

		for(map<string, uint64_t>::iterator ivl = endpoints_internal_vlinks.begin(); ivl != endpoints_internal_vlinks.end(); ivl++)
		{
			if(ivl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the internal endpoint: %s",ivl->first.c_str());
				return;
			}
		}

		ULOG_DBG_INFO("The virtual link must be removed");

		try
		{
			VLink toBeRemoved = lsi->getVirtualLink(tobeRemovedID);
			DestroyVirtualLinkIn dvli(lsi->getDpid(), toBeRemoved.getLocalID(), toBeRemoved.getRemoteDpid(), toBeRemoved.getRemoteID());
			switchManager.destroyVirtualLink(dvli);
			lsi->removeVlink(tobeRemovedID);
		} catch (SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			throw GraphManagerException();
		}
		return;
	}//end if(rri.isNFport && nfs_vlinks.count(rri.nf_port) != 0)

	/**
	*	Consider the gre-tunnel endpoints
	*/

	if(rri.isEndpointGre)
		ULOG_DBG_INFO("Check if the vlink associated with the gre endpoint '%s' must be removed (if this vlink exists)",rri.endpointGre.c_str());

	if(rri.isEndpointGre && endpoints_gre_vlinks.count(rri.endpointGre) != 0)
	{
		ULOG_DBG_INFO("The gre endpoint '%s' is associated with a vlink",rri.endpointGre.c_str());

		/**
		*	In case the gre endpoint does not appear in other actions, and it is not use for any physical port or internal endpoint, then the vlink must be removed
		*/
		for(list<highlevel::Rule>::iterator again = rules.begin(); again != rules.end(); again++)
		{
			highlevel::Action *a = again->getAction();
			highlevel::Match m = again->getMatch();

			list<OutputAction*> outputActions = a->getOutputActions();
			for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
			{
				if((*outputAction)->getType() == ACTION_ON_ENDPOINT_GRE && (m.matchOnEndPointInternal() || m.matchOnPort()))
				{
					if(((ActionEndpointGre*)(*outputAction))->toString() == rri.endpointGre)
					{
						//The action is on the same gre endpoint of the removed one, hence the vlink must not be removed
						return;
					}
				}
			}
		}//end of again iterator on the rules of the graph

		ULOG_DBG_INFO("Virtual link no longer required for the gre endpoint: %s",rri.endpointGre.c_str());

		uint64_t tobeRemovedID = endpoints_gre_vlinks.find(rri.endpointGre)->second;
		lsi->removeEndPointGrevlink(rri.endpointGre);

		for(map<string, uint64_t>::iterator pvl = ports_vlinks.begin(); pvl != ports_vlinks.end(); pvl++)
		{
			if(pvl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the port: %s",pvl->first.c_str());
				return;
			}
		}

		for(map<string, uint64_t>::iterator ivl = endpoints_internal_vlinks.begin(); ivl != endpoints_internal_vlinks.end(); ivl++)
		{
			if(ivl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the internal endpoint: %s",ivl->first.c_str());
				return;
			}
		}

		ULOG_DBG_INFO("The virtual link must be removed");

		try
		{
			VLink toBeRemoved = lsi->getVirtualLink(tobeRemovedID);
			DestroyVirtualLinkIn dvli(lsi->getDpid(), toBeRemoved.getLocalID(), toBeRemoved.getRemoteDpid(), toBeRemoved.getRemoteID());
			switchManager.destroyVirtualLink(dvli);
			lsi->removeVlink(tobeRemovedID);
		} catch (SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			throw GraphManagerException();
		}
		return;
	}//end if(rri.isEndpointGre && endpoints_gre_vlinks.count(rri.endpointGre) != 0)

	/**
	*	Consider the interface endpoints (i.e., physical ports)
	*/

	if(rri.isPort)
		ULOG_DBG_INFO("Check of the vlink associated with the port '%s' must be removed (if this vlink exists)",rri.port.c_str());

	if(rri.isPort && ports_vlinks.count(rri.port) != 0)
	{
		ULOG_DBG_INFO("The port '%s' is associated with a vlink",rri.port.c_str());
		/**
		*	In case port does not appear in other actions, and it is not use for any gre-tunnel or VNF port, then the vlink must be removed
		*/
		for(list<highlevel::Rule>::iterator again = rules.begin(); again != rules.end(); again++)
		{
			highlevel::Action *a = again->getAction();
			highlevel::Match m = again->getMatch();

			list<OutputAction*> outputActions = a->getOutputActions();
			for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
			{
				if((*outputAction)->getType() == ACTION_ON_PORT && (m.matchOnEndPointGre() || m.matchOnNF() || m.matchOnEndPointHoststack()))
				{
					if(((ActionPort*)(*outputAction))->getInfo() == rri.port)
					{
						//The action are the same, hence no vlink must be removed
						return;
					}
				}
			}

		}//end of again iterator on the rules of the graph

		//We just know that the vlink is no longer used for a port. However, it might used in the opposite
		//direction, for a NF port or for a gre-tunnel endpoint
		ULOG_DBG_INFO("Virtual link no longer required for port: %s",rri.port.c_str());
		uint64_t tobeRemovedID = ports_vlinks.find(rri.port)->second;

		lsi->removePortvlink(rri.port);

		for(map<string, uint64_t>::iterator nfvl = nfs_vlinks.begin(); nfvl != nfs_vlinks.end(); nfvl++)
		{
			if(nfvl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the NF port: %s",nfvl->first.c_str());
				return;
			}
		}

		for(map<string, uint64_t>::iterator gvl = endpoints_gre_vlinks.begin(); gvl != endpoints_gre_vlinks.end(); gvl++)
		{
			if(gvl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the gre-tunnel endpoint: %s",gvl->first.c_str());
				return;
			}
		}

		ULOG_DBG_INFO("The virtual link must be removed");
		try
		{
			VLink toBeRemoved = lsi->getVirtualLink(tobeRemovedID);
			DestroyVirtualLinkIn dvli(lsi->getDpid(), toBeRemoved.getLocalID(), toBeRemoved.getRemoteDpid(), toBeRemoved.getRemoteID());
			switchManager.destroyVirtualLink(dvli);
			lsi->removeVlink(tobeRemovedID);
		} catch (SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			throw GraphManagerException();
		}
	}

	/**
	*	Consider the internal endpoints
	*/

	if(rri.isEndpointInternal)
		ULOG_DBG_INFO("Check if the vlink associated with the internal endpoint '%s' must be removed (if this vlink exists)",rri.endpointInternal.c_str());

	if(rri.isEndpointInternal && endpoints_internal_vlinks.count(rri.endpointInternal) != 0)
	{
		ULOG_DBG_INFO("The internal endpoint '%s' is associated with a vlink",rri.endpointInternal.c_str());

		/**
		*	In case the internal endpoint does not appear in other actions, and it is not use for any gre-tunnel or VNF port, then the vlink must be removed
		*/
		for(list<highlevel::Rule>::iterator again = rules.begin(); again != rules.end(); again++)
		{
			highlevel::Action *a = again->getAction();
			highlevel::Match m = again->getMatch();

			list<OutputAction*> outputActions = a->getOutputActions();
			for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
			{
				if((*outputAction)->getType() == ACTION_ON_ENDPOINT_INTERNAL  && (m.matchOnEndPointGre() || m.matchOnNF()))
				{
					if(((ActionEndpointInternal*)(*outputAction))->toString() == rri.endpointInternal)
					{
						//The action is on the same endpoint of the removed one, hence the vlink must not be removed
						return;
					}
				}
			}
		}//end of again iterator on the rules of the graph

		ULOG_DBG_INFO("Virtual link no longer required for the internal endpoint: %s",rri.endpointInternal.c_str());

		uint64_t tobeRemovedID = endpoints_internal_vlinks.find(rri.endpointInternal)->second;
		lsi->removeEndPointvlink(rri.endpointInternal);

		for(map<string, uint64_t>::iterator nfvl = nfs_vlinks.begin(); nfvl != nfs_vlinks.end(); nfvl++)
		{
			if(nfvl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the NF port: %s",nfvl->first.c_str());
				return;
			}
		}

		for(map<string, uint64_t>::iterator gvl = endpoints_gre_vlinks.begin(); gvl != endpoints_gre_vlinks.end(); gvl++)
		{
			if(gvl->second == tobeRemovedID)
			{
				ULOG_DBG_INFO("The virtual link cannot be removed because it is still used by the gre-tunnel endpoint: %s",gvl->first.c_str());
				return;
			}
		}

		ULOG_DBG_INFO("The virtual link must be removed");

		try
		{
			VLink toBeRemoved = lsi->getVirtualLink(tobeRemovedID);
			DestroyVirtualLinkIn dvli(lsi->getDpid(), toBeRemoved.getLocalID(), toBeRemoved.getRemoteDpid(), toBeRemoved.getRemoteID());
			switchManager.destroyVirtualLink(dvli);
			lsi->removeVlink(tobeRemovedID);
		} catch (SwitchManagerException e)
		{
			ULOG_ERR("%s",e.what());
			throw GraphManagerException();
		}

	}
}

vector<set<string> > GraphManager::identifyVirtualLinksRequired(highlevel::Graph *graph)
{
	set<string> NFs;
	set<string> endPointsGre;
	set<string> phyPorts;
	set<string> endPointsInternal;
	set<string> endPointsHoststack;

	//The number of virtual link depends on the rules, and not on the VNF ports / endpoints of the graph
	list<highlevel::Rule> rules = graph->getRules();

	for(list<highlevel::Rule>::iterator rule = rules.begin(); rule != rules.end(); rule++)
	{
		list<OutputAction*> outputActions = rule->getAction()->getOutputActions();
		highlevel::Match match = rule->getMatch();
		for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
		{
			if((*outputAction)->getType() == ACTION_ON_NETWORK_FUNCTION)
			{
				//we are considering rules as
				//	- match: internal-25 - action: output to VNF firewall port 1
				//	- match: interface eth0 - action: output to VNF firewall port 1
				//Each different VNF port that is in an action matching an internal endpoint or an interface endpoint requires a different virtual link
				if(match.matchOnPort() || match.matchOnEndPointInternal())
				{
					ActionNetworkFunction *action_nf = (ActionNetworkFunction*)(*outputAction);
					stringstream ss;
					ss << (*outputAction)->getInfo() << "_" << action_nf->getPort();
					NFs.insert(ss.str()); //the set avoids duplications
				}

				// gre -> VNF does not require any virtual link
				// VNF -> VNF does not require any virtual link
			}
			else if((*outputAction)->getType() == ACTION_ON_ENDPOINT_GRE)
			{
				//we are considering rules as
				//	- match: internal-25 - action: output to gre tunnel with key 1
				//	- match: interface eth0 - action: output to gre tunnel with key 1
				//Each different gre tunnel (i.e., tunnel with a different key) that is in an action matching
				//an internal endpoint or an interface endpoint requires a different virtual link

				if(match.matchOnPort() || match.matchOnEndPointInternal())
				{
					ActionEndpointGre *action_ep = (ActionEndpointGre*)(*outputAction);
					endPointsGre.insert(action_ep->toString());
				}

				// gre -> gre does not require any virtual link
				// VNF -> gre does not require any virtual link
				// VNF -> VNF does not require any virtual link
			}
			else if((*outputAction)->getType() == ACTION_ON_PORT)
			{
				//we are considering rules as
				//	- match: gre tunnel with key 1 - action: output to interface eth0
				//	- match: VNF firewall port 1 - action: output to interface eth0
				//	- match: Hoststack port - action: output to interface eth0
				//Each different interface requires a different virtual link
				if(match.matchOnNF() || match.matchOnEndPointGre() || match.matchOnEndPointHoststack())
					phyPorts.insert((*outputAction)->getInfo());

				// interface -> interface does not require any virtual link
				// internal endpoint -> interface does not require any virtual link
			}
			else if((*outputAction)->getType() == ACTION_ON_ENDPOINT_INTERNAL)
			{
				if(!match.matchOnPort() && !match.matchOnEndPointInternal())
				{
					ActionEndpointInternal *action_ep = (ActionEndpointInternal*)(*outputAction);
					endPointsInternal.insert(action_ep->toString());
				}

				// internal endpoint -> internal endpoint does not require any virtual link
				// interface -> internal endpoint does not require any virtual link
			}
			else if((*outputAction)->getType() == ACTION_ON_ENDPOINT_HOSTSTACK)
			{
				//we are considering rules as
				//	- match: internal-25 - action: output to hoststack endpoint
				//	- match: interface eth0 - action: output to hoststack endpoint

				if(match.matchOnPort() || match.matchOnEndPointInternal())
				{
					ActionEndPointHostStack *action_ep = (ActionEndPointHostStack*)(*outputAction);
					endPointsHoststack.insert(action_ep->toString());
				}

				// VNF -> hoststackEP does not require any virtual link
				// hoststackEP -> VNF does not require any virtual link
			}
		}
	}

	if(NFs.size() != 0)
	{
		ULOG_DBG_INFO("Network functions input ports requiring a virtual link:");
		for(set<string>::iterator nf = NFs.begin(); nf != NFs.end(); nf++)
			ULOG_DBG_INFO("\t%s",(*nf).c_str());
	}
	if(phyPorts.size() != 0)
	{
		ULOG_DBG_INFO("Physical ports requiring a virtual link:");
		for(set<string>::iterator p = phyPorts.begin(); p != phyPorts.end(); p++)
			ULOG_DBG_INFO("\t%s",(*p).c_str());
	}
	if(endPointsGre.size() != 0)
	{
		ULOG_DBG_INFO("Gre endpoints requiring a virtual link:");
		for(set<string>::iterator e = endPointsGre.begin(); e != endPointsGre.end(); e++)
			ULOG_DBG_INFO("\t%s",(*e).c_str());
	}
	if(endPointsInternal.size() != 0)
	{
		ULOG_DBG_INFO("Internal endpoints requiring a virtual link:");
		for(set<string>::iterator e = endPointsInternal.begin(); e != endPointsInternal.end(); e++)
			ULOG_DBG_INFO("\t%s",(*e).c_str());
	}
	if(endPointsHoststack.size() != 0)
	{
		ULOG_DBG_INFO("Hoststack endpoint requiring a virtual link:");
		for(set<string>::iterator e = endPointsHoststack.begin(); e != endPointsHoststack.end(); e++)
			ULOG_DBG_INFO("\t%s",(*e).c_str());
	}
	
	vector<set<string> > retval;
	vector<set<string> >::iterator rv;

	rv = retval.end();
	retval.insert(rv,NFs);
	rv = retval.end();
	retval.insert(rv,phyPorts);
	rv = retval.end();
	retval.insert(rv,endPointsGre);
	rv = retval.end();
	retval.insert(rv,endPointsInternal);
	rv = retval.end();
	retval.insert(rv,endPointsHoststack);
	return retval;
}

// TODO: implement support for hoststack endpoint
vector<set<string> > GraphManager::identifyVirtualLinksRequired(highlevel::Graph *newPiece, LSI *lsi)
{
	set<string> NFs;
	set<string> phyPorts;
	set<string> endPointsGre;
	set<string> endPointsHoststack;
	set<string> endPointsInternal;

	list<highlevel::Rule> rules = newPiece->getRules();
	for(list<highlevel::Rule>::iterator rule = rules.begin(); rule != rules.end(); rule++)
	{
		highlevel::Action *action = rule->getAction();
		highlevel::Match match = rule->getMatch();

		list<OutputAction*> outputActions = action->getOutputActions();
		for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
		{
			if((*outputAction)->getType() == ACTION_ON_NETWORK_FUNCTION)
			{
				if(match.matchOnPort() || match.matchOnEndPointInternal())
				{
					ActionNetworkFunction *action_nf = (ActionNetworkFunction*)(*outputAction);

					//Check if a vlink is required for this network function port
					map<string, uint64_t> nfs_vlinks = lsi->getNFsVlinks();
					stringstream ss;
					ss << (*outputAction)->getInfo() << "_" << action_nf->getPort();
					if(nfs_vlinks.count(ss.str()) == 0)
						NFs.insert(ss.str());
				}
			}
			else if((*outputAction)->getType() == ACTION_ON_ENDPOINT_GRE)
			{
				if(match.matchOnPort() || match.matchOnEndPointInternal())
				{
					//check if a vlink is required for this gre-tunnel endpoint
					map<string, uint64_t> endpoints_vlinks = lsi->getEndPointsGreVlinks();
					ActionEndpointGre *action_ep = (ActionEndpointGre*)(*outputAction);
					if(endpoints_vlinks.count(action_ep->toString()) == 0)
						endPointsGre.insert(action_ep->toString());
				}
			}
			else if((*outputAction)->getType() == ACTION_ON_ENDPOINT_HOSTSTACK)
			{
				if(match.matchOnPort() || match.matchOnEndPointInternal())
				{
					//check if a vlink is required for this gre-tunnel endpoint
					map<string, uint64_t> endpoints_vlinks = lsi->getEndPointsHoststackVlinks();
					ActionEndPointHostStack *action_ep = (ActionEndPointHostStack*)(*outputAction);
					if(endpoints_vlinks.count(action_ep->toString()) == 0)
						endPointsHoststack.insert(action_ep->toString());
				}
			}
			else if((*outputAction)->getType() == ACTION_ON_PORT)
			{
				if(match.matchOnNF() || match.matchOnEndPointGre())
				{
					//check if a vlink is required for this physical port (i.e., interface endpoint)
					map<string, uint64_t> ports_vlinks = lsi->getPortsVlinks();
					ActionPort *action_port = (ActionPort*)(*outputAction);
					if(ports_vlinks.count(action_port->getInfo()) == 0)
						phyPorts.insert(action_port->getInfo());
				}
			}
			else if((*outputAction)->getType() == ACTION_ON_ENDPOINT_INTERNAL)
			{
				if(!match.matchOnPort() && !match.matchOnEndPointInternal())
				{
					//check if a vlink is required for this internal endpoint
					map<string, uint64_t> endpoints_vlinks = lsi->getEndPointsVlinks();
					ActionEndpointInternal *action_ep = (ActionEndpointInternal*)(*outputAction);
					if(endpoints_vlinks.count(action_ep->toString()) == 0)
						endPointsInternal.insert(action_ep->toString());
				}
			}
		}
	}

	ULOG_DBG_INFO("Network functions input ports requiring a virtual link:");
	for(set<string>::iterator nf = NFs.begin(); nf != NFs.end(); nf++)
		ULOG_DBG_INFO("\t%s",(*nf).c_str());
	ULOG_DBG_INFO("Physical ports requiring a virtual link:");
	for(set<string>::iterator p = phyPorts.begin(); p != phyPorts.end(); p++)
		ULOG_DBG_INFO("\t%s",(*p).c_str());
	ULOG_DBG_INFO("Gre endpoints requiring a virtual link:");
	for(set<string>::iterator e = endPointsGre.begin(); e != endPointsGre.end(); e++)
		ULOG_DBG_INFO("\t%s",(*e).c_str());
	ULOG_DBG_INFO("Internal endpoints requiring a virtual link:");
	for(set<string>::iterator e = endPointsInternal.begin(); e != endPointsInternal.end(); e++)
		ULOG_DBG_INFO("\t%s",(*e).c_str());
	ULOG_DBG_INFO("Hoststack endpoints requiring a virtual link:");
	for(set<string>::iterator e = endPointsHoststack.begin(); e != endPointsHoststack.end(); e++)
		ULOG_DBG_INFO("\t%s",(*e).c_str());

	//prepare the return value
	vector<set<string> > retval;
	vector<set<string> >::iterator rv;
	rv = retval.end();
	retval.insert(rv,NFs);
	rv = retval.end();
	retval.insert(rv,phyPorts);
	rv = retval.end();
	retval.insert(rv,endPointsGre);
	rv = retval.end();
	retval.insert(rv,endPointsInternal);
	rv = retval.end();
	retval.insert(rv,endPointsHoststack);
	return retval;
}

void GraphManager::printInfo(lowlevel::Graph graphLSI0, LSI *lsi0)
{
	ULOG_INFO("");
	ULOG_INFO("");
	ULOG_INFO("Graphs deployed:");
	ULOG_INFO("");

	ULOG_INFO("\tID: 'LSI-0'");
	ULOG_INFO("\tRules: ");

	map<string,LSI *> lsis;
	for(map<string,GraphInfo>::iterator graphs = tenantLSIs.begin(); graphs != tenantLSIs.end(); graphs++)
		lsis[graphs->first] = graphs->second.getLSI();

	graphLSI0.prettyPrint(lsi0,lsis);
	ULOG_INFO("");

	printInfo(false);
}

void GraphManager::printInfo(bool completed)
{
	if(completed)
	{
		ULOG_INFO("");
		ULOG_INFO("");
		ULOG_INFO("Graphs deployed:");
		ULOG_INFO("");
	}

	map<string,GraphInfo>::iterator it;
	for(it = tenantLSIs.begin(); it != tenantLSIs.end(); it++)
	{
		int id;
		sscanf(it->first.c_str(),"%d",&id);

		if(id == 2)
		{
			coloredLogger(ANSI_COLOR_BLUE,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\tGraph ID: '%s'",it->first.c_str());
			coloredLogger(ANSI_COLOR_BLUE,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\tVNF installed:");
		}
		else if(id == 3)
		{
			coloredLogger(ANSI_COLOR_RED,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\tID: '%s'",it->first.c_str());
			coloredLogger(ANSI_COLOR_RED,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\tVNF installed:");
		}
		else
		{
			ULOG_INFO("\tID: '%s'",it->first.c_str());
			ULOG_INFO("\tVNF installed:");
		}

		ComputeController *computeController = it->second.getComputeController();
		computeController->printInfo(id);
		ULOG_INFO("");
	}

	ULOG_INFO("");
}

list<string> GraphManager::getRulesIDForLSI0(highlevel::Rule rule, string graphID)
{
	list<string> rulesID;

	stringstream ss;
	ss << graphID << "_" << rule.getRuleID();
	string startingRuleID = ss.str();

	list<OutputAction*> outputActions = rule.getAction()->getOutputActions();
	highlevel::Match match = rule.getMatch();

	int vlinkUsed=0;
	if(match.matchOnEndPointGre() || match.matchOnNF())
		for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
		{
			output_action_t actionType = (*outputAction)->getType();
			if(actionType == ACTION_ON_PORT || actionType == ACTION_ON_ENDPOINT_INTERNAL)
				vlinkUsed++;
		}

	if(vlinkUsed<2)
		rulesID.push_back(startingRuleID);
	else
		for(int n=0; n<vlinkUsed; n++)
		{
			stringstream newRuleID;
			newRuleID << startingRuleID << "_split" << (n+1);
			rulesID.push_back(newRuleID.str());
		}

	return rulesID;
}

list<string> GraphManager::getRulesIDForLSITenant(highlevel::Rule rule)
{
	list<string> rulesID;

	stringstream ss;
	ss << rule.getRuleID();
	string startingRuleID = ss.str();

	list<OutputAction*> outputActions = rule.getAction()->getOutputActions();
	highlevel::Match match = rule.getMatch();

	int vlinkUsed=0;
	if(match.matchOnPort() || match.matchOnEndPointInternal())
		for(list<OutputAction*>::iterator outputAction = outputActions.begin(); outputAction != outputActions.end(); outputAction++)
		{
			output_action_t actionType = (*outputAction)->getType();
			if(actionType == ACTION_ON_NETWORK_FUNCTION || actionType == ACTION_ON_ENDPOINT_GRE)
				vlinkUsed++;
		}

	if(vlinkUsed<2)
		rulesID.push_back(startingRuleID);
	else
		for(int n=0; n<vlinkUsed; n++)
		{
			stringstream newRuleID;
			newRuleID << startingRuleID << "_split" << (n+1);
			rulesID.push_back(newRuleID.str());
		}

	return rulesID;
}

string GraphManager::getVnfRepoEndpoint()
{
	stringstream ss;
	ss << "http://" << Configuration::instance()->getVnfRepoIp() << ":" << Configuration::instance()->getVnfRepoPort();
	return ss.str();
}

string GraphManager::getVnfTemplateId(string graphId, string macAddress)
{
	string templateId;

	if(tenantLSIs.count(graphId) == 0)
		return "";

	GraphInfo graphInfo = (tenantLSIs.find(graphId))->second;
	ComputeController *computeController = graphInfo.getComputeController();
	highlevel::Graph *graph = graphInfo.getGraph();

	list<highlevel::VNFs> network_functions = graph->getVNFs();
	for(list<highlevel::VNFs>::iterator nf = network_functions.begin(); nf != network_functions.end(); nf++)
	{
		list<highlevel::vnf_port_t> ports = nf->getPorts();
		for(list<highlevel::vnf_port_t>::iterator port = ports.begin(); port != ports.end(); port++)
			if(port->configuration.mac_address == macAddress)
			{
				templateId = nf->getVnfTemplate();
				if(templateId.empty())
				{
					Description *description = computeController->getNFSelectedImplementation(nf->getId());
					templateId = description->getTemplate()->getId();
				}
				break;
			}
	}
	return templateId;
}

