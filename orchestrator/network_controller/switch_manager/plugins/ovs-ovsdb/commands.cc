#include "ovsdb_manager.h"
#include "ovsdb_constants.h"

#include <iomanip>

#include <algorithm>

struct in_addr sIPaddr1;/* struct IP addr server*/
char ErrBuf[BUFFER_SIZE];

int rnumber = 1;
uint64_t dnumber = 1;

int pnumber = 1, nfnumber = 0, gnumber = 0;

/* Transaction ID */
static int tid = 0;

/*switch id, switch name*/
map<uint64_t, string> switch_id;
/*switch name, switch uuid*/
map<uint64_t, string> switch_uuid;
/*virtual link local, virtual link remote*/
map<string, string> peer_n;
/*switch id, list of ports name*/
map<uint64_t, list<string> > port_l;
/*switch id, list of gre-tunnel*/
map<uint64_t, list<string> > endpoint_l;
/*switch id, list of virtual link name*/
map<uint64_t, list<string> > vport_l;
/*switch id, list of ports uuid*/
map<uint64_t, list<string> > port_uuid;
/*switch id, list of virtual link uuid*/
map<uint64_t, list<string> > vport_uuid;
/*switch id, list of gre-tunnel uuid*/
map<uint64_t, list<string> > gport_uuid;
/*switch name, list of virtual link id*/
map<string, list<uint64_t> > virtual_link_id;
/*port id, port name*/
map<uint64_t, string> port_id;
/*
*	Map the port ID of the vlink on a switch ID
*	There are two entries per vlink (an entry on the 'local LSI', one
*	entry on the 'remote LSI'.
*/
map<uint64_t, uint64_t> vl_PortIDtoSwitchID;
/*
*	Map the port ID of the vlink on the 'local LSI' to the port ID of
*	the vlink on the 'remote LSI'.
*	There is one entry per vlink.
*/
map<uint64_t, uint64_t> vl_LocalPortIDtoRemotePortID;
/*map used to translate from port_name to DPDK Ring name*/
map<string, string> dpdkr_from_uuid;
/*map used to translate back from DPDK Ring name to port_id*/
map<string, string> dpdkr_to_uuid;

/**
 * Build name that is valid as UUID: no '-', no '_' ...
 */
string build_port_uuid_name(const string& port_name, uint64_t bridge_no)
{
	string p_lc = "x" + port_name;

	std::transform(p_lc.begin(), p_lc.end(), p_lc.begin(), ::tolower);

 	stringstream ss;
 	ss << p_lc << 'b' << dnumber;
	string uuid_name = ss.str();

	std::replace(uuid_name.begin(), uuid_name.end(), '_', 'p');
	std::replace(uuid_name.begin(), uuid_name.end(), '-', 's');
	return uuid_name;
}

//Constructor
commands::commands(){
}

//Destroyer
commands::~commands(){
}

