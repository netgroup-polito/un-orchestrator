#include "manager.h"

/* TODO - These should come from an orchestrator config file (currently, there is only one for the UN ports) */
//static const char* OVS_BASE_SOCK_PATH = "/usr/local/var/run/openvswitch/";

ERFSManager::ERFSManager() : nextLsi(0)
{
}

ERFSManager::~ERFSManager()
{
}

void ERFSManager::checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi)
{ // SwitchManager implementation
	//logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "checkPhysicalInterfaces(dpid: %" PRIu64 " NF:%s NFType:%d)", anpi.getDpid(), anpi.getNfId().c_str(), anpi.getNFtype());
}

CreateLsiOut *ERFSManager::createLsi(CreateLsiIn cli)
{  // SwitchManager implementation
    unsigned int dpid = nextLsi++;
    nextPort[dpid]=1;
    string core_id = "auto"; // FIXME: this should be handled by the LO

	map<string,unsigned int >  endpoints_ports;

    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "createLsi() creating ERFS LSI %d", dpid);

    stringstream cmd;
    cmd << getenv("un_script_path") << CMD_CREATE_LSI << " " << dpid << " " << cli.getControllerAddress() << " " << cli.getControllerPort() << " ";
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd.str().c_str());
    int retVal = system(cmd.str().c_str());
    if(retVal != 0) {
        logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Failed to create LSI, result: %d", retVal);
        throw ERFSManagerException();
    }

    // Add physical ports
    list<string> ports = cli.getPhysicalPortsName();
    typedef map<string,unsigned int> PortsNameIdMap;
    PortsNameIdMap out_physical_ports;
    list<string>::iterator pit = ports.begin();
    for(; pit != ports.end(); pit++)
    {
        unsigned int port_id = nextPort[dpid]++;
        logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, " phy port \"%s\" = %d", pit->c_str(), port_id);
        stringstream cmd_add;
        cmd_add << getenv("un_script_path") << CMD_ADD_PORT << " " << dpid << " " << *pit << " " << "dpdk" << " " << port_id << " " << core_id;
        logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
        int retVal = system(cmd_add.str().c_str());
        if(retVal != 0) {
            logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Failed to add port, result: %d", retVal);
            throw ERFSManagerException();
        }
        // TODO - Really check result!
        out_physical_ports.insert(PortsNameIdMap::value_type(*pit, port_id));
    }

    // Add NF ports
    map<string,map<string, unsigned int> > out_nf_ports_name_and_id;
    typedef map<string,PortsNameIdMap> NfPortsMapMap;
    NfPortsMapMap out_nf_ports;
    map<string,nf_t> nf_types = cli.getNetworkFunctionsType();
    map<string,list<string> > out_nf_ports_name_on_switch;
    set<string> nfs = cli.getNetworkFunctionsName();
    for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++) {
//        list<string> port_name_on_switch;
        AddNFportsIn anfpi(dpid, *nf, nf_types[*nf], cli.getNetworkFunctionsPortsInfo(*nf));
        AddNFportsOut *anfpo = addNFPorts(anfpi);
        if (anfpo == NULL) 
            logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "error in nfport add");
        out_nf_ports.insert(NfPortsMapMap::value_type(*nf, anfpo->getPorts()));
        out_nf_ports_name_on_switch[*nf] = anfpo->getPortsNameOnSwitch();
        out_nf_ports_name_and_id[*nf] = anfpo->getPortNamesAndId();
        delete anfpo;

    }

    // Add Ports for Virtual Links (patch ports)
    int vlink_n = 0;
    list<pair<unsigned int, unsigned int> > out_virtual_links;
    list<uint64_t> vlinks = cli.getVirtualLinksRemoteLSI();
    for(list<uint64_t>::iterator vl = vlinks.begin(); vl != vlinks.end(); vl++) {
        AddVirtualLinkIn avli(dpid, *vl);
        AddVirtualLinkOut *avlo = addVirtualLink(avli);
        if (avlo == NULL) logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "error in vlink add");
        out_virtual_links.push_back(make_pair(avlo->getIdA(), avlo->getIdB()));
        vlink_n++;
    }

    CreateLsiOut *clo = new CreateLsiOut(dpid, out_physical_ports, out_nf_ports, endpoints_ports, out_nf_ports_name_on_switch, out_virtual_links,out_nf_ports_name_and_id);
    return clo;
}

