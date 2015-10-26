#include "ovsdb_manager.h"
#include "ovsdb_constants.h"

struct in_addr sIPaddr1;/* struct IP addr server*/
char ErrBuf[BUFFER_SIZE];

int rnumber = 1, rnumber_new = 1;
uint64_t dnumber = 1;
int pnumber = 1, nfnumber = 0;

int id = 0;

/*map use to obtain name of switch from id*/
map<uint64_t, string> switch_id;
/*map use to obtain uuid of switch from name*/
map<uint64_t, string> switch_uuid;
/*map use to obtain vport peer of switch from vport name*/
map<string, string> peer_n;
/*map use to obtain list of ports from bridge-id*/
map<uint64_t, list<string> > port_l;
/*map use to obtain list of vports from bridge-id*/
map<uint64_t, list<string> > vport_l;
/*map use to obtain list of port uuid from bridge-id*/
map<uint64_t, list<string> > port_uuid;
/*map use to obtain list of vport uuid from bridge-id*/
map<uint64_t, list<string> > vport_uuid;
/*map use to obtain virtual link id from bridge name*/
map<string, list<uint64_t> > virtual_link_id;
/*map use to obtain name of port from port_id*/
map<uint64_t, string> port_id;
/*map use to id of switch from vport_id*/
map<uint64_t, uint64_t> vl_id;

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
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Invalid IP address.");
		throw commandsException();
	}
	
	if (sscanf(SOCKET_PORT, "%" SCNu16, &tport_h)!=1){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Invalid number of port.");
		throw commandsException();
	}
	
	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AF_INET;
	Hints.ai_socktype= SOCK_STREAM;
	
	if (sock_initaddress (SOCKET_IP, SOCKET_PORT, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  SOCKET_IP, SOCKET_PORT, ErrBuf);
		throw commandsException();
	}

	if ( (s=sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Connection failure!");
		throw commandsException();
	}
	
	return s;
}

int commands::cmd_disconnect(int s){
	char ErrBuf[BUFFER_SIZE];

	if(sock_close(s, ErrBuf, sizeof(ErrBuf)) != 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Closing socket failed.");
		throw commandsException();
	}
	
	return EXIT_SUCCESS;
}