/*connect to a ovs server using Socket*/
int commands::cmd_connect() {
	uint16_t tport_h;
	int	result, s;

	struct addrinfo Hints;
	struct addrinfo *AddrInfo;

	/*Read ip and port by the server*/
	result = inet_aton(SOCKET_IP, &sIPaddr1);
	if (!result){
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Invalid IP address.");
		throw commandsException();
	}

	if (sscanf(SOCKET_PORT, "%" SCNu16, &tport_h)!=1){
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Invalid number of port.");
		throw commandsException();
	}

	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AF_INET;
	Hints.ai_socktype= SOCK_STREAM;

	if (sock_initaddress (SOCKET_IP, SOCKET_PORT, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  SOCKET_IP, SOCKET_PORT, ErrBuf);
		throw commandsException();
	}

	if ( (s=sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Connection failure!");
		throw commandsException();
	}

	return s;
}

int commands::cmd_disconnect(int s){
	char ErrBuf[BUFFER_SIZE];

	if(sock_close(s, ErrBuf, sizeof(ErrBuf)) != 0){
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Closing socket failed.");
		throw commandsException();
	}

	return EXIT_SUCCESS;
}

/*
*	@cli = struct CreateLsiIn
*/
CreateLsiOut* commands::cmd_editconfig_lsi (CreateLsiIn cli, int s)
{
	unsigned int i=0;
	int nwritten = 0;

	ssize_t r = 0;
	char read_buf[BUFFER_SIZE];
	//list of remote LSI
	char rp[BUFFER_SIZE][BUF_SIZE];
	//Local variables
	const char *peer_name;
	char sw[BUF_SIZE], tcp_s[BUF_SIZE], ctr[BUF_SIZE], vrt[BUF_SIZE], trv[BUF_SIZE];
	char of_version[BUF_SIZE];

	stringstream datapath_id;

	map<string,unsigned int> physical_ports;
	map<string,map<string, unsigned int> >  network_functions_ports;
	map<string,unsigned int >  endpoints_ports;
	list<pair<unsigned int, unsigned int> > virtual_links;

	int dnumber_new = 0, nfnumber_old = 0;

	//list of physical ports
	list<string> ports = cli.getPhysicalPortsName();
	//list of nf
	set<string> nfs = cli.getNetworkFunctionsName();
	//list of nft
	map<string,nf_t> nf_type = cli.getNetworkFunctionsType();
	//list of endpoints
	map<string,vector<string> > endpoints = cli.getEndpointsPortsName();
	//list of remote LSI
	list<uint64_t> vport = cli.getVirtualLinksRemoteLSI();

	CreateLsiOut *clo = NULL;

	//save IPsec certificate
	this->ipsec_certificate = cli.getIPsecCertificate();

	/*force to use OpenFlow12*/
	strcpy(of_version, "OpenFlow12");

	//connect socket
	s = cmd_connect();

	/*root object contained three object [method, params, id]*/
	Object root;
	root["method"] = "transact";

	Array params;
	params.push_back("Open_vSwitch");

	Object first_obj;
	Object row;
	Array iface;
	Array iface1;
	Array iface2;

	//create Bridge
	/*create current name of a bridge "Bridge+dnumber"*/
	sprintf(sw, "Bridge%" PRIu64, dnumber);

	/*fill the map switch_id*/
	switch_id[dnumber] = string(sw);

	Array peer;
	Array peer1;
	Array peer2;

	int l = 0;

	/*Create the current target of a controller*/
	sprintf(tcp_s, "tcp:%s:%s", cli.getControllerAddress().c_str(), cli.getControllerPort().c_str());

	/*insert Controller*/
	first_obj["op"] = "insert";
	first_obj["table"] = "Controller";

	row["target"] = tcp_s;

	row["local_ip"] = cli.getLocalIP();
	row["connection_mode"] = "out-of-band";
	row["is_connected"] = true;

   	first_obj["row"] = row;

	//create the current name of controller --> ctrl+dnumber
	sprintf(ctr, "ctrl%" PRIu64, dnumber);

	first_obj["uuid-name"] = ctr;

	params.push_back(first_obj);

	row.clear();
   	first_obj.clear();

	datapath_id << std::setfill('0') << std::setw(16) << std::hex << dnumber << std::dec;

	/*insert a bridge*/
	first_obj["op"] = "insert";
	first_obj["table"] = "Bridge";

    row["name"] = sw;

    Array port;
	Array port1;
	Array port2;

	Array i_array;
   	Array ctrl;
   	ctrl.push_back("set");

	Array ctrl1;
	Array ctrl2;

	ctrl2.push_back("named-uuid");
	ctrl2.push_back(ctr);

	ctrl1.push_back(ctrl2);

	ctrl.push_back(ctrl1);

   	row["controller"] = ctrl;

	peer.push_back("map");

	peer2.push_back("disable-in-band");
	peer2.push_back("true");
	peer1.push_back(peer2);
	peer2.clear();

	peer2.push_back("datapath-id");
	peer2.push_back(datapath_id.str().c_str());

	peer1.push_back(peer2);
	peer.push_back(peer1);

#ifdef ENABLE_OVSDB_DPDK
	row["datapath_type"] = "netdev";
#endif

	row["other_config"] = peer;

    row["protocols"] = of_version;

    first_obj["row"] = row;

    first_obj["uuid-name"] = sw;

   	params.push_back(first_obj);

   	row.clear();
   	first_obj.clear();
	peer.clear();
   	peer1.clear();
   	peer2.clear();

   	dnumber_new = dnumber;

  	/*Object with four items [op, table, where, mutations]*/
   	Object second_obj;
    second_obj["op"] = "mutate";
    second_obj["table"] = "Open_vSwitch";

    /*Empty array [where]*/
   	Array where;
    second_obj["where"] = where;

	/*Array with one element*/
	Array w_array;

	/*Array with three elements*/
	Array m_array;
	m_array.push_back("bridges");
	m_array.push_back("insert");

	/*Array with two elements*/
	i_array.push_back("set");

	/*Array with one element*/
	Array s_array;

	/*Array with two element*/
	Array a_array;
	a_array.push_back("named-uuid");
	a_array.push_back(sw);

	s_array.push_back(a_array);

	i_array.push_back(s_array);

   	m_array.push_back(i_array);

   	w_array.push_back(m_array);

   	second_obj["mutations"] = w_array;

   	params.push_back(second_obj);

    root["params"] = params;
	root["id"] = tid;

	w_array.clear();
	m_array.clear();
	i_array.clear();
	s_array.clear();
	a_array.clear();

	//Increment transaction id
	tid++;

   	string *strr = new string[256];

   	stringstream ss;
 	write_formatted(root, ss);

    nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

 	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	//parse json response
	Value value;
	read( read_buf, value );
	Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		const string name = (*it).first;
		const Value &node = (*it).second;

		if (name == "result")
		{
			const Array &result = node.getArray();

			for(i=0;i<result.size();i++){
				Object uuidNode = result[i].getObject();

				for (Object::const_iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
				{
					const string name1 = (*it1).first;
					const Value &node1 = (*it1).second;

					if(name1 == "uuid"){
						const Array &stuff1 = node1.getArray();
						strr[i] = stuff1[1].getString();
					} else if(name1 == "details"){
						logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
						throw commandsException();
					}
				}
			}
		}
	}

	//store the switch-uuid
	switch_uuid[dnumber] = strr[i-2];

	/*create physical ports ports*/
	if(ports.size() !=0){
		for(list<string>::iterator p = ports.begin(); p != ports.end(); p++)
		{
			add_port((*p), dnumber, false, s);

			port_l[dnumber].push_back((*p).c_str());
			physical_ports[(*p)] = rnumber-1;
		}
	}

	/*Create interfaces related to the NFs*/
	map<string,list<string> > out_nf_ports_name_on_switch;
	map<string,map<string, unsigned int> > out_nf_ports_name_and_id;
	if(nfs.size() != 0) {

		/*for each network function port in the list of nfs*/
		for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		{
			list<struct nf_port_info> nfs_ports = cli.getNetworkFunctionsPortsInfo(*nf);
			list<string> port_names_on_switch;
			map<string, unsigned int> port_names_and_id;
			map<string,unsigned int> n_ports_1;

			/*for each network function port in the list of nfs_ports*/
			for(list<struct nf_port_info>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){
				string name_on_switch = add_port(nfp->port_name, dnumber, true, s, nfp->port_type);

				port2.push_back("named-uuid");

				//insert this port name into port_l
				string uuid_name = build_port_uuid_name(nfp->port_name, dnumber);
				//port_l[dnumber].push_back(name_on_switch);

				/*fill the map ports*/
				n_ports_1[nfp->port_name] = rnumber-1; //This is the ID Openflow
				port_names_on_switch.push_back(name_on_switch);
				port_names_and_id[name_on_switch] = nfp->port_id;

				logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Network function ID: %s - port name on graph: %s - port ID on LSI: %d",(*nf).c_str(),(nfp->port_name).c_str(),rnumber-1);
				logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Network function ID: %s - port name on LSI: %s",(*nf).c_str(),name_on_switch.c_str());
				logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Network function ID: %s - port name on LSI: %s - port ID on graph: %d",(*nf).c_str(),name_on_switch.c_str(),nfp->port_id);
			}

			/*fill the network_functions_ports*/
			network_functions_ports[(*nf)] = n_ports_1;
			out_nf_ports_name_on_switch[*nf] = port_names_on_switch;
			out_nf_ports_name_and_id[*nf] = port_names_and_id;
		}
	}

	//if there are one or more endpoints
	if(endpoints.size() != 0)
	{
		i = 0;

		/*for each endpoint in the list of endpoints*/
		for(map<string,vector<string> >::iterator ep = endpoints.begin(); ep != endpoints.end(); ep++)
		{
			char local_ip[BUF_SIZE];
			char remote_ip[BUF_SIZE];
			char key[BUF_SIZE];
			char port_name[BUF_SIZE];
			char ifac[BUF_SIZE];
			char is_safe[BUF_SIZE];

			string id = ep->first;

			vector<string> gre_param = ep->second;

			/*save the params of gre tunnel*/
			strcpy(key, gre_param[0].c_str());
			strcpy(local_ip, gre_param[1].c_str());
			strcpy(remote_ip, gre_param[2].c_str());
			strcpy(is_safe, gre_param[4].c_str());

			sprintf(port_name, "gre%d", gnumber);
			sprintf(ifac, "iface%d", rnumber);

			gnumber++;

			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "GRE tunnel -> key: %s - local IP: %s - remote IP: %s - interface: %s - safe: %s",key,local_ip,remote_ip,ifac,is_safe);

			add_endpoint(dnumber, local_ip, remote_ip, key, port_name, ifac, s, is_safe);

			endpoints_ports[id] = rnumber-1;

			endpoint_l[dnumber].push_back(id);
		}
	}

	/*Create interfaces related by the vlink ports*/
	if(vport.size() != 0)
	{
		nfnumber_old = pnumber;

		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			char ifac[BUF_SIZE];

			sprintf(vrt, "vport%d", pnumber);

			pnumber++;

			sprintf(trv, "vport%d", pnumber);

			peer_n[trv] = vrt;

			strcpy(rp[l], trv);

			sprintf(ifac, "iface%d", rnumber);

			cmd_add_virtual_link(vrt, trv, ifac, dnumber, s);

			port_id[rnumber-1] = vrt;
			port_id[rnumber+vport.size()-1] = trv;

			virtual_link_id[sw].push_back(rnumber-1);

			vport_l[dnumber].push_back(vrt);

			vl_PortIDtoSwitchID[rnumber-1] = dnumber;
			vl_PortIDtoSwitchID[rnumber+vport.size()-1] = (*nf);

			vl_LocalPortIDtoRemotePortID[rnumber-1] = rnumber+vport.size()-1;

			pnumber++;

			l++;

			virtual_links.push_back(make_pair(rnumber-1, rnumber+vport.size()-1));
		}

		pnumber = nfnumber_old;
	}

	//increment switch number
	dnumber++;

	root.clear();
	params.clear();
	row.clear();
	first_obj.clear();
	port2.clear();
	port1.clear();
	port.clear();
	peer.clear();
	peer1.clear();
	peer2.clear();

	uint64_t pi = 0;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	//disconnect socket
	cmd_disconnect(s);

	l = 0;

	if(vport.size() != 0)
	{
		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			char ifac[BUF_SIZE];

			//connect socket
			s = cmd_connect();

			root["method"] = "transact";
			params.push_back("Open_vSwitch");

			pi = (*nf);

			sprintf(vrt, "vport%d", pnumber);

			pnumber++;

			sprintf(ifac, "iface%d", rnumber);

			//store this vport
			vport_l[pi].push_back(rp[l]);

			peer_name = peer_n[rp[l]].c_str();

			cmd_add_virtual_link(rp[l], peer_name, ifac, pi, s);

			pnumber++;

			root.clear();
			params.clear();

			//increment transaction id
			tid++;

			l++;

			//disconnect socket
			cmd_disconnect(s);
		}
	}

    	clo = new CreateLsiOut(dnumber_new, physical_ports, network_functions_ports, endpoints_ports, out_nf_ports_name_on_switch, virtual_links, out_nf_ports_name_and_id);

	return clo;
}

