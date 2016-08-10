#include "xdpd_manager.h"

#include <algorithm>

static const char LOG_MODULE_NAME[] = "xDPd-Manager";

// Functor to compare port names of nf_port_info structs
struct comparePortInfoName
{
	comparePortInfoName(const std::string& name) : name(name) {}
	bool operator()(const struct nf_port_info& pi) { return pi.port_name == name; }

private:
	std::string name;
};

XDPDManager::XDPDManager()
	: xDPDport(XDPD_PORT)
{
	char ErrBuf[BUFFER_SIZE];
	struct addrinfo Hints;

	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AF_INET;
	Hints.ai_socktype= SOCK_STREAM;

	if (sock_initaddress (XDPD_ADDRESS, xDPDport.c_str(), &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		ULOG_ERR("Error resolving given address/port (%s/%s): %s",  XDPD_ADDRESS, xDPDport.c_str(), ErrBuf);
		throw XDPDManagerException();
	}
}

XDPDManager::~XDPDManager()
{
	sock_freeaddrinfo(AddrInfo);
}

string XDPDManager::sendMessage(string message)
{
	char ErrBuf[BUFFER_SIZE];
	int socket;						// keeps the socket ID for this connection
	int WrittenBytes;				// Number of bytes written on the socket
	int ReadBytes;					// Number of bytes received from the socket
	char DataBuffer[DATA_BUFFER_SIZE];			// Buffer containing data received from the socket

	char *command=new char[message.size()+1];
	command[message.size()]=0;
	memcpy(command,message.c_str(),message.size());

	if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		// AddrInfo is no longer required
		ULOG_ERR("Cannot contact xDPd: %s", ErrBuf);
		throw XDPDManagerException();
	}

	WrittenBytes= sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
	if (WrittenBytes == sockFAILURE)
	{
		ULOG_ERR("Error sending data: %s", ErrBuf);
		throw XDPDManagerException();

	}

	ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (ReadBytes == sockFAILURE)
	{
		ULOG_ERR("Error reading data: %s", ErrBuf);
		throw XDPDManagerException();
	}


	// Terminate buffer, just for printing purposes
	// Warning: this can originate a buffer overflow
	DataBuffer[ReadBytes]= 0;

	ULOG_DBG_INFO("Data received: ");
	ULOG_DBG_INFO("%s",DataBuffer);

	shutdown(socket,SHUT_WR);
	sock_close(socket,ErrBuf,sizeof(ErrBuf));

	return string(DataBuffer);
}

void XDPDManager::checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi)
{
	//Prepare the request
	Object json;
	json["command"] = DISCOVER_PHY_PORTS;
 	stringstream ss;
 	write_formatted(json, ss );

	string answer;
	try
	{
		answer = sendMessage(ss.str());
	}catch (...)
	{
		new XDPDManagerException();
	}

	//Parse the answer
	bool foundPorts = false;
	Value value;
	read( answer, value );
	Object obj = value.getObject();

	if(!findCommand(obj,string(DISCOVER_PHY_PORTS)))
		throw XDPDManagerException();
	if(!findStatus(obj))
		throw XDPDManagerException();

	for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;
		if( (name == "command") ||  (name == "status"))
		{
			continue;
		}
		else if(name == "ports")
		{
			const Array& ports_array = value.getArray();
			if(ports_array.size() == 0)
				ULOG_WARN("Answer to command \"%s\" with an empty port list.",DISCOVER_PHY_PORTS);

			for( unsigned int i = 0; i < ports_array.size(); ++i )
			{
				Object port = ports_array[i].getObject();
				bool foundName = false;
				string port_name;
				for( Object::const_iterator p = port.begin(); p != port.end(); ++p )
				{
					const string& p_name  = p->first;
					const Value&  p_value = p->second;
					if(p_name == "name")
					{
						foundName = true;
						port_name = p_value.getString();
					}
					else
					{
						ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",DISCOVER_PHY_PORTS,p_name.c_str());
						assert(0);
						throw XDPDManagerException();
					}
				}
				if(!foundName)
				{
					ULOG_WARN("Answer to command \"%s\" with a \"port\" without \"name\"",DISCOVER_PHY_PORTS);
					assert(0);
					throw XDPDManagerException();
				}
				ethernetInterfaces.insert(port_name);
			}

			foundPorts = true;
		}
		else
		{
			//error
			ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",DISCOVER_PHY_PORTS,name.c_str());
			throw XDPDManagerException();
		}
	}

	if(!foundPorts)
	{
		ULOG_WARN("Answer to command \"%s\" without \"ports\" received",DISCOVER_PHY_PORTS);
		throw XDPDManagerException();
	}

	set<CheckPhysicalPortsIn>::iterator pit = cppi.begin();
	for(; pit != cppi.end(); pit++)
	{
		if(pit->getPortType() == ETHERNET_PORT)
		{
			if(ethernetInterfaces.count(pit->getPortName()) == 0)
			{
				ULOG_WARN("Ethernet interface \"%s\" not supported by xDPd",(pit->getPortName()).c_str());
				throw XDPDManagerException();
			}
		}
		else
			//TODO: check if the wifi port is supported?
			wirelessInterfaces.insert(pit->getPortName());
	}

	//TODO: remove form ethernetInterfaces those not required?
}