/*
*	@cli = struct CreateLsiIn
*/
CreateLsiOut* commands::cmd_editconfig_lsi (CreateLsiIn cli, int s){

    unsigned int i=0;	
    int nwritten = 0;
    
    ssize_t r = 0;
	char read_buf[4096] = "";
	
	/*Information for CreateLsiOut to return*/
	CreateLsiOut *clo = NULL;
	map<string,unsigned int> physical_ports;
	map<string,map<string, unsigned int> >  network_functions_ports;
	list<pair<unsigned int, unsigned int> > virtual_links;
	
    int dnumber_new = 0, nfnumber_old = 0;
    
    //list of ports
	list<string> ports = cli.getPhysicalPortsName();

	//list of nf	
	set<string> nfs = cli.getNetworkFunctionsName();

	//list of nft	
	map<string,nf_t> nf_type = cli.getNetworkFunctionsType(); 	

	//list of remote LSI
	list<uint64_t> vport = cli.getVirtualLinksRemoteLSI();
	
	//list of remote LSI
	char rp[2048][64];
	
	//Local variables
	const char *peer_name = "";

	uint64_t port_id_1 = 0, port_id_2 = 0;
	
	unsigned int nf_size = 0;

	char sw[64] = "Bridge", tcp_s[64] = "tcp:", ctr[64] = "ctrl", vrt[64] = "VirtualPort", trv[64] = "vport";

	char temp[64] = "", tmp[64] = "", of_version[64] = "";
    
    /*set the OpenFlow version*/
	switch(OFP_VERSION)
	{
		case OFP_10:
			strcpy(of_version, "OpenFlow10");
			break;
		case OFP_12:
			strcpy(of_version, "OpenFlow12");
			break;
		case OFP_13:
			strcpy(of_version, "OpenFlow13");
			break;
	}
    
    //connect socket
    s = cmd_connect();
    
    /*Root object contained three object [method, params, id]*/
    Object root;
    root["method"] = "transact";

    Array params;
    params.push_back("Open_vSwitch");
    
    Object first_obj;
    Object row;
    Array iface;
    Array iface1;
    Array iface2;
	
	//Create Bridge
    /*create current name of a bridge "Bridge+dnumber"*/
	sprintf(temp, "%" PRIu64, dnumber);
	strcat(sw, temp);
	
	/*fill the map switch_id*/
	switch_id[dnumber] = string(sw);
	
	Array peer;
    Array peer1;
    Array peer2;
    
    int l = 0;
    
    //Create controller
    /*Create the current target of a controller*/
	strcat(tcp_s, cli.getControllerAddress().c_str());
	strcat(tcp_s, ":");
	strcat(tcp_s, cli.getControllerPort().c_str());
	strcpy(temp, tcp_s);
	
	first_obj["op"] = "insert";
    first_obj["table"] = "Controller";
    
    /*Insert a Controller*/
    row["target"] = temp;
    row["local_ip"] = "127.0.0.1";
    row["connection_mode"] = "out-of-band";
    row["is_connected"] = true;
    
    first_obj["row"] = row;
    
    //Create the current name of a controller --> ctrl+dnumber
    sprintf(temp, "%s%" PRIu64, ctr, dnumber);
    
    first_obj["uuid-name"] = temp;
    
    params.push_back(first_obj);
    
    row.clear();
    first_obj.clear();
    
	first_obj["op"] = "insert";
    first_obj["table"] = "Bridge";
    
    /*Insert a bridge*/
    row["name"] = sw;
    
    Array port;
	Array port1;
	Array port2;
	
	port.push_back("set");
    
	port.push_back(port1);
	
	row["ports"] = port;
	
	port1.clear();
	port.clear();
    
    Array ctrl;
    ctrl.push_back("set");

	Array ctrl1;
	
	Array ctrl2;

	//Create the current name of a controller
	sprintf(tmp, "%s%" PRIu64, ctr, dnumber);
	
	ctrl2.push_back("named-uuid");
	ctrl2.push_back(tmp);
	
	ctrl1.push_back(ctrl2);
	 
	ctrl.push_back(ctrl1);
    
    row["controller"] = ctrl;
    
	peer.push_back("map");
	
	peer2.push_back("disable-in-band");
	peer2.push_back("true");
			
	peer1.push_back(peer2);
	peer.push_back(peer1);
    
    row["other_config"] = peer;
    
    row["protocols"] = of_version;
    
    first_obj["row"] = row;
    
    first_obj["uuid-name"] = sw;
    
    params.push_back(first_obj);
    
    row.clear();
    first_obj.clear();
    port.clear();
    port1.clear();
    port2.clear();
    ctrl.clear();
    ctrl1.clear();
    ctrl2.clear();
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
    
    /*Array  with three elements*/
    Array m_array;
    m_array.push_back("bridges");
    m_array.push_back("insert");
    
    /*Array with two elements*/
    Array i_array;
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

	root["id"] = id;

	w_array.clear();
	m_array.clear();
	i_array.clear();
	s_array.clear();
	a_array.clear();
	
	//Increment transaction id
	id++;
    
    string *strr = new string[256];
    
    stringstream ss;
 	write_formatted(root, ss);
    
    nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}
    
    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

 	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
    
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
    
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
    
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);

	//Read json response
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
					} else if(name1 == "error"){
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Syntax error in Json request.");
						throw commandsException();
					}
				}	
     		}	
        }
	} 

	//store the switch-uuid
    switch_uuid[dnumber] = strr[i-2];

	/*create ports*/
	if(ports.size() !=0){
	
		ports.reverse();
	
		for(list<string>::iterator p = ports.begin(); p != ports.end(); p++)
		{
			add_ports(rnumber, (*p), dnumber, 0, s);
			
			port_l[dnumber].push_back((*p).c_str());
			physical_ports[(*p)] = rnumber_new;
			
			rnumber++;
			rnumber_new++;
		}
	}
	
	/*Create interfaces related by the nf ports*/
    if(nfs.size() != 0){
        		
		/*for each network function port in the list of nfs*/
		for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		{
			list<string> nfs_ports = cli.getNetworkFunctionsPortNames(*nf);
			
			map<string,unsigned int> n_ports_1;
			
			nfs_ports.reverse();
			
			/*for each network function port in the list of nfs_ports*/
			for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){
				add_ports(rnumber, (*nfp), dnumber, 1, s);
				
				locale loc;	
				port2.push_back("named-uuid");
				string str = (*nfp);
				
				for (string::size_type i=0; i<str.length(); ++i)
    				str[i] = tolower(str[i], loc);
    				
				strcpy(tmp, (char *)(str).c_str());
				sprintf(temp, "%" PRIu64, dnumber);
				strcat(tmp, "b");
				strcat(tmp, temp);
				strcpy(temp, tmp);
				
				for(unsigned int j=0;j<strlen(temp);j++){
					if(temp[j] == '_')
						temp[j] = 'p';
				}
				
				//insert this port name into port_n
				port_l[dnumber].push_back(temp);
				
				/*fill the map ports*/
				n_ports_1[(*nfp)] = rnumber_new;

				rnumber++;
				rnumber_new++;
				
				nf_size++;
			}
			
			/*fill the network_functions_ports*/
			network_functions_ports[(*nf)] = n_ports_1;
		}
	}
	
	/*Create interfaces related by the vlink ports*/
	if(vport.size() != 0)
	{	
		nfnumber_old = pnumber;
	
		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			strcpy(vrt, "vport");
			strcpy(trv, "vport");
	    	char ifac[64] = "iface";
	    	
			sprintf(temp, "%d", pnumber);
			strcat(vrt, temp);
			
			port_id_1 = pnumber;
			
			pnumber++;
			
			port_id_2 = pnumber;
			
			sprintf(temp, "%d", pnumber);
			strcat(trv, temp);

			peer_n[trv] = vrt;
				
			strcpy(rp[l], trv);	
			
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			cmd_add_virtual_link(vrt, trv, ifac, dnumber, s);
			
			port_id[port_id_1] = vrt;
			port_id[port_id_2] = trv;
			
			virtual_link_id[sw].push_back(port_id_2);
			
			vport_l[dnumber].push_back(vrt);
				
			vl_id[port_id_1] = dnumber;
			vl_id[port_id_2] = (*nf);
				
			rnumber++;
			pnumber++;
			
			l++;
		}
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
    
    pnumber = nfnumber_old;
    
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
			//connect socket
			s = cmd_connect();
		
			root["method"] = "transact";

			params.push_back("Open_vSwitch");
		
			pi = (*nf);
			
			strcpy(vrt, "vport");
			strcpy(trv, "vport");
			char ifac[64] = "iface";
				
			sprintf(temp, "%d", pnumber);
			strcat(vrt, temp);
				
			pnumber++;
				
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			//store this vport
			vport_l[pi].push_back(rp[l]);
				
			peer_name = peer_n[rp[l]].c_str();
		
			cmd_add_virtual_link(rp[l], peer_name, ifac, pi, s);
				
			pnumber++;
			rnumber++;
			
			root.clear();
			params.clear();
			
			//Increment transaction id
			id++;
			
			l++;
			
			virtual_links.push_back(make_pair(nf_size+ports.size() + l, port_uuid[pi].size()));
			
			//disconnect socket
			cmd_disconnect(s);
		}
	}
    	
    rnumber_new = 1;
     
    clo = new CreateLsiOut(dnumber_new, physical_ports, network_functions_ports, virtual_links);
    	
	return clo;
}