#ifdef ENABLE_OVSDB_DPDK
string find_free_dpdkr()
{
	int idx;
	char name[] = "dpdkr9999";
	for (idx = 1; idx < 9999; ++idx) {
		sprintf(name, "dpdkr%d", idx);
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Checking presence of DPDK Ring %s", name);
		if (dpdkr_to_uuid.find(name) == dpdkr_to_uuid.end())
			break;
	}

	if (idx >= 9999) {
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Failed to find a free DPDK ring (dpdkr) name)");
		throw OVSDBManagerException();
	}
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Found unused DPDK Ring %s", name);

	return name;
}
#endif

string commands::add_port(string p, uint64_t dnumber, bool is_nf_port, int s, PortType port_type)
{
	int r = 0;
    	ssize_t nwritten;

	char ifac[BUF_SIZE];
	char read_buf[BUFFER_SIZE];

	string uuid_name;
	string port_name;

	map<string, unsigned int> ports;

	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "add_port(%s,type=%s)", p.c_str(), portTypeToString(port_type).c_str());

	//connect socket
	s = cmd_connect();

	//Default port name on the switch (may be overriden later for specific port types)
	stringstream pnos;
	pnos << dnumber << "_" << p;
	string port_name_on_switch = pnos.str();

	//Create the current name of a interface
	sprintf(ifac, "iface%d", rnumber);
	if (!is_nf_port) {
		uuid_name = p;
		/**
 		* Build name that is valid as UUID: no '.', no '_' ...
 		*/
		std::replace(uuid_name.begin(), uuid_name.end(), '.', 'p');
		std::replace(uuid_name.begin(), uuid_name.end(), '-', 'p');
		port_name = p;
		port_name_on_switch = p;
	} else {
		uuid_name = build_port_uuid_name(p, dnumber);

		/*create name of port --> lsiId_portName*/
		stringstream ss;
		ss << dnumber << "_" << p.c_str();
		port_name = ss.str();
	}

	root["method"] = "transact";

	params.push_back("Open_vSwitch");

	first_obj["op"] = "insert";
	first_obj["table"] = "Interface";

	/*Insert an Interface*/
	if (is_nf_port) {
		switch (port_type) {
		case IVSHMEM_PORT:
		case DPDKR_PORT:
#ifdef ENABLE_OVSDB_DPDK
			row["type"] = "dpdkr";
			port_name = find_free_dpdkr();
			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Port %s maps to DPDK Ring %s", p.c_str(), port_name.c_str());
			dpdkr_from_uuid.insert(map<string, string>::value_type(uuid_name, port_name));
			dpdkr_to_uuid.insert(map<string, string>::value_type(port_name, uuid_name));

			port_name_on_switch = port_name;
#else
			//XXX the next rows have to be removed when this plugin with support OvS-DPDK
			logger(ORCH_WARNING, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Currently supported by the OvS-DPDK plugin");
			assert(0 && "Currently supported by the OvS-DPDK plugin");
			throw OVSDBManagerException();
#endif
			break;
		case USVHOST_PORT:
		{
#ifdef ENABLE_OVSDB_DPDK
			row["type"] = "dpdkvhostuser";

			// Delete socket to be sure OVS can create it! (strange but needed)
			stringstream prep_usvhost_cmd;
			prep_usvhost_cmd << PREP_USVHOST_PORT << " " << port_name;
			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", prep_usvhost_cmd.str().c_str());
			int retVal = system(prep_usvhost_cmd.str().c_str());
			retVal = retVal >> 8;
			if(retVal == 0) {
				logger(ORCH_WARNING, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Failed to prepare 'usvhost' port %s", port_name.c_str());
				throw OVSDBManagerException();
			}
#else
			logger(ORCH_WARNING, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Currently supported by the OvS-DPDK plugin");
			assert(0 && "Currently supported by the OvS-DPDK plugin");
			throw OVSDBManagerException();
#endif
			break;
		}
		case VETH_PORT:
		{
			//In this case the veth pair needs to be created! The pair of names is stored; this information will be used when the VNF will be destroyed
			stringstream peer_port_name;
			peer_port_name << port_name << ".d";
			stringstream cmd_create_veth_pair;
			cmd_create_veth_pair << CREATE_VETH_PAIR << " " << port_name << " " << peer_port_name.str();
			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_create_veth_pair.str().c_str());

			int retVal = system(cmd_create_veth_pair.str().c_str());
			retVal = retVal >> 8;
			if(retVal == 0) {
				logger(ORCH_WARNING, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Failed to create 'veth' port");
				throw OVSDBManagerException();
			}

			//	The veth pair peer given to the container is the one with the port_name as derived from the NF-FG.
			//	As a result, the veth pair peer we add to OVS is the one with the decorated name: port_name.d
			peersNames[port_name] = peer_port_name.str();

			port_name = peer_port_name.str();
			break;
		}
		default:
			//We are here in case of type "vhost"
			assert(port_type == VHOST_PORT);
			row["type"] = "internal";
			break;
		}
	}
	else { // External ports
		if (p.compare(0, 4, "dpdk") == 0)
			row["type"] = "dpdk";
	}

	row["name"] = port_name.c_str();

	row["admin_state"] = "up";
	row["link_state"] = "up";
	row["ofport"] = rnumber;
	row["ofport_request"] = rnumber;

	first_obj["row"] = row;

	first_obj["uuid-name"] = ifac;

	params.push_back(first_obj);

	row.clear();
	first_obj.clear();

	/*Insert a port*/
	first_obj["op"] = "insert";
	first_obj["table"] = "Port";

	/*Insert a port*/
	row["name"] = port_name.c_str();

	iface.push_back("set");

	iface2.push_back("named-uuid");
	iface2.push_back(ifac);

	iface1.push_back(iface2);
	iface.push_back(iface1);

	row["interfaces"] = iface;

	first_obj["row"] = row;

	first_obj["uuid-name"] = uuid_name.c_str();

	params.push_back(first_obj);

	//insert this port name into port_l
	port_l[dnumber].push_back(port_name);

	row.clear();
	first_obj.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();

	first_obj["op"] = "update";
	first_obj["table"] = "Bridge";

	third_object.push_back("_uuid");
	third_object.push_back("==");

	fourth_object.push_back("uuid");
	fourth_object.push_back(switch_uuid[dnumber].c_str());

	third_object.push_back(fourth_object);
	where.push_back(third_object);

	first_obj["where"] = where;

	where.clear();

	for(list<string>::iterator u = port_uuid[dnumber].begin(); u != port_uuid[dnumber].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	for(list<string>::iterator u = vport_uuid[dnumber].begin(); u != vport_uuid[dnumber].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	for(list<string>::iterator u = gport_uuid[dnumber].begin(); u != gport_uuid[dnumber].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	port2.push_back("named-uuid");
	port2.push_back(uuid_name.c_str());

	port1.push_back(port2);

	port2.clear();

	/*Array with two elements*/
	i_array.push_back("set");

	i_array.push_back(port1);

	row["ports"] = i_array;

	first_obj["row"] = row;

	params.push_back(first_obj);

	second_obj.clear();

	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";

	/*Empty array [where]*/
	second_obj["where"] = where;

	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);

	ma.push_back(maa);

	second_obj["mutations"] = ma;

	params.push_back(second_obj);

	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );

	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	Value value;
    	read( read_buf, value );
    	Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		const std::string name = (*it).first;
		const Value &node = (*it).second;

		if (name == "result")
		{
			const Array &result = node.getArray();

			for(unsigned i=0;i<result.size();i++)
			{
				Object uuidNode = result[i].getObject();

				for (Object::const_iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
				{
					std::string name1 = (*it1).first;
					const Value &node1 = (*it1).second;

					if(name1 == "uuid"){
						const Array &stuff1 = node1.getArray();

						if(i==1){
							port_uuid[dnumber].push_back(stuff1[1].getString());
						}
					} else if(name1 == "details"){
						logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
						throw commandsException();
					}
				}
			}
		}
	}

	root.clear();
	params.clear();

	//Increment transaction id
	tid++;

	rnumber++;

	//disconnect socket
    	cmd_disconnect(s);

	/**
	*	Considerations:
	*
	*	On some systems, ports created by OvS are down, although we esplicitly say that they must be created up.
	*	We noted that the same proplem appears outside of the universal node, if directly use the ovs-vsctl command.
	*
	*	The port in OvS appears sometime after that the port is created to OVSDB. Than it may happen (and we experienced
	*	it) that, when the VNF has to created, the port is still not there.
	*
	*	The segmentation offload and something else must be disabled on ports for VNFs.
	*
	*	What we do here:
	*
	*	Call a bash script that brings up the interface and disebled offloads on it. The script pools untill such interface
	*	is created, thus ensuring that, when the VNF will be created, the ports are already attached to OvS.
	*/
    	if(is_nf_port && ((port_type == VHOST_PORT) || (port_type == VETH_PORT)))
    	{
    		stringstream command;
		command << ACTIVATE_INTERFACE << " " << port_name;
		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());
		int retVal = system(command.str().c_str());
		retVal = retVal >> 8;

		assert(retVal == 0);
		if(retVal != 0)
		{
			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "This cannot happen. It is here just for the compiler.");
			assert(0);
		}
	}

	return port_name_on_switch;
}

void commands::add_endpoint(uint64_t dpi, char local_ip[BUF_SIZE], char remote_ip[BUF_SIZE], char key[BUF_SIZE], char port_name[BUF_SIZE], char ifac[BUF_SIZE], int s, char is_safe[BUF_SIZE])
{
	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "add_endpoint(local ip=%s,remote ip=%s,key=%s,port name=%s,iface=%s)",local_ip,remote_ip,key,port_name,ifac);

	ssize_t nwritten;

	char read_buf[BUFFER_SIZE] = "";

	int r = 0;

	locale loc;

	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	//connect socket
	s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");

	first_obj["op"] = "insert";
	first_obj["table"] = "Interface";

	/*Insert an Interface*/
	row["name"] = port_name;
	//test if gre tunnel required is safe or unsafe
	if(strcmp(is_safe, "true") == 0)
		row["type"] = "ipsec_gre";
	else
		row["type"] = "gre";

	row["admin_state"] = "up";
	row["link_state"] = "up";
	row["ofport"] = rnumber;
	row["ofport_request"] = rnumber;

	/*Add options local_ip, remote_ip and key*/
	peer.push_back("map");

	//test if GRE tunnel required is safe (true) or unsafe (false)
	if(strcmp(is_safe, "true") == 0)
	{
		peer2.push_back("certificate");
		peer2.push_back(this->ipsec_certificate);
		peer1.push_back(peer2);
		peer2.clear();

		peer2.push_back("pmtud");
		peer2.push_back("false");
		peer1.push_back(peer2);
		peer2.clear();

		peer2.push_back("psk");
		peer2.push_back("test");
		peer1.push_back(peer2);
		peer2.clear();
	}

	peer2.push_back("local_ip");
	peer2.push_back(local_ip);
	peer1.push_back(peer2);
	peer2.clear();

	peer2.push_back("remote_ip");
	peer2.push_back(remote_ip);
	peer1.push_back(peer2);
	peer2.clear();

	peer2.push_back("key");
	peer2.push_back(key);
	peer1.push_back(peer2);
	peer2.clear();

	peer.push_back(peer1);

    	row["options"] = peer;

	first_obj["row"] = row;

	first_obj["uuid-name"] = ifac;

	params.push_back(first_obj);

	row.clear();
	first_obj.clear();

	first_obj["op"] = "insert";
	first_obj["table"] = "Port";

	/*Insert a port*/
	row["name"] = port_name;

	iface.push_back("set");

	iface2.push_back("named-uuid");
	iface2.push_back(ifac);

	iface1.push_back(iface2);
	iface.push_back(iface1);

	row["interfaces"] = iface;

	first_obj["row"] = row;

	first_obj["uuid-name"] = port_name;

	params.push_back(first_obj);

	row.clear();
	first_obj.clear();
	peer.clear();
	peer1.clear();
	peer2.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();

	first_obj["op"] = "update";
	first_obj["table"] = "Bridge";

	third_object.push_back("_uuid");
	third_object.push_back("==");

	fourth_object.push_back("uuid");
	fourth_object.push_back(switch_uuid[dpi].c_str());

	third_object.push_back(fourth_object);
	where.push_back(third_object);

	first_obj["where"] = where;

	where.clear();

	for(list<string>::iterator u = port_uuid[dpi].begin(); u != port_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	for(list<string>::iterator u = vport_uuid[dpi].begin(); u != vport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	for(list<string>::iterator u = gport_uuid[dpi].begin(); u != gport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	port2.push_back("named-uuid");
	port2.push_back(port_name);

	port1.push_back(port2);

	port2.clear();

	/*Array with two elements*/
	i_array.push_back("set");

	i_array.push_back(port1);

	row["ports"] = i_array;

	first_obj["row"] = row;

	params.push_back(first_obj);

	second_obj.clear();

	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";

	/*Empty array [where]*/
	second_obj["where"] = where;

	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);

	ma.push_back(maa);

	second_obj["mutations"] = ma;

	params.push_back(second_obj);

	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );

	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Result of query: ");

	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());

	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Response json: ");

	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	Value value;
	read( read_buf, value );
	Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		std::string name = (*it).first;
		const Value &node = it->second;
		if (name == "result")
		{
			const Array &result = node.getArray();

			for(unsigned i=0;i<result.size();i++)
			{
				Object uuidNode = result[i].getObject();

				for (Object::iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
				{
					std::string name1 = (*it1).first;
					Value &node1 = (*it1).second;

					if(name1 == "uuid")
					{
						const Array &stuff1 = node1.getArray();

						if(i==1){
							gport_uuid[dpi].push_back(stuff1[i].getString());
						}
					} else if(name1 == "details"){
						logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
						throw commandsException();
					}
				}
			}
		}
	}

	root.clear();
	params.clear();

	//Increment transaction id
	tid++;

	rnumber++;

	//disconnect socket
    	cmd_disconnect(s);
}