CreateLsiOut *XDPDManager::createLsi(CreateLsiIn cli)
{
	Value value;

	string answer;
	//This is a mapping between port name in the graph and the port id in the graph
	map<string,unsigned int> port_name_id_in_graph;
	try
	{
		answer = sendMessage(prepareCreateLSIrequest(cli,port_name_id_in_graph));
	} catch(...) {
		throw;
	}

	read( answer, value );
	Object obj = value.getObject();

	if(!findCommand(obj,string(CREATE_LSI)) || !findStatus(obj))
		throw XDPDManagerException();

	CreateLsiOut *clo = 0;
	try
	{
		clo = parseCreateLSIresponse(cli, obj,port_name_id_in_graph);
	} catch(...) {
		throw;
	}

	return clo;
}

string XDPDManager::prepareCreateLSIrequest(CreateLsiIn cli, map<string,unsigned int> &port_name_id_in_graph)
{
	Object json;
	json["command"] = CREATE_LSI;

 	Object controller;
   	controller["address"] = cli.getControllerAddress();
	controller["port"] = cli.getControllerPort();

	json["controller"] = controller;

	list<string> ports = cli.getPhysicalPortsName();

	//FIXME: currently a single wifi port is supported. The message toward xDPd should be extended
	//to support an unbounded number of wifi ports.

	Array ports_array;
	bool foundWireless = false;
	for(list<string>::iterator p = ports.begin(); p != ports.end(); p++)
	{
		if(ethernetInterfaces.count(*p) != 0)
			ports_array.push_back(*p);
		else if(wirelessInterfaces.count(*p) != 0)
		{
			if(foundWireless)
			{
				ULOG_WARN("Only a single wireless interface is currently supported in xDPd",(*p).c_str());
				throw XDPDManagerException();
			}
			json["wireless"] = *p;
			foundWireless = true;
		}
		else
		{
			ULOG_WARN("Interface \"%s\" was not discovered by xDPd",(*p).c_str());
			throw XDPDManagerException();
		}
	}
	if(ports.size() > 0)
		json["ports"] = ports_array;

	set<string> nfs = cli.getNetworkFunctionsName();
	map<string,nf_t> nf_type = cli.getNetworkFunctionsType();

	Array nfs_array;
	for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		Object network_function;
		network_function["name"] = *nf;

		assert(nf_type.count(*nf) != 0);
		nf_t nft = nf_type[*nf];

		Array nf_ports_array;
		list<struct nf_port_info> nf_ports = cli.getNetworkFunctionsPortsInfo(*nf);

		PortType port_type = UNDEFINED_PORT;
		for(list<struct nf_port_info>::iterator nfp = nf_ports.begin(); nfp != nf_ports.end(); nfp++)
		{
			nf_ports_array.push_back(nfp->port_name);
			port_type = nfp->port_type;

			port_name_id_in_graph[nfp->port_name] = nfp->port_id;

			switch (port_type) {
				case VETH_PORT:
					assert(nft == DOCKER || nft == NATIVE);
					if(nft == NATIVE){
						//XXX xDPd does not expect the port type, but the VNF type
						nft = DOCKER;
					}
					continue;
				case DPDKR_PORT:
					assert(nft == DPDK);
					continue;
				case VHOST_PORT:
					//XXX xDPd does not expect the port type, but the VNF type
					nft = DOCKER;
					ULOG_DBG_INFO("Network function \"%s\" (that is a VM) is handled in xDPd as a \"%s\"",(*nf).c_str(),(NFType::toString(nft)).c_str());
					continue;
				case IVSHMEM_PORT:
					//XXX xDPd does not expect the port type, but the VNF type
					nft = DPDK;
					ULOG_DBG_INFO("Network function \"%s\" (that is a VM) is handled in xDPd as a \"%s\"",(*nf).c_str(),(NFType::toString(nft)).c_str());
					continue;
				case USVHOST_PORT:
					ULOG_WARN("Port type not supported.");
					assert(0 && "Port type not supported");
					throw XDPDManagerException();
					break;
				default:
					assert(0 && "You cannot be here!");
					throw XDPDManagerException();
			}

		}

		network_function["ports"] = nf_ports_array;

		network_function["type"] = NFType::toString(nft);

		nfs_array.push_back(network_function);
	}
	if(nfs.size() > 0)
		json["network-functions"] = nfs_array;

	list<uint64_t> vlinks = cli.getVirtualLinksRemoteLSI();
	//XXX: it supports only the case in which all the vlinks are shered with the same LSI
	if(vlinks.size() > 0)
	{
		list<uint64_t>::iterator vl = vlinks.begin();
		uint64_t remoteDpid = *vl;
		for(; vl != vlinks.end(); vl++)
		{
			if(*vl != remoteDpid)
				throw XDPDManagerException();
		}

		Object vlink;
		vlink["number"] = vlinks.size();
		vlink["remote-lsi"] = remoteDpid;
		json["virtual-links"] = vlink;
	 }

 	stringstream ss;
 	write_formatted(json, ss );

 	ULOG_DBG_INFO("Command to be sent: ");
 	ULOG_DBG_INFO("%s",ss.str().c_str());

 	return ss.str();
}