void commands::add_ports(int rnumber, string p, uint64_t dnumber, int nf, int s){
	
	char temp[64] = "", tmp[64] = "";
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	locale loc;	
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	//connect socket
    s = cmd_connect();

	char ifac[64] = "iface";
		
	//Create the current name of a interface
	if(nf == 0){
		sprintf(temp, "%d", rnumber);
		strcat(ifac, temp);
		
		strcpy(temp, p.c_str());
	}else{
		string str = p;
		
		sprintf(temp, "%d", rnumber);
		strcat(ifac, temp);
				
		//Create port name to lower case
		for (string::size_type i=0; i<str.length(); ++i)
    		str[i] = tolower(str[i], loc);
    				
    	//Create the current port name
		strcpy(tmp, (char *)(str).c_str());
		sprintf(temp, "%" PRIu64, dnumber);
		strcat(tmp, "b");
		strcat(tmp, temp);
		strcpy(temp, tmp);
				
		for(unsigned int j=0;j<strlen(temp);j++){
			if(temp[j] == '_')
				temp[j] = 'p';
		}
	}
	
	root["method"] = "transact";

	params.push_back("Open_vSwitch");
		
	first_obj["op"] = "insert";
	first_obj["table"] = "Interface";
		
	/*Insert an Interface*/
	row["name"] = temp;
	if(nf != 0)
		row["type"] = "internal";
		
	row["admin_state"] = "up";
	row["link_state"] = "up";
		
	first_obj["row"] = row;
		
	first_obj["uuid-name"] = ifac;
		
	params.push_back(first_obj);
		
	row.clear();
	first_obj.clear();
			
	first_obj["op"] = "insert";
	first_obj["table"] = "Port";
		
	/*Insert a port*/
	row["name"] = temp;
		
	iface.push_back("set");
	
	iface2.push_back("named-uuid");
	iface2.push_back(ifac);
	
	iface1.push_back(iface2);
	iface.push_back(iface1);
		
	row["interfaces"] = iface;
		
	first_obj["row"] = row;
		
	first_obj["uuid-name"] = temp;
		
	params.push_back(first_obj);
		
	//insert this port name into port_l
	port_l[dnumber].push_back(temp);
		
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
		
	port2.push_back("named-uuid");
	port2.push_back(temp);
	
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

	root["id"] = id;

	stringstream ss;
 	write_formatted(root, ss );
		
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}
    
    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
		
	Value value;
    read( read_buf, value );
    Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{	
		//Search the first object related by updated Bridge
		
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
					 	//save the second element, the new port create
							 	
						if(i==1){
							//insert in a port_uuid the list of port-uuid for this switch
							port_uuid[dnumber].push_back(stuff1[1].getString());
						}
					} else if(name1 == "error"){
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Syntax error in Json request.");
						throw commandsException();
					}
				}		
			}
		}
	}
			
	root.clear();
	params.clear();
			
	//Increment transaction id
	id++;

	//disconnect socket
    cmd_disconnect(s);
}

