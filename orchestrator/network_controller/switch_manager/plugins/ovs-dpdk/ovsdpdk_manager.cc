#include "ovsdpdk_manager.h"

/* TODO - These should come from an orchestrator config file (currently, there is only one for the UN ports) */
static const char* OVS_BASE_SOCK_PATH = "/usr/local/var/run/openvswitch/";

#if defined(ENABLE_KVM_IVSHMEM) || defined(ENABLE_DPDK_PROCESSES)
	int OVSDPDKManager::nextportname = 1;
#endif

OVSDPDKManager::OVSDPDKManager() : m_NextLsiId(0), m_NextPortId(1) /* 0 is not valid for OVS */
{
}

OVSDPDKManager::~OVSDPDKManager()
{
}

void OVSDPDKManager::checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi)
{ // SwitchManager implementation

	//TODO: not implemented yet
}

CreateLsiOut *OVSDPDKManager::createLsi(CreateLsiIn cli)
{  // SwitchManager implementation

	unsigned int dpid = m_NextLsiId++;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "createLsi() creating LSI %d", dpid);

	stringstream cmd;
	cmd << CMD_CREATE_LSI << " " << dpid << " " << cli.getControllerAddress() << " " << cli.getControllerPort() << " ";
	// Set the OpenFlow version
	switch(OFP_VERSION) {
		case OFP_10:
			cmd << "OpenFlow10";
			break;
		case OFP_12:
			cmd << "OpenFlow12";
			break;
		case OFP_13:
			cmd << "OpenFlow13";
			break;
	}
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd.str().c_str());
	int retVal = system(cmd.str().c_str());
	retVal = retVal >> 8;
	if(retVal == 0) {
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Failed to create LSI");
		throw OVSDPDKManagerException();
	}

	// Add physical ports
	list<string> ports = cli.getPhysicalPortsName();
	typedef map<string,unsigned int> PortsNameIdMap;
	PortsNameIdMap out_physical_ports;

	list<string>::iterator pit = ports.begin();
	for(; pit != ports.end(); pit++)
	{
		// Go create it!
		unsigned int port_id = m_NextPortId++;
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " phy port \"%s\" = %d", pit->c_str(), port_id);
		stringstream cmd_add;

		const char * port_type = ((*pit).compare(0, 4, "dpdk") == 0) ? "dpdk":"host";
		cmd_add << CMD_ADD_PORT << " " << dpid << " " << *pit << " " << port_type << " " << port_id;
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
		int retVal = system(cmd_add.str().c_str());
		retVal = retVal >> 8;
		if(retVal == 0) {
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Failed to add port");
			throw OVSDPDKManagerException();
		}
		// TODO - Really check result!
		out_physical_ports.insert(PortsNameIdMap::value_type(*pit, port_id));
	}

	// Add NF ports
	typedef map<string,PortsNameIdMap > NfPortsMapMap;
	map<string,nf_t> nf_types = cli.getNetworkFunctionsType();
	NfPortsMapMap out_nf_ports;
	list<pair<unsigned int, unsigned int> > out_virtual_links;
	set<string> nfs = cli.getNetworkFunctionsName();

	map<string,list<string> > out_nf_ports_name_on_switch;

	for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++) {
		nf_t nf_type = nf_types[*nf];

		list<string> nf_ports = cli.getNetworkFunctionsPortNames(*nf);
		PortsNameIdMap nf_ports_ids;

		list<string> port_name_on_switch;

		for(list<string>::iterator nfp = nf_ports.begin(); nfp != nf_ports.end(); nfp++) {
			unsigned int port_id = m_NextPortId++;

			stringstream sspn;
// TODO - Support most combinations of NF types at run-time instead of compile-time
#if defined(ENABLE_KVM_IVSHMEM) || defined(ENABLE_DPDK_PROCESSES)
			//In these cases we use the rte_rings to exchange packets with the VNFs

			char* port_type = "dpdkr";
			if ((nf_type == KVM) || (nf_type == DPDK)) {  // IVSHMEM-based DPDK rings
				sspn << port_type << nextportname;
				nextportname++;
			}
			else { // This supports e.g. Docker containers
				port_type = "veth";
				sspn << dpid << "_" << *nfp;
			}
#else
			const char* port_type = (nf_type == KVM) ? "dpdkvhostuser" : "veth";  // TODO - dpdkr, dpdkvhostuser, tap, virtio ...

			sspn << dpid << "_" << *nfp;
#endif
			string port_name = sspn.str();

			port_name_on_switch.push_back(port_name);

			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Network function port '%s' corresponds to the port '%s' on the switch", nfp->c_str(),port_name.c_str());

			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " NF port \"%s.%s\" = %d (nf_type=%d)", nf->c_str(), nfp->c_str(), port_id, nf_type);
			stringstream cmd_add;
			cmd_add << CMD_ADD_PORT << " " << dpid << " " << port_name << " " << port_type << " " << port_id;
			cmd_add << " " << OVS_BASE_SOCK_PATH;
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
			int retVal = system(cmd_add.str().c_str());
			retVal = retVal >> 8;
			if(retVal == 0) {
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Failed to add port");
				throw OVSDPDKManagerException();
			}
			// TODO - Really check result!
			nf_ports_ids.insert(PortsNameIdMap::value_type(*nfp, port_id));
		}
		out_nf_ports.insert(NfPortsMapMap::value_type(*nf, nf_ports_ids));
		out_nf_ports_name_on_switch[*nf] = port_name_on_switch;
	}

	// Add Ports for Virtual Links (patch ports)
	int vlink_n = 0;
	list<uint64_t> vlinks = cli.getVirtualLinksRemoteLSI();
	for(list<uint64_t>::iterator vl = vlinks.begin(); vl != vlinks.end(); vl++) {

		unsigned int s_port_id = m_NextPortId++;
		unsigned int d_port_id = m_NextPortId++;

		// In order to avoid creating loops for broadcast messages, flooding can only be enabled on one
		// of the Virtual Links between two same LSIs.
		// TODO: We need to track which VLink is doing it so that, when there are updates made to the
		// graph, we know when the flooding VLink is deleted and we can enable another VLink.
		bool enable_flooding = (vlink_n == 0);

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, " Virtual link to LSI %u: %u:%u <-> %u:%u", *vl, dpid, s_port_id, *vl, d_port_id);
		stringstream cmd_add;
		cmd_add << CMD_VIRTUAL_LINK << " " << dpid << " " << *vl << " " << s_port_id << " " << d_port_id << " " << vlink_n << " " << enable_flooding;
		// Set the OpenFlow version
		switch(OFP_VERSION) {
			case OFP_10:
				cmd << "OpenFlow10";
				break;
			case OFP_12:
				cmd << "OpenFlow12";
				break;
			case OFP_13:
				cmd << "OpenFlow13";
				break;
		}

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
		int retVal = system(cmd_add.str().c_str());
		retVal = retVal >> 8;
		if(retVal == 0) {
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Failed to create virtual link");
			throw OVSDPDKManagerException();
		}

		out_virtual_links.push_back(make_pair(s_port_id, d_port_id));

		vlink_n++;
	}

	CreateLsiOut *clo = new CreateLsiOut(dpid, out_physical_ports, out_nf_ports, out_nf_ports_name_on_switch, out_virtual_links);
	return clo;
}