CreateLsiOut *XDPDManager::parseCreateLSIresponse(CreateLsiIn cli, Object message, map<string,unsigned int> port_name_id_in_graph)
{
	uint64_t dpid = 0;
	map<string,unsigned int> physical_ports;
	pair<string,unsigned int> wireless_port;
	map<string,map<string, unsigned int> >  network_functions_ports;
	list<pair<unsigned int, unsigned int> > virtual_links;

	map<string,list<string> > out_nf_ports_name_on_switch;
	map<string, map<string, unsigned int> > out_nf_ports_name_and_id;
	map<string,unsigned int >  endpoints_ports;

	list<string> ports = cli.getPhysicalPortsName();
	bool hasWireless = false;
	string wirelessName;
	for(list<string>::iterator p = ports.begin(); p != ports.end(); p++)
	{
		if(wirelessInterfaces.count(*p) != 0)
		{
			hasWireless = true;
			wirelessName = *p;
			break;
		}
	}

	bool foundLSIid = false;
	bool foundWireless = false;

	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;
		if( (name == "command") ||  (name == "status"))
		{
			continue;
		}
		else if(name == "lsi-id")
		{
			foundLSIid = true;
			dpid = value.getInt(); //FIXME: it is not an int
		}
		else if(name == "ports")
		{
			const Array& ports_array = value.getArray();

			if(ports_array.size() != (cli.getPhysicalPortsName().size() - ((hasWireless)?1:0)))
			{
				ULOG_WARN("Answer to command \"%s\" contains a wrong number of physical ports",CREATE_LSI);
				throw XDPDManagerException();
			}

			for( unsigned int j = 0; j < ports_array.size(); ++j )
			{
				Object port = ports_array[j].getObject();
				unsigned int id = 0;
				string name;
				bool foundID = false;
				bool foundName = false;
				for( Object::const_iterator p = port.begin(); p != port.end(); ++p )
				{
					const string& p_name  = p->first;
					const Value&  p_value = p->second;

					if(p_name == "name")
					{
						name = p_value.getString();
						foundName = true;
					}
					else if(p_name == "id")
					{
						id = p_value.getInt(); //FIXME: it isn't an int!
						foundID = true;
					}
				}
				if(foundName && foundID)
				{
					physical_ports[name] = id;
					list<string> ep = cli.getPhysicalPortsName();
					set<string> tmp_ep(ep.begin(),ep.end());
					if(tmp_ep.count(name) == 0)
					{
						ULOG_WARN("Answer to command \"%s\" contains a non-required port \"%d\"",CREATE_LSI,name.c_str());
						throw XDPDManagerException();
					}
				}
				else
				{
					ULOG_WARN("Answer to command \"%s\" contains a port without the name, the ID, or both",CREATE_LSI);
					throw XDPDManagerException();
				}
			} //end iteration on the array
		} //end name=="ports"
		else if(name == "wireless")
		{
			foundWireless = true;
			unsigned int wirelessID = value.getInt();

			physical_ports[wirelessName] = wirelessID;

			list<string> ep = cli.getPhysicalPortsName();
			set<string> tmp_ep(ep.begin(),ep.end());
			if(tmp_ep.count(wirelessName) == 0)
			{
				ULOG_WARN("Answer to command \"%s\" contains a non-required port \"%d\"",CREATE_LSI,wirelessName.c_str());
				throw XDPDManagerException();
			}
		} //end name=="wireless"
		else if(name == "network-functions")
		{
			const Array& nfs_array = value.getArray();

			if(nfs_array.size() != cli.getNetworkFunctionsName().size())
			{
				ULOG_WARN("Answer to command \"%s\" contains a wrong number of network functions (expected: %d - received: %d)",CREATE_LSI,cli.getNetworkFunctionsName().size(),nfs_array.size());
				throw XDPDManagerException();
			}

			for( unsigned int j = 0; j < nfs_array.size(); ++j )
			{
				//Each element of the array is a NF
				Object nf = nfs_array[j].getObject();

				string name;
				map<string,unsigned int> ports;
				list<string> port_name_on_switch;
				map<string, unsigned int> port_names_and_id;

				bool foundName = false;
				bool foundPorts = false;

				for( Object::const_iterator n = nf.begin(); n != nf.end(); ++n )
				{
					const string& n_name  = n->first;
					const Value&  n_value = n->second;

					if(n_name == "name")
					{
						name = n_value.getString();
						foundName = true;
					}
					else if(n_name == "ports")
					{
						const Array &ports_array = n_value.getArray();
						for( unsigned int p = 0; p < ports_array.size(); ++p )
						{
							Object port = ports_array[p].getObject();

							bool foundPortName = false;
							bool foundPortID = false;
							string port_name;
							unsigned int port_id_on_switch = 0;

							for( Object::const_iterator a_port = port.begin(); a_port != port.end(); ++a_port )
							{
								const string& ap_name  = a_port->first;
								const Value&  ap_value = a_port->second;

								if(ap_name == "name")
								{
									foundPortName = true;
									port_name = ap_value.getString();
								}
								else if(ap_name == "id")
								{
									port_id_on_switch = ap_value.getInt(); //FIXME: this is not an Int!
									foundPortID = true;
								}
							}
							if(!foundPortName || !foundPortID)
							{
								ULOG_WARN("Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_LSI);
								throw XDPDManagerException();
							}
							ports[port_name] = port_id_on_switch;

							//XXX The name of the port created on xDPd is in the form "lsiid_portname". Note that, if the plugin on xDPd is changed, this part of code must be
							//changed accordingly
							assert(dpid != 0);
							stringstream pnos;
							pnos << dpid << "_" << port_name;
							port_name_on_switch.push_back(pnos.str());

							unsigned int port_id_on_graph = port_name_id_in_graph[port_name];
							port_names_and_id[pnos.str()] = port_id_on_graph;
						}
						if(ports_array.size() > 0)
							foundPorts = true;
					} //end if(n_name == "ports")
				}
				if(foundName && foundPorts)
				{
					network_functions_ports[name] = ports;
					out_nf_ports_name_on_switch[name] = port_name_on_switch;
					out_nf_ports_name_and_id[name] = port_names_and_id;

					set<string> names = cli.getNetworkFunctionsName();
					if(names.count(name) == 0)
					{
						ULOG_WARN("Answer to command \"%s\" contains a non-required network function",CREATE_LSI,name.c_str());
						throw XDPDManagerException();
					}
				}
				else
				{
					ULOG_WARN("Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_LSI);
					throw XDPDManagerException();
				}
			} //end iteration on the array
		}//end name==network-functions
		else if(name == "virtual-links")
		{
			const Array& vls_array = value.getArray();

			if(vls_array.size() != cli.getVirtualLinksRemoteLSI().size())
			{
				ULOG_WARN("Answer to command \"%s\" contains virtual links",CREATE_LSI);
				throw XDPDManagerException();
			}

			//unsigned int currentTranslation = 0; //XXX: this may be useful to associate the endpoints of a virtual link with a specific position
			for( unsigned int j = 0; j < vls_array.size(); ++j )
			{
				Object vl = vls_array[j].getObject();
				unsigned int localID = 0;
				unsigned int remoteID = 0;
				bool foundLocalID = false;
				bool foundRemoteID = false;
				for( Object::const_iterator v = vl.begin(); v != vl.end(); ++v )
				{
					const string& v_name  = v->first;
					const Value&  v_value = v->second;

					if(v_name == "local-id")
					{
						localID = v_value.getInt();
						foundLocalID = true;
					}
					else if(v_name == "remote-id")
					{
						remoteID = v_value.getInt(); //FIXME: it isn't an int!
						foundRemoteID = true;
					}
				}
				if(foundLocalID && foundRemoteID)
				{
					virtual_links.push_back(make_pair(localID,remoteID));
				}
				else
				{
					ULOG_WARN("Answer to command \"%s\" contains a virtual link without the local ID, the remote ID, or both",CREATE_LSI);
					throw XDPDManagerException();
				}
			} //end iteration on the array
		}//end name=="virtual-links"
		else
		{
			//error
			ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",CREATE_LSI,name.c_str());
			throw XDPDManagerException();
		}
	} //end parsing the message

	if(!foundLSIid)
	{
		ULOG_WARN("Answer to command \"%s\" without \"lsi-id\" received",CREATE_LSI);
		throw XDPDManagerException();
	}

	if(hasWireless && !foundWireless)
	{
		ULOG_WARN("%d %d",hasWireless,foundWireless);

		ULOG_WARN("Answer to command \"%s\" without \"wireless\" received, although a wireless interface was required",CREATE_LSI);
		throw XDPDManagerException();
	}

	list<string> wirelessList;
	if(hasWireless)
	{
		//The virtual wireless interface created on xDPd must be connected to a physical wireless interface through a Linux bridge
		if(!attachWirelessPort(dpid, wirelessName))
		{
			ULOG_ERR("An error occurred while attaching the wireless interface \"%s\" to a Linux bridge",wirelessName.c_str());
			throw XDPDManagerException();
		}
		wirelessList.push_back(wirelessName);
	}

	dpdiWirelessInterfaces[dpid] = wirelessList;

	CreateLsiOut *clo = new CreateLsiOut(dpid,physical_ports,network_functions_ports, endpoints_ports, out_nf_ports_name_on_switch, virtual_links, out_nf_ports_name_and_id);
	return clo;
}
bool XDPDManager::findCommand(Object message, string expected)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;
		if( name == "command" )
		{
			if(value.getString() != expected)
			{
				ULOG_WARN("Expected command \"%s\" - received \"%s\"",expected.c_str(),value.getString().c_str());
				return false;
			}
			else
				return true;
 		}
 	}

 	ULOG_WARN("Command not found in the answer");
 	return false;
}