void commands::cmd_editconfig_lsi_delete(uint64_t dpi, int s)
{
	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Deleting LSI with DPI '%u' switch '%s'",dpi, switch_id[dpi].c_str());

    	ssize_t nwritten = 0;

	int r = 0;

	char read_buf[4096] = "";

	map<string, unsigned int> ports;

	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	/*for all virtual link ports, destroy it*/
	for(list<uint64_t>::iterator i = virtual_link_id[switch_id[dpi]].begin(); i != virtual_link_id[switch_id[dpi]].end(); i++)
	{
		//*i -> the port on the bridge we are deleting (is it in vl_PortIDtoSwitchID? )

	//	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Considering virtual link ID %d", *i);

		cmd_delete_virtual_link(vl_PortIDtoSwitchID[(*i)], (*i), s);
		cmd_delete_virtual_link(vl_PortIDtoSwitchID[vl_LocalPortIDtoRemotePortID[(*i)]], vl_LocalPortIDtoRemotePortID[(*i)], s);
	}

	// Delete DPDK Ring related entries of dpdkr ports
	for(list<string>::iterator it = port_l[dpi].begin(); it != port_l[dpi].end(); ++it) {
		map<string, string>::iterator f_it = dpdkr_from_uuid.find(*it);
		if (f_it != dpdkr_from_uuid.end()) {
			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "\tRemoving entries for DPDK Ring %s", f_it->second.c_str());
			dpdkr_to_uuid.erase(f_it->second);
			dpdkr_from_uuid.erase(f_it);
		}
	}

	port_l.erase(dpi);
	vport_l.erase(dpi);
	port_uuid.erase(dpi);
	vport_uuid.erase(dpi);
	virtual_link_id[switch_id[dpi]].clear();

	//connect socket
    s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");

	first_obj["op"] = "update";
	first_obj["table"] = "Open_vSwitch";

	first_obj["where"] = where;

	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Removing dpi: %d", dpi);
	switch_uuid.erase(dpi);

	for(map<uint64_t, string>::iterator sww = switch_uuid.begin(); sww != switch_uuid.end(); sww++)
	{
		port2.push_back("uuid");
		port2.push_back(sww->second);
		port1.push_back(port2);

		port2.clear();
	}

	/*Array with two elements*/
	i_array.push_back("set");

	i_array.push_back(port1);

	row["bridges"] = i_array;

	first_obj["row"] = row;

	params.push_back(first_obj);

	second_obj.clear();

	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";

	/*Empty array [where]*/
	second_obj["where"] = where;

	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);

	ma.push_back(maa);

	second_obj["mutations"] = ma;

	params.push_back(second_obj);

	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );

	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	root.clear();
	params.clear();

	//Increment transaction id
	tid++;

	//disconnect socket
    	cmd_disconnect(s);
}