void commands::cmd_editconfig_lsi_delete(uint64_t dpi, int s){
	
    ssize_t nwritten = 0;
	
	int r = 0;
	
	char read_buf[4096] = "";
	
	locale loc;	
	
	map<string, unsigned int> ports;
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	/*for all virtual link ports, destroy it*/
	for(list<uint64_t>::iterator i = virtual_link_id[switch_id[dpi]].begin(); i != virtual_link_id[switch_id[dpi]].end(); i++){
		cmd_delete_virtual_link(vl_id[(*i)], (*i), s);
	}

	port_l.erase(dpi);
	vport_l.erase(dpi);
	virtual_link_id[switch_id[dpi]].clear();

	//connect socket
    s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");
				
	first_obj["op"] = "update";
	first_obj["table"] = "Open_vSwitch";
			   
	first_obj["where"] = where;
	
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

	root["id"] = id;

	stringstream ss;
 	write_formatted(root, ss );	
	
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}
    
    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
			
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);

	root.clear();
	params.clear();
			
	//Increment transaction id
	id++;
	
	//disconnect socket
    cmd_disconnect(s);
}

AddNFportsOut *commands::cmd_editconfig_NFPorts(AddNFportsIn anpi, int s){

	AddNFportsOut *anf = NULL;
	
	char temp[64] = "", tmp[64] = "";
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	locale loc;	
	
	map<string, unsigned int> ports;

	list<string> nfp = anpi.getNetworkFunctionsPorts();
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	//connect socket
    s = cmd_connect();

	if(nfp.size() != 0){
		
		root["method"] = "transact";

		params.push_back("Open_vSwitch");
		
		/*for each port in the list of nfp*/
		for(list<string>::iterator nf = nfp.begin(); nf != nfp.end(); nf++)
		{
			char ifac[64] = "iface";
				
			//Create the current name of a interface
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
			
			string str = (*nf);
				
			//Create port name to lower case
			for (string::size_type i=0; i<str.length(); ++i)
    			str[i] = tolower(str[i], loc);
    				
    		//Create the current port name
			strcpy(tmp, (char *)(str).c_str());
			sprintf(temp, "%" PRIu64, anpi.getDpid());
			strcat(tmp, "b");
			strcat(tmp, temp);
			strcpy(temp, tmp);
				
			for(unsigned int j=0;j<strlen(temp);j++){
				if(temp[j] == '_')
					temp[j] = 'p';
			}
				
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			first_obj["op"] = "insert";
			first_obj["table"] = "Interface";
		
			row["name"] = temp;
			row["type"] = "internal";
			
			row["admin_state"] = "up";
			row["link_state"] = "up";
		
			first_obj["row"] = row;
		
			first_obj["uuid-name"] = ifac;
		
			params.push_back(first_obj);
		
			row.clear();
			first_obj.clear();
				
			first_obj["op"] = "insert";
			first_obj["table"] = "Port";
		
			row["name"] = temp;
		
			iface.push_back("set");
	
			iface2.push_back("named-uuid");
			iface2.push_back(ifac);
	
			iface1.push_back(iface2);
			iface.push_back(iface1);
		
			row["interfaces"] = iface;
		
			first_obj["row"] = row;
		
			first_obj["uuid-name"] = temp;
		
			params.push_back(first_obj);
		
			row.clear();
			first_obj.clear();
			iface.clear();
			iface1.clear();
			iface2.clear();
				
			//insert this port name into port_n
			port_l[anpi.getDpid()].push_back(temp);
				
			ports[(*nf)] = rnumber;	
				
			rnumber++;
		}
		
		first_obj["op"] = "update";
		first_obj["table"] = "Bridge";
			   
		third_object.push_back("_uuid");
		third_object.push_back("==");
		
		fourth_object.push_back("uuid");
		fourth_object.push_back(switch_uuid[anpi.getDpid()].c_str());
		
		third_object.push_back(fourth_object);
		where.push_back(third_object);
		
		first_obj["where"] = where;
	
		where.clear();
	
		for(list<string>::iterator u = port_uuid[anpi.getDpid()].begin(); u != port_uuid[anpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");
				
			port2.push_back((*u));
	
			port1.push_back(port2);
	
			port2.clear();
		}
		
		for(list<string>::iterator u = vport_uuid[anpi.getDpid()].begin(); u != vport_uuid[anpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");
			
			port2.push_back((*u));
	
			port1.push_back(port2);
	
			port2.clear();
		}
	
		for(list<string>::iterator nff = nfp.begin(); nff != nfp.end(); nff++)
		{
			string str = (*nff);
				
			//Create port name to lower case
			for (string::size_type i=0; i<str.length(); ++i)
    			str[i] = tolower(str[i], loc);
    				
			strcpy(tmp, (char *)(str).c_str());
			sprintf(temp, "%" PRIu64, anpi.getDpid());
			strcat(tmp, "b");
			strcat(tmp, temp);
			strcpy(temp, tmp);
				
			for(unsigned int j=0;j<strlen(temp);j++){
				if(temp[j] == '_')
					temp[j] = 'p';
			}
		
			port2.push_back("named-uuid");
			port2.push_back(temp);
	
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

		root["id"] = id;

		stringstream ss;
 		write_formatted(root, ss );
		
		nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
		if (nwritten == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			throw commandsException();
		}
    
    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (r == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			throw commandsException();
		}
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
		
		Value value;
    	read( read_buf, value );
    	Object rootNode = value.getObject();

		for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
		{
			//Search the first object related by updated Bridge
			std::string name = (*it).first;
			const Value &node = it->second;
			if (name == "result")
			{
				const Array &result = node.getArray();
				
				for(unsigned i=0;i<result.size();i++)
				{
					Object uuidNode = result[i].getObject();
								 		
					for (Object::const_iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
					{
						std::string name1 = (*it1).first;
						const Value &node1 = it1->second;
							
						if(name1 == "uuid")
						{
							const Array &stuff1 = node1.getArray();
									
							//save the second element, the new port created
							if(i==1){
								//insert in a port_uuid the list of port-uuid for this switch
								port_uuid[dnumber].push_back(stuff1[i].getString());
							}
						} else if(name1 == "error"){
							logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Syntax error in Json request.");
							throw commandsException();
						}
					}		
				}
			}
		}
			
		root.clear();
		params.clear();
			
		//Increment transaction id
		id++;
	}

	//disconnect socket
    cmd_disconnect(s);

    anf = new AddNFportsOut(anpi.getNFname(), ports);

	return anf;
}

void commands::cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi, int s){
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	locale loc;	
	
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
		
		list<string>::iterator uu = port_uuid[dnpi.getDpid()].begin();
		
		/*for each port in the list of the getNetworkFunctionsPorts*/
		for(set<string>::iterator p = nfp.begin(); p != nfp.end(); p++){
			string sss = (*p);
			uu = port_uuid[dnpi.getDpid()].begin();
			//should be search in port_l, p....if find it take the index and remove it from the set port-uuid[pi]
			for(list<string>::iterator u = port_l[dnpi.getDpid()].begin(); u != port_l[dnpi.getDpid()].end(); u++){
				string s = (*u);
				if(s.compare(sss) == 0){
					port_uuid[dnpi.getDpid()].remove((*uu));
					break;	
				}
				uu++;
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

		root["id"] = id;

		stringstream ss;
 		write_formatted(root, ss );
		
		nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
		if (nwritten == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			throw commandsException();
		}
    
    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (r == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			throw commandsException();
		}
			
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
			
		root.clear();
		params.clear();
			
		//Increment transaction id
		id++;
		
		cmd_disconnect(s);
	}
}

AddVirtualLinkOut *commands::cmd_addVirtualLink(AddVirtualLinkIn avli, int s){
	/*struct to return*/
	AddVirtualLinkOut *avlo = NULL;
	
	uint64_t port_id_1 = 0, port_id_2 = 0;
	char temp[64] = "", vrt[64] = "", trv[64] = "";
	
	list<pair<unsigned int, unsigned int> > virtual_links;
	
	strcpy(vrt, "vport");
	char ifac[64] = "iface";
	    	
	/*create virtual link*/
	sprintf(temp, "%d", pnumber);
	strcat(vrt, temp);
			
	port_id_1 = pnumber;
		
	pnumber++;
			
	port_id_2 = pnumber;
			
	/*create virtual link*/
	sprintf(temp, "%d", pnumber);
	strcat(trv, temp);
		
	//Create the current name of a interface
	sprintf(temp, "%d", rnumber);
	strcat(ifac, temp);

	rnumber++;

	//create first endpoint
	cmd_add_virtual_link(vrt, trv, ifac, avli.getDpidA(), s);
	
	//Create the current name of a interface
	strcpy(ifac, "iface");
	sprintf(temp, "%d", rnumber);
	strcat(ifac, temp);
	
	//create second endpoint
	cmd_add_virtual_link(trv, vrt, ifac, avli.getDpidB(), s);

	/*store the information [bridge name, list of peer port]*/
	virtual_link_id[switch_id[avli.getDpidA()]].push_back(port_id_2);
	
	/*store the information [bridge name, list of peer port]*/
	virtual_link_id[switch_id[avli.getDpidB()]].push_back(port_id_1);

	port_id[port_id_1] = vrt;
	port_id[port_id_2] = trv;

	/*store the information [switch_id, port_id]*/
	vl_id[port_id_1] = avli.getDpidA();
	vl_id[port_id_2] = avli.getDpidB();
	
	//insert this port name into port_n
	vport_l[avli.getDpidA()].push_back(vrt);
	//insert this port name into port_n
	vport_l[avli.getDpidB()].push_back(trv);
	
	/*peer name of the bridge name vrt is trv*/
	peer_n[trv] = vrt;
	
	/*peer name of the bridge name trv is vrt*/
	peer_n[vrt] = trv;
				
	/*store the information of virtual link*/
	virtual_links.push_back(make_pair(port_id_1, port_id_2));
	
	/*store the information of virtual link*/
	virtual_links.push_back(make_pair(port_id_2, port_id_1));

	avlo = new AddVirtualLinkOut(port_uuid[avli.getDpidA()].size() + vport_uuid[avli.getDpidA()].size(), port_uuid[avli.getDpidB()].size() + vport_uuid[avli.getDpidB()].size());

	return avlo;
}

void commands::cmd_add_virtual_link(string vrt, string trv, char ifac[64], uint64_t dpi, int s){
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	locale loc;	
	
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

	root["id"] = id;

	stringstream ss;
 	write_formatted(root, ss );
	
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}
    
    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);	
		
	Value value;
    read( read_buf, value );
    Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		//Search the first object related by updated Bridge
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
									
						//save the second element, the new port created
						if(i==1){
							//insert in a port_uuid the list of port-uuid for this switch
							port_uuid[dpi].push_back(stuff1[i].getString());
						}
					} else if(name1 == "error"){
						logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Syntax error in Json request.");
						throw commandsException();
					}
				}		
			}
		}
	}
			
	root.clear();
	params.clear();
			
	//Increment transaction id
	id++;
	
	//disconnect socket
    cmd_disconnect(s);
}