bool XDPDManager::findStatus(Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;
		if( name == "status" )
		{
			if(value.getString() != OK)
			{
				ULOG_WARN("Expected status \"%s\" - received \"%s\"",OK,value.getString().c_str());
				return false;
			}
			else
				return true;
 		}
 	}

 	ULOG_WARN("Status not found in the answer");
 	return false;
}

AddNFportsOut *XDPDManager::addNFPorts(AddNFportsIn anpi)
{
//	lsi.addNF(nf.first, nf.second);

	string answer = sendMessage(prepareCreateNFPortsRequest(anpi));
	map<string, unsigned int> port_name_id_in_graph;

	Value value;
	read( answer, value );
	Object obj = value.getObject();
	if(!findCommand(obj,string(CREATE_NF_PORTS)))
		throw XDPDManagerException();
	if(!findStatus(obj))
		throw XDPDManagerException();

	AddNFportsOut *anpo = 0;
	list<struct nf_port_info> port_list = anpi.getNetworkFunctionsPorts();
	for(list<struct nf_port_info>::iterator port = port_list.begin(); port != port_list.end(); port++)
	{
		port_name_id_in_graph[port->port_name] = port->port_id;
	}
	try
	{
		anpo = parseCreateNFPortsResponse(anpi,obj, port_name_id_in_graph);
	} catch(...) {
	//	lsi.removeNF(nf.first); lo dovro' fare fuori (posso evitarlo, se faccio la add dopo a sta chiamata)
		throw;
	}

	return anpo;
}