AddNFportsOut *commands::cmd_editconfig_NFPorts(AddNFportsIn anpi, int socketNumber)
{
	list<struct nf_port_info> portInfo = anpi.getNetworkFunctionsPorts();//each element of portInfo contains the port name and the port type
	uint64_t datapathNumber = anpi.getDpid();
	map<string, unsigned int> port_names_and_id;

	list<string> ports_name_on_switch;
	map<string, unsigned int> ports;

	for(list<struct nf_port_info>::iterator pinfo = portInfo.begin(); pinfo != portInfo.end(); pinfo++)
	{
		string nameOnSwitch = add_port(pinfo->port_name, datapathNumber, true, socketNumber, pinfo->port_type);
		ports_name_on_switch.push_back(nameOnSwitch);
		ports[pinfo->port_name] = rnumber - 1;
		port_names_and_id[nameOnSwitch] = rnumber - 1;

		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Port '%s' has name '%s' on the switch.",(pinfo->port_name).c_str(),nameOnSwitch.c_str());
	}

	AddNFportsOut *anf = new AddNFportsOut(anpi.getNfId(), ports, ports_name_on_switch, port_names_and_id);

	return anf;
}

AddEndpointOut *commands::cmd_editconfig_endpoint(AddEndpointIn aepi, int s)
{
	AddEndpointOut *apf = NULL;

	char port_name[BUF_SIZE], ifac[BUF_SIZE];
	char local_ip[BUF_SIZE];
	char remote_ip[BUF_SIZE];
	char key[BUF_SIZE];
	char safe[BUF_SIZE];

	string id = aepi.getEPname();
	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "add_endpoint(name=%s)",id.c_str());

	/*save the params of gre tunnel*/
	vector<string> l_param = aepi.getEPparam();

	strcpy(key, l_param[1].c_str());
	strcpy(local_ip, l_param[0].c_str());
	strcpy(remote_ip, l_param[2].c_str());
	strcpy(safe, l_param[4].c_str());

	sprintf(port_name, "gre%d", gnumber);

	sprintf(ifac, "iface%d", rnumber);

	gnumber++;

	//create endpoint
	add_endpoint(aepi.getDpid(), local_ip, remote_ip, key, port_name, ifac, s, safe);

	endpoint_l[aepi.getDpid()].push_back(id);

	apf = new AddEndpointOut(aepi.getEPname(), rnumber-1);

	return apf;
}