AddNFportsOut *OVSDPDKManager::addNFPorts(AddNFportsIn anpi)
{ // SwitchManager implementation

  	//TODO: not implemented yet

	assert(0 && "Method not implemented yet!");

	AddNFportsOut *anpo = NULL;
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "addNFPorts(dpid: %" PRIu64 " NF:%s NFType:%d)", anpi.getDpid(), anpi.getNFname().c_str(), anpi.getNFtype());
	list<string> nfs_ports = anpi.getNetworkFunctionsPorts();
	for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++) {
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\tport: %s", (*nfp).c_str());
	}
	return anpo;
}

AddVirtualLinkOut *OVSDPDKManager::addVirtualLink(AddVirtualLinkIn avli)
{ // SwitchManager implementation

	//TODO: not implemented yet

	assert(0 && "Method not implemented yet!");

	AddVirtualLinkOut *avlo = NULL;
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "addVirtualLink(dpid: %" PRIu64 " -> %" PRIu64 ")", avli.getDpidA(), avli.getDpidB());
	return avlo;
}

void OVSDPDKManager::destroyLsi(uint64_t dpid)
{ // SwitchManager implementation
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "destroyLsi(dpid: %" PRIu64 " -> %" PRIu64 ")", dpid);
	stringstream cmd;
	cmd << CMD_DESTROY_LSI << " " << dpid;

	int retVal = system(cmd.str().c_str());
	retVal = retVal >> 8;
	if(retVal == 0) {
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Failed to destroy LSI");
		throw OVSDPDKManagerException();
	}
}

void OVSDPDKManager::destroyVirtualLink(DestroyVirtualLinkIn dvli)
{ // SwitchManager implementation

	//TODO: not implemented yet

	assert(0 && "Method not implemented yet!");

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "destroyVirtualLink(%" PRIu64 ".%" PRIu64 " -> %" PRIu64 ".%" PRIu64 ")",
			dvli.getDpidA(), dvli.getIdA(), dvli.getDpidB(), dvli.getIdB());

}

void OVSDPDKManager::destroyNFPorts(DestroyNFportsIn dnpi)
{ // SwitchManager implementation
	//TODO: not implemented yet

	assert(0 && "Method not implemented yet!");
}