string XDPDManager::prepareCreateNFPortsRequest(AddNFportsIn anpi)
{
	Object json;
	json["command"] = CREATE_NF_PORTS;
 	json["lsi-id"] = anpi.getDpid();

	Array nfs_array;
	Object network_function;
	network_function["name"] = anpi.getNfId();

	nf_t type = anpi.getNFtype();

	Array nf_ports_array;
	list<struct nf_port_info> nf_ports = anpi.getNetworkFunctionsPorts();
	PortType port_type = UNDEFINED_PORT;
	for(list<struct nf_port_info>::iterator nfp = nf_ports.begin(); nfp != nf_ports.end(); nfp++)
	{
		nf_ports_array.push_back(nfp->port_name);
		port_type = nfp->port_type;

		switch (port_type) {
			case VETH_PORT:
				assert(type == DOCKER || type == NATIVE);
				if(type == NATIVE){
					//XXX xDPd does not expect the port type, but the VNF type
					type = DOCKER;
				}
				continue;
			case DPDKR_PORT:
				assert(type == DPDK);
				continue;
			case VHOST_PORT:
				//XXX xDPd does not exepct the port type, but the VNF type
				type = DOCKER;
				ULOG_DBG_INFO("Network function with id \"%s\" (that is a VM) is handled in xDPd as a \"%s\"",anpi.getNfId().c_str(),(NFType::toString(type)).c_str());
				continue;
			case IVSHMEM_PORT:
				//XXX xDPd does not exepct the port type, but the VNF type
				type = DPDK;
				ULOG_DBG_INFO("Network function with id \"%s\" (that is a VM) is handled in xDPd as a \"%s\"",anpi.getNfId().c_str(),(NFType::toString(type)).c_str());
				continue;
			case USVHOST_PORT:
				ULOG_WARN("Port type not supported.");
				assert(0 && "Port type not supported");
				throw XDPDManagerException();
				break;
			default:
				assert(0 && "You cannot be here!");
				throw XDPDManagerException();
		}
	}

	network_function["type"] = NFType::toString(type);

	network_function["ports"] = nf_ports_array;
	nfs_array.push_back(network_function);

	json["network-functions"] = nfs_array;

 	stringstream ss;
 	write_formatted(json, ss );

 	ULOG_DBG_INFO("Command to be sent: ");
 	ULOG_DBG_INFO("%s",ss.str().c_str());

 	return ss.str();
}