void commands::cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi, int s)
{
    	ssize_t nwritten;

	char read_buf[BUFFER_SIZE];

	int r = 0;

	map<string, unsigned int> ports;

	set<string> nfp = dnpi.getNFports();

	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	//connect socket
    	s = cmd_connect();

	if(nfp.size() != 0){

		list<string> portsToBeRemoved;

		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Network function ports to be removed from the switch:");
		for(set<string>::iterator port = nfp.begin(); port != nfp.end(); port++)
		{
			if(peersNames.count(*port) != 0)
			{
				//this port is a VETH
				portsToBeRemoved.push_back(peersNames[*port]);
				logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "\t* %s",peersNames[*port].c_str());
			}
			else
			{
				portsToBeRemoved.push_back(*port);
				logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "\t* %s",port->c_str());
			}
		}

		root["method"] = "transact";

		params.push_back("Open_vSwitch");

		first_obj["op"] = "update";
		first_obj["table"] = "Bridge";

		third_object.push_back("_uuid");
		third_object.push_back("==");

		fourth_object.push_back("uuid");
		fourth_object.push_back(switch_uuid[dnpi.getDpid()].c_str());

		third_object.push_back(fourth_object);
		where.push_back(third_object);

		first_obj["where"] = where;

		where.clear();

		list<string>::iterator port_uuid_on_switch = port_uuid[dnpi.getDpid()].begin();

		/*for each port in the list of the getNetworkFunctionsPorts*/
		//for(set<string>::iterator p = nfp.begin(); p != nfp.end(); p++){
		for(list<string>::iterator p = portsToBeRemoved.begin(); p != portsToBeRemoved.end(); p++)
		{
			string port_to_be_removed = (*p);

			port_uuid_on_switch = port_uuid[dnpi.getDpid()].begin();
			//should be search in port_l, p....if find it take the index and remove it from the set port-uuid[pi]
			for(list<string>::iterator u = port_l[dnpi.getDpid()].begin(); u != port_l[dnpi.getDpid()].end(); u++){
				string port_name_on_switch = (*u);

				if(port_name_on_switch.compare(port_to_be_removed) == 0){
					port_l[dnpi.getDpid()].remove(port_name_on_switch);
					port_uuid[dnpi.getDpid()].remove((*port_uuid_on_switch));
					break;
				}
				port_uuid_on_switch++;
			}
		}

		for(list<string>::iterator u = port_uuid[dnpi.getDpid()].begin(); u != port_uuid[dnpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");

			port2.push_back((*u));

			port1.push_back(port2);

			port2.clear();
		}

		for(list<string>::iterator u = vport_uuid[dnpi.getDpid()].begin(); u != vport_uuid[dnpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");

			port2.push_back((*u));

			port1.push_back(port2);

			port2.clear();
		}

		for(list<string>::iterator u = gport_uuid[dnpi.getDpid()].begin(); u != gport_uuid[dnpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");

			port2.push_back((*u));

			port1.push_back(port2);

			port2.clear();
		}

		/*Array with two elements*/
		i_array.push_back("set");

		i_array.push_back(port1);

		row["ports"] = i_array;

		first_obj["row"] = row;

		params.push_back(first_obj);

		second_obj.clear();

		/*Object with four items [op, table, where, mutations]*/
		second_obj["op"] = "mutate";
		second_obj["table"] = "Open_vSwitch";

		/*Empty array [where]*/
		second_obj["where"] = where;

		/*Array with two element*/
		maa.push_back("next_cfg");
		maa.push_back("+=");
		maa.push_back(1);

		ma.push_back(maa);

		second_obj["mutations"] = ma;

		params.push_back(second_obj);

		ma.clear();
		maa.clear();
		row.clear();
		where.clear();
		first_obj.clear();
		second_obj.clear();
		third_object.clear();
		fourth_object.clear();
		i_array.clear();
		iface.clear();
		iface1.clear();
		iface2.clear();
		port1.clear();
		port2.clear();
		root["params"] = params;

		root["id"] = tid;

		stringstream ss;
 		write_formatted(root, ss );

		nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
		if (nwritten == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			throw commandsException();
		}

    		r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (r == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			throw commandsException();
		}
		read_buf[r] = '\0';

		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

		root.clear();
		params.clear();

		//Increment transaction id
		tid++;

		cmd_disconnect(s);
	}
}

void commands::cmd_editconfig_endpoint_delete(DestroyEndpointIn depi, int s){

	ssize_t nwritten;

	char read_buf[4096] = "";

	int r = 0;

	locale loc;

//	map<string, unsigned int> ports;

	string ep_name = depi.getEPname();
	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "endpoint_delete(name=%s)",ep_name.c_str());

	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	//connect socket
	s = cmd_connect();

	if(ep_name.compare("") != 0){

		root["method"] = "transact";

		params.push_back("Open_vSwitch");

		first_obj["op"] = "update";
		first_obj["table"] = "Bridge";

		third_object.push_back("_uuid");
		third_object.push_back("==");

		fourth_object.push_back("uuid");
		fourth_object.push_back(switch_uuid[depi.getDpid()].c_str());

		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "switch_uuid[depi.getDpid()]=%s",switch_uuid[depi.getDpid()].c_str());

		third_object.push_back(fourth_object);
		where.push_back(third_object);

		first_obj["where"] = where;

		where.clear();

		list<string>::iterator uu = gport_uuid[depi.getDpid()].begin(); //list of uuid related to tunnel GRE
		assert(gport_uuid.count(depi.getDpid()) != 0);
//		uu = gport_uuid[depi.getDpid()].begin();

		//should be search in endpoint_l, p....if find it take the index and remove it from the set endpoint-uuid[pi]
		list<string> ep_l = endpoint_l[depi.getDpid()];
		assert(endpoint_l.count(depi.getDpid()) != 0);
		//iterate on all the gre tunnels that are part of this bridge
		for(list<string>::iterator u = ep_l.begin(); u != ep_l.end(); u++){
			string s = (*u);
			if(s.compare(ep_name) == 0){
				gport_uuid[depi.getDpid()].remove((*uu));
				ep_l.erase(u);
				endpoint_l[depi.getDpid()] = ep_l;
				break;
			}
			uu++;
		}

		for(list<string>::iterator u = port_uuid[depi.getDpid()].begin(); u != port_uuid[depi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");

			port2.push_back((*u));

			port1.push_back(port2);

			port2.clear();
		}

		for(list<string>::iterator u = vport_uuid[depi.getDpid()].begin(); u != vport_uuid[depi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");

			port2.push_back((*u));

			port1.push_back(port2);

			port2.clear();
		}

		for(list<string>::iterator u = gport_uuid[depi.getDpid()].begin(); u != gport_uuid[depi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");

			port2.push_back((*u));

			port1.push_back(port2);

			port2.clear();
		}

		/*Array with two elements*/
		i_array.push_back("set");

		i_array.push_back(port1);

		row["ports"] = i_array;

		first_obj["row"] = row;

		params.push_back(first_obj);

		second_obj.clear();

		/*Object with four items [op, table, where, mutations]*/
		second_obj["op"] = "mutate";
		second_obj["table"] = "Open_vSwitch";

		/*Empty array [where]*/
		second_obj["where"] = where;

		/*Array with two element*/
		maa.push_back("next_cfg");
		maa.push_back("+=");
		maa.push_back(1);

		ma.push_back(maa);

		second_obj["mutations"] = ma;

		params.push_back(second_obj);

		ma.clear();
		maa.clear();
		row.clear();
		where.clear();
		first_obj.clear();
		second_obj.clear();
		third_object.clear();
		fourth_object.clear();
		i_array.clear();
		iface.clear();
		iface1.clear();
		iface2.clear();
		port1.clear();
		port2.clear();
		root["params"] = params;

		root["id"] = tid;

		stringstream ss;
 		write_formatted(root, ss );

		nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
		if (nwritten == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			throw commandsException();
		}

    		r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (r == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			throw commandsException();
		}

		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

		root.clear();
		params.clear();

		//Increment transaction id
		tid++;

		cmd_disconnect(s);
	}
}

AddVirtualLinkOut *commands::cmd_addVirtualLink(AddVirtualLinkIn avli, int s)
{
	/*struct to return*/
	AddVirtualLinkOut *avlo = NULL;

	char vrt[BUF_SIZE], trv[BUF_SIZE];
	char ifac[BUF_SIZE];

	list<pair<unsigned int, unsigned int> > virtual_links;

	strcpy(vrt, "vport");

	/*create virtual link*/
	sprintf(vrt, "vport%d", pnumber);
	pnumber++;

	/*create virtual link*/
	sprintf(trv, "vport%d", pnumber);
	pnumber++;

	//Create the current name of a interface
	sprintf(ifac, "iface%d", rnumber);

	//create first endpoint
	cmd_add_virtual_link(vrt, trv, ifac, avli.getDpidA(), s);

	//Create the current name of a interface
	sprintf(ifac, "iface%d", rnumber);

	//create second endpoint
	cmd_add_virtual_link(trv, vrt, ifac, avli.getDpidB(), s);

	/*store the information [bridge name, list of peer port]*/
	virtual_link_id[switch_id[avli.getDpidA()]].push_back(rnumber-2);

	port_id[rnumber-2] = vrt;
	port_id[rnumber-1] = trv;

	/*store the information [switch_id, port_id]*/

	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Creating the following virtual link:");
	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, " Port ID '%d' on switch ID '%d'",rnumber-2,avli.getDpidA());
	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, " Port ID '%d' on switch ID '%d'",rnumber-1,avli.getDpidB());
	vl_PortIDtoSwitchID[rnumber-2] = avli.getDpidA();
	vl_PortIDtoSwitchID[rnumber-1] = avli.getDpidB();

	vl_LocalPortIDtoRemotePortID[rnumber-2] = rnumber-1;

	//insert this port name into port_n
	vport_l[avli.getDpidA()].push_back(vrt);
	//insert this port name into port_n
	vport_l[avli.getDpidB()].push_back(trv);

	/*peer name of the bridge name vrt is trv*/
	peer_n[trv] = vrt;

	/*peer name of the bridge name trv is vrt*/
	peer_n[vrt] = trv;

	avlo = new AddVirtualLinkOut(rnumber-2, rnumber-1);

	return avlo;
}