void commands::cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli, int s){
	
	uint64_t port_id_1 = 0, port_id_2 = 0;
	char vrt[64] = "", trv[64] = "";
	
	list<pair<unsigned int, unsigned int> > virtual_links;

	//destroy first endpoint
	cmd_delete_virtual_link(dvli.getDpidA(), dvli.getIdA(), s);
	
	//destroy second endpoint
	cmd_delete_virtual_link(dvli.getDpidB(), dvli.getIdB(), s);

	/*store the information [switch_id, port_id]*/
	vl_id.erase(port_id_1);
	vl_id.erase(port_id_2);
	
	//remove this port name into port_n
	vport_l[dvli.getDpidA()].remove(vrt);
	//insert this port name into port_n
	vport_l[dvli.getDpidB()].remove(trv);
				
	/*store the information of virtual link*/
	virtual_links.remove(make_pair(dvli.getDpidA(), dvli.getDpidB()));
	
	/*store the information of virtual link*/
	virtual_links.remove(make_pair(dvli.getDpidB(), dvli.getDpidA()));
}

void commands::cmd_delete_virtual_link(uint64_t dpi, uint64_t idp, int s){

    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	locale loc;	
	
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

	root["id"] = id;

	stringstream ss;
 	write_formatted(root, ss );
	
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}
    
    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
			
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
			
	root.clear();
	params.clear();
			
	//Increment transaction id
	id++;
	
	//disconnect socket
    cmd_disconnect(s);
}