AddNFportsOut *XDPDManager::parseCreateNFPortsResponse(AddNFportsIn anpi, Object message, map<string, unsigned int> port_name_id_in_graph)
{
	bool foundNFs = false;
	map<string, unsigned int> ports;
	list<string> ports_name_on_switch;
	map<string, unsigned int> port_names_and_id;

	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;
		if( (name == "command") ||  (name == "status"))
		{
			continue;
		}
		else if(name == "network-functions")
		{
			foundNFs = true;

			const Array& nfs_array = value.getArray();

			if(nfs_array.size() != 1)
			{
				ULOG_WARN("Answer to command \"%s\" contains a wrong number of network functions ports",CREATE_LSI);
				throw XDPDManagerException();
			}

			for( unsigned int j = 0; j < nfs_array.size(); ++j )
			{
				//Each element of the array is a NF
				Object nf = nfs_array[j].getObject();

				string name;

				bool foundName = false;
				bool foundPorts = false;

				for( Object::const_iterator n = nf.begin(); n != nf.end(); ++n )
				{
					const string& n_name  = n->first;
					const Value&  n_value = n->second;

					if(n_name == "name")
					{
						name = n_value.getString();
						if(name != anpi.getNfId())
						{
							ULOG_WARN("Answer to command \"%s\" contains a non-required network function",CREATE_LSI,name.c_str());
							throw XDPDManagerException();
						}

						foundName = true;
					}
					else if(n_name == "ports")
					{
						const Array &ports_array = n_value.getArray();
						for( unsigned int p = 0; p < ports_array.size(); ++p )
						{
							Object port = ports_array[p].getObject();

							bool foundPortName = false;
							bool foundPortID = false;
							string port_name;
							unsigned int port_id = 0;

							for( Object::const_iterator a_port = port.begin(); a_port != port.end(); ++a_port )
							{
								const string& ap_name  = a_port->first;
								const Value&  ap_value = a_port->second;

								if(ap_name == "name")
								{
									foundPortName = true;
									port_name = ap_value.getString();
								}
								else if(ap_name == "id")
								{
									port_id = ap_value.getInt(); //FIXME: this is not an Int!
									foundPortID = true;
								}
							}
							if(!foundPortName || !foundPortID)
							{
								ULOG_WARN("Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_LSI);
								throw XDPDManagerException();
							}

							list<struct nf_port_info> ports_to_be_translated = anpi.getNetworkFunctionsPorts();
							if (count_if(ports_to_be_translated.begin(), ports_to_be_translated.end(), comparePortInfoName(port_name)) == 0)
							{
								ULOG_WARN("Answer to command \"%s\" contains a non-required network function port",CREATE_LSI,port_name.c_str());
								throw XDPDManagerException();
							}

							ports[port_name] = port_id;

							stringstream pnos;
							pnos << anpi.getDpid() << "_" << port_name;
							ports_name_on_switch.push_back(pnos.str());
							unsigned int port_id_on_graph = port_name_id_in_graph[port_name];
							port_names_and_id[pnos.str()] = port_id_on_graph;
						}
						if(ports_array.size() > 0)
							foundPorts = true;
					} //end if(n_name == "ports")
				}
				if(!foundName || !foundPorts)
				{
					ULOG_WARN("Answer to command \"%s\" contains a network function without the name, the ports, or both",CREATE_NF_PORTS);
					throw XDPDManagerException();
				}
			} //end iteration on the array
		}//end name==network-functions
		else
		{
			//error
			ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",CREATE_NF_PORTS,name.c_str());
			throw XDPDManagerException();
		}
	} //end parsing the message

	if(!foundNFs)
	{
		ULOG_WARN("Answer to command \"%s\" without \"network-functions\" received",CREATE_NF_PORTS);
		throw XDPDManagerException();
	}

	AddNFportsOut *anpo = new AddNFportsOut(anpi.getNfId(),ports,ports_name_on_switch, port_names_and_id);

	return anpo;
}