void commands::cmd_add_virtual_link(string vrt, string trv, char ifac[BUF_SIZE], uint64_t dpi, int s){

    ssize_t nwritten;

	char read_buf[BUFFER_SIZE];

	int r = 0;

	map<string, unsigned int> ports;

	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	//connect socket
    	s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");

	first_obj["op"] = "insert";
	first_obj["table"] = "Interface";

	/*Insert an Interface*/
	row["name"] = vrt;
	row["type"] = "patch";

	row["admin_state"] = "up";
	row["link_state"] = "up";
	row["ofport"] = rnumber;
	row["ofport_request"] = rnumber;

	/*Add options peer*/
	peer.push_back("map");

	peer2.push_back("peer");
	peer2.push_back(trv);

	peer1.push_back(peer2);
	peer.push_back(peer1);

    	row["options"] = peer;

	first_obj["row"] = row;

	first_obj["uuid-name"] = ifac;

	params.push_back(first_obj);

	row.clear();
	first_obj.clear();

	first_obj["op"] = "insert";
	first_obj["table"] = "Port";

	/*Insert a port*/
	row["name"] = vrt;

	iface.push_back("set");

	iface2.push_back("named-uuid");
	iface2.push_back(ifac);

	iface1.push_back(iface2);
	iface.push_back(iface1);

	row["interfaces"] = iface;

	first_obj["row"] = row;

	first_obj["uuid-name"] = vrt;

	params.push_back(first_obj);

	row.clear();
	first_obj.clear();
	peer.clear();
	peer1.clear();
	peer2.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();

	first_obj["op"] = "update";
	first_obj["table"] = "Bridge";

	third_object.push_back("_uuid");
	third_object.push_back("==");

	fourth_object.push_back("uuid");
	fourth_object.push_back(switch_uuid[dpi].c_str());

	third_object.push_back(fourth_object);
	where.push_back(third_object);

	first_obj["where"] = where;

	where.clear();

	for(list<string>::iterator u = port_uuid[dpi].begin(); u != port_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	for(list<string>::iterator u = vport_uuid[dpi].begin(); u != vport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	for(list<string>::iterator u = gport_uuid[dpi].begin(); u != gport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	port2.push_back("named-uuid");
	port2.push_back(vrt);

	port1.push_back(port2);

	port2.clear();

	/*Array with two elements*/
	i_array.push_back("set");

	i_array.push_back(port1);

	row["ports"] = i_array;

	first_obj["row"] = row;

	params.push_back(first_obj);

	second_obj.clear();

	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";

	/*Empty array [where]*/
	second_obj["where"] = where;

	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);

	ma.push_back(maa);

	second_obj["mutations"] = ma;

	params.push_back(second_obj);

	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );

	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	Value value;
    	read( read_buf, value );
   	Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		std::string name = (*it).first;
		const Value &node = it->second;
		if (name == "result")
		{
			const Array &result = node.getArray();

			for(unsigned i=0;i<result.size();i++)
			{
				Object uuidNode = result[i].getObject();

				for (Object::iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
				{
					std::string name1 = (*it1).first;
					Value &node1 = (*it1).second;

					if(name1 == "uuid")
					{
						const Array &stuff1 = node1.getArray();

						if(i==1){
							vport_uuid[dpi].push_back(stuff1[i].getString());
						}
					} else if(name1 == "details"){
						logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
						throw commandsException();
					}
				}
			}
		}
	}

	root.clear();
	params.clear();

	//Increment transaction id
	tid++;

	rnumber++;

	//disconnect socket
    	cmd_disconnect(s);
}

void commands::cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli, int s){

	char vrt[BUF_SIZE] = "", trv[BUF_SIZE] = "";

	list<pair<unsigned int, unsigned int> > virtual_links;

	//destroy first endpoint
	cmd_delete_virtual_link(dvli.getDpidA(), dvli.getIdA(), s);

	//destroy second endpoint
	cmd_delete_virtual_link(dvli.getDpidB(), dvli.getIdB(), s);

	/*erase information*/
	vl_PortIDtoSwitchID.erase(dvli.getIdA());
	vl_PortIDtoSwitchID.erase(dvli.getIdB());

	//remove this port name from port_n
	vport_l[dvli.getDpidA()].remove(vrt);
	vport_l[dvli.getDpidB()].remove(trv);

	//remove this port id from vport id list
	virtual_link_id[switch_id[dvli.getDpidA()]].remove(dvli.getIdA());
	virtual_link_id[switch_id[dvli.getDpidB()]].remove(dvli.getIdB());

}