AddNFportsOut *ERFSManager::addNFPorts(AddNFportsIn anpi)
{ // SwitchManager implementation
    uint32_t numa_node = 0; // FIXME: assign ring to the correct NUMA node
    typedef map<string,unsigned int> PortsNameIdMap;
    map<string, unsigned int> port_names_and_id;
    AddNFportsOut *anpo = NULL;
    uint64_t dpid = anpi.getDpid();
    string nf = anpi.getNfId();
    nf_t nf_type = anpi.getNFtype();
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "addNFPorts(dpid: %" PRIu64 " NF id:%s NFType:%d)", anpi.getDpid(), anpi.getNfId().c_str(), anpi.getNFtype());
    list<struct nf_port_info> nfs_ports = anpi.getNetworkFunctionsPorts();
    list<string> port_name_on_switch;
    PortsNameIdMap nf_ports_ids;

    for(list<struct nf_port_info>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++) {
        logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "\tport: %s", nfp->port_name.c_str());
        unsigned int port_id = nextPort[dpid]++;
        const char* port_type = "ivshmem"; // TODO: Use nfp->port_type and act accordingly
        logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, " NF port \"%s.%s\" = %d (type=%d)", nf.c_str(), nfp->port_name.c_str(), port_id, nf_type);
        stringstream cmd_add;
        cmd_add << getenv("un_script_path") << CMD_ADD_PORT << " " << dpid << " " << numa_node << " " << port_type << " " << port_id << " auto";
        logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
        int retVal = system(cmd_add.str().c_str());
        if(retVal != 0) {
            logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Failed to add port, result: %d", retVal);
            throw ERFSManagerException();
        }
        // TODO - Really check result!
        nf_ports_ids.insert(PortsNameIdMap::value_type(nfp->port_name, port_id));
        stringstream pid;
        pid << port_id;
        port_name_on_switch.push_back(pid.str());
	port_names_and_id[pid.str()] = port_id;
    }
    anpo = new AddNFportsOut(anpi.getNfId(), nf_ports_ids, port_name_on_switch, port_names_and_id);
    return anpo;
}

AddVirtualLinkOut *ERFSManager::addVirtualLink(AddVirtualLinkIn avli)
{ // SwitchManager implementation
    AddVirtualLinkOut *avlo = NULL;
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "addVirtualLink(dpid: %" PRIu64 " -> %" PRIu64 ")", avli.getDpidA(), avli.getDpidB());
    unsigned int a_port_id = nextPort[avli.getDpidA()]++;
    unsigned int b_port_id = nextPort[avli.getDpidB()]++;
    const char* port_type = "xswitch";
    const char* port_name = "XSWITCH";
    stringstream cmd_add;
    int retVal;

    // XSWITCH port A
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, " XSWITCH port %u.%u (type=%s)", avli.getDpidA(), a_port_id, port_type);
    cmd_add << getenv("un_script_path") << CMD_ADD_PORT << " " << avli.getDpidA() << " " << port_name << " " << port_type << " " << a_port_id;
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
    retVal = system(cmd_add.str().c_str());
    if(retVal != 0) {
        logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Failed to add port, result: %d", retVal);
        throw ERFSManagerException();
    }
    cmd_add.str("");
    cmd_add.clear();

    // XSWITCH port B
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, " XSWITCH port %u.%u (type=%s)", avli.getDpidB(), b_port_id, port_type);
    cmd_add << getenv("un_script_path") << CMD_ADD_PORT << " " << avli.getDpidB() << " " << port_name << " " << port_type << " " << b_port_id;
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
    retVal = system(cmd_add.str().c_str());
    if(retVal != 0) {
        logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Failed to add port, result: %d", retVal);
        throw ERFSManagerException();
    }
    cmd_add.str("");
    cmd_add.clear();

    // Link between them
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, " Virtual link between LSIs %u:%u <-> %u:%u", avli.getDpidA(), a_port_id, avli.getDpidB(), b_port_id);
    cmd_add << getenv("un_script_path") << CMD_VIRTUAL_LINK << " " << avli.getDpidA() << " " << avli.getDpidB() << " " << a_port_id << " " << b_port_id;
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_add.str().c_str());
    retVal = system(cmd_add.str().c_str());
    if(retVal != 0) {
        logger(ORCH_WARNING, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "Failed to create virtual link, result: %d", retVal);
        throw ERFSManagerException();
    }

    avlo = new AddVirtualLinkOut(a_port_id, b_port_id);
    return avlo;
}

void ERFSManager::destroyLsi(uint64_t dpid)
{ // SwitchManager implementation
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "destroyLsi(dpid: %" PRIu64 " -> %" PRIu64 ")", dpid);
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "NOT SUPPORTED");
}

void ERFSManager::destroyVirtualLink(DestroyVirtualLinkIn dvli)
{ // SwitchManager implementation
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "destroyVirtualLink(%" PRIu64 ".%" PRIu64 " -> %" PRIu64 ".%" PRIu64 ")", dvli.getDpidA(), dvli.getIdA(), dvli.getDpidB(), dvli.getIdB());
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "NOT SUPPORTED");
}

void ERFSManager::destroyNFPorts(DestroyNFportsIn dnpi)
{ // SwitchManager implementation
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "destroyNFPorts");
    logger(ORCH_DEBUG_INFO, ERFS_MAN_MODULE_NAME, __FILE__, __LINE__, "NOT SUPPORTED");
}