AddVirtualLinkOut *XDPDManager::addVirtualLink(AddVirtualLinkIn avli)
{
	string answer = sendMessage(prepareCreateVirtualLinkRequest(avli));

	Value value;
	read( answer, value );
	Object obj = value.getObject();
	if(!findCommand(obj,string(CREATE_VLINKS)))
		throw XDPDManagerException();
	if(!findStatus(obj))
		throw XDPDManagerException();

	//int vlink_position = lsi.addVlink(vlink);

	//logger(ORCH_DEBUG, XDPD_MODULE_NAME, __FILE__, __LINE__, "Virtual link with ID %d inserted in position %d",vlink.getID(),vlink_position);

	AddVirtualLinkOut *avlo = NULL;
	try
	{
		avlo = parseCreateVirtualLinkResponse(avli, obj);
	} catch(...) {
//		lsi.removeVlink(vlink.getID());
		assert(0);
		throw;
	}

	//return vlink.getID();
	return avlo;
}

string XDPDManager::prepareCreateVirtualLinkRequest(AddVirtualLinkIn avli)
{
	Object json;
	json["command"] = CREATE_VLINKS;
	json["number"] = 1;

	json["lsi-a"] = avli.getDpidA();
	json["lsi-b"] = avli.getDpidB();

 	stringstream ss;
 	write_formatted(json, ss );

 	ULOG_DBG_INFO("Command to be sent: ");
 	ULOG_DBG_INFO("%s",ss.str().c_str());

 	return ss.str();

}

AddVirtualLinkOut *XDPDManager::parseCreateVirtualLinkResponse(AddVirtualLinkIn avli, Object message)
{
	bool foundVlinks = false;
	uint64_t idA = 0, idB = 0;

	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;
		if( (name == "command") ||  (name == "status"))
		{
			continue;
		}
		else if(name == "virtual-links")
		{
			foundVlinks = true;
			const Array& vls_array = value.getArray();

			if(vls_array.size() != 1)//XXX: we create a vlink at a time, althugh xDPd allows the creation of many vlinks together
			{
				ULOG_WARN("Answer to command \"%s\" contains a wrong number of virtual links",CREATE_VLINKS);
				throw XDPDManagerException();
			}

			for( unsigned int j = 0; j < vls_array.size(); ++j )
			{
				Object vl = vls_array[j].getObject();
				bool foundA = false;
				bool foundB = false;
				for( Object::const_iterator v = vl.begin(); v != vl.end(); ++v )
				{
					const string& v_name  = v->first;
					const Value&  v_value = v->second;

					if(v_name == "id-a")
					{
						idA = v_value.getInt(); //FIXME: it isn't an int!
						foundA = true;
					}
					else if(v_name == "id-b")
					{
						idB = v_value.getInt(); //FIXME: it isn't an int!
						foundB = true;
					}
				}
				if(!foundA || !foundB)
				{
					ULOG_WARN("Answer to command \"%s\" contains a virtual link without the id-a, the id-b, or both",CREATE_VLINKS);
					throw XDPDManagerException();
				}
			} //end iteration on the array
		}//end name=="virtual-links"
		else
		{
			//error
			ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",CREATE_VLINKS,name.c_str());
			throw XDPDManagerException();
		}
	} //end parsing the message

	if(!foundVlinks)
	{
		ULOG_WARN("Answer to command \"%s\" without \"virtual-links\" received",CREATE_VLINKS);
		throw XDPDManagerException();
	}

	AddVirtualLinkOut *avlo = new AddVirtualLinkOut(idA,idB);
	return avlo;
}

void XDPDManager::destroyLsi(uint64_t dpid)
{
	string answer = sendMessage(prepareDestroyLSIrequest(dpid));

	Value value;
	read( answer, value );
	Object obj = value.getObject();
	if(!findCommand(obj,string(DESTROY_LSI)))
		throw XDPDManagerException();
	if(!findStatus(obj))
		throw XDPDManagerException();

	try
	{
		parseDestroyLSIresponse(obj);
		list<string> wirelessInterface = dpdiWirelessInterfaces[dpid];
		for(list<string>::iterator w = wirelessInterface.begin(); w != wirelessInterface.end(); w++)
			detachWirelessPort(dpid,*w);
	} catch(...) {
		throw;
	}
}

void XDPDManager::destroyVirtualLink(DestroyVirtualLinkIn dvli)
{
	string answer = sendMessage(prepareDestroyVirtualLinkRequest(dvli));

	Value value;
	read(answer, value);
	Object obj = value.getObject();
	if(!findCommand(obj,string(DESTROY_VLINKS)))
		throw XDPDManagerException();
	if(!findStatus(obj))
		throw XDPDManagerException();

	try
	{
		parseDestroyVirtualLinkResponse(obj);
	} catch(...) {
		throw;
	}
}