void commands::cmd_delete_virtual_link(uint64_t dpi, uint64_t idp, int s)
{
	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "cmd_delete_virtual_link(dpi=%d, ipd=%d)",dpi,idp);

	ssize_t nwritten;

	char read_buf[4096] = "";

	int r = 0;

	map<string, unsigned int> ports;

	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;

	Array third_object;
	Array fourth_object;

	//connect socket
    	s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");

	first_obj["op"] = "update";
	first_obj["table"] = "Bridge";

	third_object.push_back("_uuid");
	third_object.push_back("==");

	fourth_object.push_back("uuid");
	fourth_object.push_back(switch_uuid[dpi].c_str());

	third_object.push_back(fourth_object);
	where.push_back(third_object);

	first_obj["where"] = where;

	where.clear();

	//search the port-id and after erase this port-uuid
	list<string>::iterator uu = vport_uuid[dpi].begin();

	//should be search in port_l, p....if find it take the index and remove it from the set port-uuid[pi]
	for(list<string>::iterator u = vport_l[dpi].begin(); u != vport_l[dpi].end(); u++){
		string sss = (*u);
		if(port_id[idp].compare(sss) == 0){
			vport_uuid[dpi].remove(*uu);
			vport_l[dpi].remove(*u);
			break;
		}
		uu++;
	}

	//insert normal ports
	for(list<string>::iterator u = port_uuid[dpi].begin(); u != port_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	//insert vports
	for(list<string>::iterator u = vport_uuid[dpi].begin(); u != vport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	//insert gport
	for(list<string>::iterator u = gport_uuid[dpi].begin(); u != gport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");

		port2.push_back((*u));

		port1.push_back(port2);

		port2.clear();
	}

	/*Array with two elements*/
	i_array.push_back("set");

	i_array.push_back(port1);

	row["ports"] = i_array;

	first_obj["row"] = row;

	params.push_back(first_obj);

	second_obj.clear();

	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";

	/*Empty array [where]*/
	second_obj["where"] = where;

	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);

	ma.push_back(maa);

	second_obj["mutations"] = ma;

	params.push_back(second_obj);

	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );

	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	root.clear();
	params.clear();

	//Increment transaction id
	tid++;

	//disconnect socket
    cmd_disconnect(s);
}