void XDPDManager::destroyNFPorts(DestroyNFportsIn dnpi)
{
	string answer = sendMessage(prepareDestroyNFPortsRequest(dnpi));

	Value value;
	read( answer, value );
	Object obj = value.getObject();
	if(!findCommand(obj,string(DESTROY_NF_PORTS)))
		throw XDPDManagerException();
	if(!findStatus(obj))
		throw XDPDManagerException();

	try
	{
		parseDestroyNFPortsResponse(obj);
	} catch(...) {
		throw;
	}
}

string XDPDManager::prepareDestroyLSIrequest(uint64_t dpid)
{
	Object json;
	json["command"] = DESTROY_LSI;

 	Object controller;
   	json["lsi-id"] = dpid;

 	stringstream ss;
 	write_formatted(json, ss );

 	ULOG_DBG_INFO("Command to be sent: ");
 	ULOG_DBG_INFO("%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseDestroyLSIresponse(Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		if( (name == "command") ||  (name == "status"))
		{
			continue;
		}
		else
		{
			//error
			ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",DESTROY_LSI,name.c_str());
			throw XDPDManagerException();
		}
	} //end parsing the message
}

string XDPDManager::prepareDestroyVirtualLinkRequest(DestroyVirtualLinkIn dvli)
{
	Object json;
	json["command"] = DESTROY_VLINKS;

	Object vlink;
	vlink["lsi-id"] = dvli.getDpidA();
	vlink["vlink-id"] = dvli.getIdA();

	Array vlinks_array;
	vlinks_array.push_back(vlink);

	json["virtual-links"] = vlinks_array;

 	stringstream ss;
 	write_formatted(json, ss );

 	ULOG_DBG_INFO("Command to be sent: ");
 	ULOG_DBG_INFO("%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseDestroyVirtualLinkResponse(Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		if( (name == "command") ||  (name == "status"))
		{
			continue;
		}
		else
		{
			//error
			ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",DESTROY_VLINKS,name.c_str());
			throw XDPDManagerException();
		}
	} //end parsing the message
}

string XDPDManager::prepareDestroyNFPortsRequest(DestroyNFportsIn dnpi)
{
	Object json;
	json["command"] = DESTROY_NF_PORTS;
	json["lsi-id"] = dnpi.getDpid();

	Array ports_array;

	set<string> nf_ports = dnpi.getNFports();

	set<string>::iterator p = nf_ports.begin();
	for(; p != nf_ports.end(); p++)
	{
		stringstream ss;
		ss << dnpi.getDpid() << "_" << *p;
		ports_array.push_back(ss.str());
	}

	if( ports_array.size() == 0)
	{
	   	//error
		ULOG_WARN("It seems that NF '%s' does not have any port!",dnpi.getNfId().c_str());
		assert(0);
		throw XDPDManagerException();
	}
	json["ports"] = ports_array;

 	stringstream ss;
 	write_formatted(json, ss );

 	ULOG_DBG_INFO("Command to be sent: ");
 	ULOG_DBG_INFO("%s",ss.str().c_str());

 	return ss.str();
}

void XDPDManager::parseDestroyNFPortsResponse(Object message)
{
	for( Object::const_iterator i = message.begin(); i != message.end(); ++i )
	{
		const string& name  = i->first;
		if( (name == "command") ||  (name == "status"))
		{
			continue;
		}
		else
		{
			//error
			ULOG_WARN("Answer to command \"%s\" with the unespected parameter \"%s\"",DESTROY_NF_PORTS,name.c_str());
			throw XDPDManagerException();
		}
	} //end parsing the message
}


bool XDPDManager::attachWirelessPort(uint64_t dpid, string wirelessInterfaceName)
{
	//The virtual wireless interface created by xDPd must be attached to the real wireles interface through a bridge

	ULOG_INFO("Attaching the wireless interface '%s'...",wirelessInterfaceName.c_str());

	stringstream command;
	command << getenv("un_script_path") << ATTACH_WIRELESS_INTERFACE << " " << dpid << " " << wirelessInterfaceName;
	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;
	else
		return true;
}

void XDPDManager::detachWirelessPort(uint64_t dpid, string wirelessInterfaceName)
{
	ULOG_INFO("Detaching the wireless interface '%s'...",wirelessInterfaceName.c_str());
	stringstream command;
	command << getenv("un_script_path") << DETACH_WIRELESS_INTERFACE << " " << dpid << " " << wirelessInterfaceName;
	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());
	int retVal = system(command.str().c_str());
	retVal += 1; //XXX: just to remove a warning

	//FIXME: no error can occur here?
}

