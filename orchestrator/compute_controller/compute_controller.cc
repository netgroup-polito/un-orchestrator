#include "compute_controller.h"

static const char LOG_MODULE_NAME[] = "Compute-Controller";

pthread_mutex_t ComputeController::nfs_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

map<int,uint64_t> ComputeController::cores;
int ComputeController::nextCore = 0;

ComputeController::ComputeController()
{
}

ComputeController::~ComputeController()
{
}

void ComputeController::setCoreMask(uint64_t core_mask)
{
	uint64_t mask = 1;
	for(uint64_t i = 0; i < 64; i++)
	{
		if(core_mask & mask)
		{
			cores[nextCore] = mask;
			nextCore++;
		}

		mask = mask << 1;
	}
	nextCore = 0;

	for(unsigned int i = 0; i < cores.size(); i++)
		ULOG_DBG_INFO("Mask of an available core: \"%d\"",cores[i]);
}

nf_manager_ret_t ComputeController::retrieveDescription(string nf_id, string nf_name, string name_resolver_ip, int name_resolver_port)
{
	try
 	{
 		string translation;

 		ULOG_DBG_INFO("Considering the NF with id \"%s\"",nf_id.c_str());

		char ErrBuf[BUFFER_SIZE];
		struct addrinfo Hints;
		struct addrinfo *AddrInfo;
		int socket;							// keeps the socket ID for this connection
		int WrittenBytes;					// Number of bytes written on the socket
		int ReadBytes;						// Number of bytes received from the socket
		char DataBuffer[DATA_BUFFER_SIZE];	// Buffer containing data received from the socket

		memset(&Hints, 0, sizeof(struct addrinfo));

		Hints.ai_family= AF_INET;
		Hints.ai_socktype= SOCK_STREAM;

		ostringstream oss;
		oss << name_resolver_port;

		if (sock_initaddress (name_resolver_ip.c_str(), oss.str().c_str(), &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
		{
			ULOG_ERR("Error resolving given address/port (%s/%d): %s",  name_resolver_ip.c_str(), name_resolver_port, ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		stringstream tmp;
		tmp << "GET " << NAME_RESOLVER_BASE_URL << nf_name << " HTTP/1.1\r\n";
		tmp << "Host: :" << name_resolver_ip << ":" << name_resolver_port << "\r\n";
		tmp << "Connection: close\r\n";
		tmp << "Accept: */*\r\n\r\n";
		string message = tmp.str();

		char command[message.size()+1];
		command[message.size()]=0;
		memcpy(command,message.c_str(),message.size());

		if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			// AddrInfo is no longer required
			ULOG_ERR("Cannot contact the name resolver at \"%s:%d\"", name_resolver_ip.c_str(), name_resolver_port);
			ULOG_ERR("%s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		WrittenBytes = sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
		if (WrittenBytes == sockFAILURE)
		{
			ULOG_ERR("Error sending data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;

		}

		ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			ULOG_ERR("Error reading data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		// Terminate buffer, just for printing purposes
		// Warning: this can originate a buffer overflow
		DataBuffer[ReadBytes]= 0;

		ULOG_DBG_INFO("Data received: ");
		ULOG_DBG_INFO("%s",DataBuffer);

		shutdown(socket,SHUT_WR);
		sock_close(socket,ErrBuf,sizeof(ErrBuf));

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_METHOD_NOT_ALLLOWED,3) == 0)
			return NFManager_NO_NF;

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_OK,3) != 0)
			return NFManager_SERVER_ERROR;

		//the HTTP headers must be removed
		int i = 0;
		for(; i < ReadBytes; i++)
		{
			if((i+4) <= ReadBytes)
			{
				if((DataBuffer[i] == '\r') && (DataBuffer[i+1] == '\n') && (DataBuffer[i+2] == '\r') && (DataBuffer[i+3] == '\n'))
				{
					i += 4;
					break;
				}
			}
		}

		translation.assign(&DataBuffer[i]);

		if(!parseAnswer(translation,nf_name,nf_id))
		{
			//ERROR IN THE SERVER
			return NFManager_SERVER_ERROR;
		}
 	}
	catch (std::exception& e)
	{
		ULOG_ERR("Exception: %s",e.what());
		return NFManager_SERVER_ERROR;
	}

	return NFManager_OK;
}

bool ComputeController::parseAnswer(string answer, string nf_name, string nf_id)
{
	try
	{
		Value value;
		read(answer, value);
		Object obj = value.getObject();

		bool foundName = false;
		bool foundImplementations = false;

		list<Description*> possibleDescriptions;
		string nf_received_name;

		bool foundNports = false;
		unsigned int numports = 0;
		bool foundTextDescription = false;

		//A first sacan of the json is done in order to read the number of ports of the VNF.
		for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
		{
	 	    const string& name  = i->first;
		    const Value&  value = i->second;
			if(name == "nports")
		    {
				foundNports = true;
		    	numports = value.getInt();
		    	break;
		    }
		}
		if(!foundNports)
		{
			ULOG_WARN("Key \"num-ports\" not found in the answer");
			return false;
		}

		//Now let's do a second scan in order to retrieve all the other information received from
		//the name-resolver
		for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
		{
	 	    const string& name  = i->first;
		    const Value&  value = i->second;

		    if(name == "name")
		    {
		    	foundName = true;
		    	nf_received_name = value.getString();
		    	if(nf_received_name != nf_name)
		    	{
			    	ULOG_WARN("Required NF \"%s\", received info for NF \"%s\"",nf_name.c_str(),nf_received_name.c_str());
					return false;
		    	}
		    }
		    else if(name == "nports")
		    {
		    	//Information already retrieved
		    	continue;
		    }
		    else if(name == "summary")
		    {
				foundTextDescription = true;
				//XXX: the content of this field is ignored
		    }
		    else if(name == "implementations")
		    {
		    	foundImplementations = true;
		    	const Array& impl_array = value.getArray();
		    	if(impl_array.size() == 0)
		    	{
			    	ULOG_WARN("Key \"implementations\" without descriptions");
					return false;
		    	}

				bool foundPorts = false; //Mandatory only in case of KVM
				bool next = false;
		    	//Iterate on the implementations
		    	for( unsigned int impl = 0; impl < impl_array.size(); ++impl)
				{
					//This is an implementation, with a type and an URI
					Object implementation = impl_array[impl].getObject();
					bool foundURI = false;
					bool foundType = false;
					bool foundCores = false;
					bool foundLocation = false;
					bool foundDependencies = false;

					string type;
			    	string uri;
					string cores;
					string location;
					string dependencies;
					std::map<unsigned int, PortType> port_types; // port_id -> port_type
					std::map<unsigned int, string> port_mac; // port_id -> port_mac
					std::map<unsigned int, string> port_ip; // port_id -> port_ip

					for( Object::const_iterator impl_el = implementation.begin(); impl_el != implementation.end(); ++impl_el )
					{
				 	    const string& el_name  = impl_el->first;
						const Value&  el_value = impl_el->second;

						if(el_name == "type")
						{
							foundType = true;
							type = el_value.getString();
							if(!NFType::isValid(type))
							{
								ULOG_DBG_INFO("Invalid implementation type \"%s\". Skip it.", type.c_str());
								//return false;
								next = true;
								break;
							}
						}
						else if(el_name == "uri")
						{
							foundURI = true;
							uri = el_value.getString();
						}
						else if(el_name == "cores")
						{
							foundCores = true;
							cores = el_value.getString();
						}
						else if(el_name == "location")
						{
							foundLocation = true;
							location = el_value.getString();
						}
						else if(el_name == "ports")
						{
							foundPorts = true;

					    	const Array& ports_array = el_value.getArray();

					    	if (ports_array.size() == 0)
					    	{
						    	ULOG_WARN("Empty ports list in implementation");
								return false;
					    	}
					    	for( unsigned int p = 0; p < ports_array.size(); ++p)
							{
								Object port = ports_array[p].getObject();
								int port_id = -1;
								PortType port_type = UNDEFINED_PORT;

								for( Object::const_iterator port_el = port.begin(); port_el != port.end(); ++port_el )
								{
							 	    const string& pel_name  = port_el->first;
									const Value&  pel_value = port_el->second;
									if (pel_name == "id") {
										port_id = pel_value.getInt();
									}
									else if (pel_name == "type") {
										port_type = portTypeFromString(pel_value.getString());
										if (port_type == INVALID_PORT) {
											ULOG_WARN("Invalid port type \"%s\" for implementation port", pel_value.getString().c_str());
											return false;
										}
									}
									else {
										ULOG_WARN("Invalid key \"%s\" within an implementation port", pel_name.c_str());
										return false;
									}
								}
								if (port_id == -1) {
									ULOG_WARN("Missing port \"ïd\" attribute for implementation");
									return false;
								}
								if (port_type == UNDEFINED_PORT) {
									ULOG_WARN("Missing port \"type\" attribute for implementation");
									return false;
								}
								ULOG_DBG_INFO(" Port %d id=%d type=%s", p, port_id, portTypeToString(port_type).c_str());

								port_types.insert(std::map<unsigned int, PortType>::value_type(port_id, port_type));
							}
						}
						else if(el_name == "dependencies")
						{
							foundDependencies = true;
							dependencies = el_value.getString();
						}
						else
						{
							ULOG_WARN("Invalid key \"%s\" within an implementation", el_name.c_str());
							return false;
						}
					}

					if(next)
					{
						//The current network function is of a type not supported by the orchestrator
						next = false;
						continue;
					}

					if(!foundURI || !foundType)
					{
						ULOG_WARN("Key \"uri\", key \"type\", or both are not found into an implementation description");
						return false;
					}


					//The port types are not specified in the message coming from the name-resolver.
					//We set the port type as follows:
					//	* VETH_PORT in case of Docker container or Native functions
					//	* DPDKR_PORT in case of DPDK process


					if(type == "dpdk")
					{
#ifdef ENABLE_DPDK_PROCESSES
						if(!foundCores || !foundLocation)
						{
							ULOG_WARN("Description of a NF of type \"%s\" received without the \"cores\" attribute, \"location\" attribute, or both",type.c_str());
							return false;
						}

						assert(!foundPorts);

						for(unsigned int i = 0; i < numports; i++)
							port_types.insert(std::map<unsigned int, PortType>::value_type(i+1, DPDKR_PORT));

						possibleDescriptions.push_back(dynamic_cast<Description*>(new DPDKDescription(type,uri,cores,location,port_types)));
#endif
						continue;
					}
					else if(type == "native")
					{
#ifdef ENABLE_NATIVE
						assert(!foundPorts);
						if(!foundLocation || !foundDependencies)
						{
							ULOG_WARN("Description of a NF of type \"%s\" received without the \"dependencies\" attribute, \"location\" attribute, or both",type.c_str());
							return false;
						}
						std::stringstream stream(dependencies);
						std::string s;
						std::list<std::string>* dep_list = new std::list<std::string>;
						while(stream >> s){
							dep_list->push_back(s);
						}

						for(unsigned int i = 0; i < numports; i++)
							port_types.insert(std::map<unsigned int, PortType>::value_type(i+1, VETH_PORT));

						possibleDescriptions.push_back(dynamic_cast<Description*>(new NativeDescription(type,uri,location,dep_list,port_types)));
#endif
						continue;
					}
					else if(foundCores || foundLocation || foundDependencies)
					{
						ULOG_WARN("Description of a NF of type \"%s\" received with a wrong attribute (\"cores\", \"location\" or \"dependencies\")",type.c_str());
						return false;
					}

					if(!foundPorts)
					{
						if(type == "docker")
						{
							for(unsigned int i = 0; i < numports; i++)
								port_types.insert(std::map<unsigned int, PortType>::value_type(i+1, VETH_PORT));
						}
						else
						{
							assert(type == "kvm");
							//In case of KVM, the port type must be specified by the name-resolver.
							assert(0 && "Probably there is a BUG in the name resover!");
							ULOG_WARN("Description of a NF of type \"%s\" received without the element \"ports\"",type.c_str());
							return false;
						}
					}

					Description* descr = new Description(type, uri, port_types);
					possibleDescriptions.push_back(descr);
				}
		    } //end if(name == "implementations")
		    else
			{
				ULOG_WARN("Invalid key \"%s\"",name.c_str());
				return false;
			}
		}//end iteration on the answer

		if(!foundName || !foundImplementations || !foundTextDescription		)
		{
			ULOG_WARN("Key \"name\", and/or key \"implementations\", and/or key \"description\" not found in the answer");
			return false;
		}

		NF *new_nf = new NF(nf_name);
		assert(possibleDescriptions.size() != 0);

		if(possibleDescriptions.size() == 0)
		{
			ULOG_WARN("Cannot find a supported implementation for the network function \"%s\"",nf_name.c_str());
			return false;
		}

		for(list<Description*>::iterator impl = possibleDescriptions.begin(); impl != possibleDescriptions.end(); impl++)
			new_nf->addDescription(*impl);

		nfs[nf_id] = new_nf;

	}
	catch (std::runtime_error& e) {
		ULOG_WARN("JSON parse error: %s", e.what());
		return false;
	}
	catch(...)
	{
		ULOG_WARN("The content does not respect the JSON syntax");
		return false;
	}

	return true;
}

void ComputeController::checkSupportedDescriptions() {

	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++){

		NF *current = nf->second;

		list<Description*> descriptions = current->getAvailableDescriptions();

		ULOG_DBG_INFO("%d descriptions available for NF \"%s\".", descriptions.size(), (current->getName()).c_str());

		list<Description*>::iterator descr;
		for(descr = descriptions.begin(); descr != descriptions.end(); descr++){

			switch((*descr)->getType()){

#ifdef ENABLE_DOCKER
					//Manage Docker execution environment
				case DOCKER:{
					NFsManager *dockerManager = new Docker();
					if(dockerManager->isSupported(**descr)){
						(*descr)->setSupported(true);
						ULOG_DBG_INFO("Docker description of NF \"%s\" is supported.",(current->getName()).c_str());
					} else {
						ULOG_DBG_INFO("Docker description of NF \"%s\" is not supported.",(current->getName()).c_str());
					}
					delete dockerManager;
				}
				break;
#endif

#ifdef ENABLE_DPDK_PROCESSES
					//Manage DPDK execution environment
				case DPDK:{
					NFsManager *dpdkManager = new Dpdk();
					if(dpdkManager->isSupported(**descr)){
						(*descr)->setSupported(true);
						ULOG_DBG_INFO("DPDK description of NF \"%s\" is supported.",(current->getName()).c_str());
					} else {
						ULOG_DBG_INFO("DPDK description of NF \"%s\" is not supported.",(current->getName()).c_str());
					}
					delete dpdkManager;
				}
				break;
#endif

#ifdef ENABLE_KVM
					//Manage QEMU/KVM execution environment through libvirt
				case KVM:{
					NFsManager *libvirtManager = new Libvirt();
					if(libvirtManager->isSupported(**descr)){
						(*descr)->setSupported(true);
						ULOG_DBG_INFO("KVM description of NF \"%s\" is supported.",(current->getName()).c_str());
					} else {
						ULOG_DBG_INFO("KVM description of NF \"%s\" is not supported.",(current->getName()).c_str());
					}
					delete libvirtManager;
				}
				break;
#endif

#ifdef ENABLE_NATIVE
					//Manage NATIVE execution environment
				case NATIVE:
				{
					NFsManager *nativeManager = NULL;
					try{
						nativeManager = new Native();
						if(nativeManager->isSupported(**descr)){
							(*descr)->setSupported(true);
							ULOG_DBG_INFO("Native description of NF \"%s\" is supported.",(current->getName()).c_str());
						} else {
							ULOG_DBG_INFO("Native description of NF \"%s\" is not supported.",(current->getName()).c_str());
						}
						delete nativeManager;
					} catch (exception& e) {
						ULOG_DBG_INFO("exception %s has been thrown", e.what());
						delete nativeManager;
					}
				}
					break;
#endif
					//[+] Add here other implementations for the execution environment

				default:
					ULOG_DBG_INFO("No available execution environments for description type %s", NFType::toString((*descr)->getType()).c_str());
			}

		}

	}

}

NFsManager* ComputeController::selectNFImplementation(list<Description*> descriptions) {

	list<Description*>::iterator descr;

	/*
	 * TODO: descriptions.sort({COMPARATOR})
	 */

	bool selected = false;

	for(descr = descriptions.begin(); descr != descriptions.end() && !selected; descr++) {

		if((*descr)->isSupported()){

			switch((*descr)->getType()){

#ifdef ENABLE_DOCKER
				//Manage Docker execution environment
			case DOCKER:{

				NFsManager *dockerManager = new Docker();
				dockerManager->setDescription(*descr);

				selected = true;
				ULOG_DBG_INFO("Docker description has been selected.");

				return dockerManager;

			}
			break;
#endif

#ifdef ENABLE_DPDK_PROCESSES
				//Manage DPDK execution environment
			case DPDK:{

				NFsManager *dpdkManager = new Dpdk();
				dpdkManager->setDescription(*descr);

				selected = true;
				ULOG_DBG_INFO("DPDK description has been selected.");

				return dpdkManager;

			}
			break;
#endif

#ifdef ENABLE_KVM
				//Manage QEMU/KVM execution environment through libvirt
			case KVM:{

				NFsManager *libvirtManager = new Libvirt();
				libvirtManager->setDescription(*descr);

				selected = true;
				ULOG_DBG_INFO("KVM description has been selected.");

				return libvirtManager;

			}
			break;
#endif

#ifdef ENABLE_NATIVE
				//Manage NATIVE execution environment
			case NATIVE:
			{
				NFsManager *nativeManager = NULL;
				try{

					nativeManager = new Native();
					nativeManager->setDescription(*descr);

					selected = true;
					ULOG_DBG_INFO("Native description has been selected.");

					return nativeManager;

				} catch (exception& e) {
					ULOG_DBG_INFO("exception %s has been thrown", e.what());
					delete nativeManager;
				}
			}
				break;
#endif
				//[+] Add here other implementations for the execution environment

			default:
				assert(0);
			}
		}
	}
	return NULL;
}


bool ComputeController::selectImplementation()
{
	/**
	 * set boolean `supported` in each supported network function
	 */
	checkSupportedDescriptions();

	/**
	 * Select an implementation of the NF
	 */
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++){

		NF *current = nf->second;

		//A description is selected only for those functions that do not have a description yet
		if(current->getSelectedDescription() == NULL){

			list<Description*> descriptions = current->getAvailableDescriptions();

			NFsManager *selectedImplementation = selectNFImplementation(descriptions);

			if(selectedImplementation == NULL) {

				ULOG_DBG_INFO("No available description for NF with id\'%s\'", nf->first.c_str());
				return false;

			}

			current->setSelectedDescription(selectedImplementation);
			ULOG_DBG_INFO("Implementation has been selected for NF with id \"%s\".",nf->first.c_str());

		}
	}

	if(allSelected()){
		return true;
	}

	ULOG_ERR("Some network functions do not have a supported description!");

	return false;

}

bool ComputeController::allSelected()
{
	bool retVal = true;

	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		NF *current = nf->second;
		if(current->getSelectedDescription() == NULL)
		{
			ULOG_DBG_INFO("The NF with id \"%s\" has not been selected yet.",nf->first.c_str());
			retVal = false;
		}
	}
	return retVal;
}

nf_t ComputeController::getNFType(string id)
{
	assert(nfs.count(id) != 0);

	NF *nf = nfs[id];
	NFsManager *impl = nf->getSelectedDescription();

	assert(impl != NULL);

	return impl->getNFType();
}

const Description* ComputeController::getNFSelectedImplementation(string id)
{
	map<string, NF*>::iterator nf_it = nfs.find(id);
	if (nf_it == nfs.end()) { // Not found
		return NULL;
	}

	NFsManager *impl = (nf_it->second)->getSelectedDescription();
	if (impl == NULL)
		return NULL;

	return impl->getDescription();
}

void ComputeController::setLsiID(uint64_t lsiID)
{
	this->lsiID = lsiID;
}

bool ComputeController::updateNF(string nf_id, map<unsigned int, string> namesOfPortsOnTheSwitch, map<unsigned int, port_network_config_t > portsConfiguration, list<unsigned int> newPortsToAdd)
{
	ULOG_INFO("Updating the NF with id \"%s\"", nf_id.c_str());

	ULOG_INFO("Number of ports for updating %d", newPortsToAdd.size());

	NF *nf = nfs[nf_id];
	NFsManager *nfsManager = nf->getSelectedDescription();
	UpdateNFIn uni(lsiID, nf_id, namesOfPortsOnTheSwitch, portsConfiguration, newPortsToAdd);

	if(!nfsManager->updateNF(uni))
	{
		ULOG_ERR("An error occurred while updating the NF with id \"%s\"",nf_id.c_str());
		return false;
	}

	return true;
}

bool ComputeController::startNF(string nf_id, map<unsigned int, string> namesOfPortsOnTheSwitch, map<unsigned int, port_network_config_t > portsConfiguration
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	, list<port_mapping_t > controlConfiguration, list<string> environmentVariables
#endif
	)
{
	ULOG_INFO("Starting the NF with id \"%s\"", nf_id.c_str());
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	if(!controlConfiguration.empty())
	{
		ULOG_DBG_INFO("\tControl (%d):",controlConfiguration.size());
		for(list<port_mapping_t >::iterator n = controlConfiguration.begin(); n != controlConfiguration.end(); n++)
		{
			if(!(n->host_port).empty())
				ULOG_DBG_INFO("\t\tHost tcp port -> %s",(n->host_port).c_str());
			if(!(n->guest_port).empty())
				ULOG_DBG_INFO("\t\tVnf tcp port -> %s",(n->guest_port).c_str());
		}
	}

	if(!environmentVariables.empty())
	{
		ULOG_DBG_INFO("\tEnvironment variables (%d):",environmentVariables.size());
		for(list<string>::iterator ev = environmentVariables.begin(); ev != environmentVariables.end(); ev++)
			ULOG_DBG_INFO("\t\t%s",ev->c_str());
	}
#endif
	ULOG_DBG_INFO("Ports of the NF connected to the switch:");
	//map<unsigned int, port_network_config_t >::iterator it1 = portsConfiguration.begin();
	for(map<unsigned int, string>::iterator it = namesOfPortsOnTheSwitch.begin(); it != namesOfPortsOnTheSwitch.end(); it++) {
		ULOG_DBG_INFO("\t\t%d : %s", it->first, it->second.c_str());

		port_network_config_t configuration = portsConfiguration[it->first];

		if(!configuration.mac_address.empty())
			ULOG_DBG_INFO("\t\t\tMAC address : %s", configuration.mac_address.c_str());
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		if(!configuration.ip_address.empty())
			ULOG_DBG_INFO("\t\t\tIP address : %s", configuration.ip_address.c_str());
#endif
		//it1++;
	}

	if(nfs.count(nf_id) == 0)
	{
		assert(0);
		ULOG_WARN("Unknown NF with id \"%s\"",nf_id.c_str());
		return false;
	}

	NF *nf = nfs[nf_id];
	NFsManager *nfsManager = nf->getSelectedDescription();

	StartNFIn sni(lsiID, nf_id, namesOfPortsOnTheSwitch, portsConfiguration,
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		controlConfiguration, environmentVariables,
#endif
		calculateCoreMask(nfsManager->getCores()));

	if(!nfsManager->startNF(sni))
	{
		ULOG_ERR("An error occurred while starting the NF with id \"%s\"",nf_id.c_str());
		return false;
	}

	nf->setRunning(true);

	return true;
}

void ComputeController::stopAll()
{
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		stopNF(nf->first);
}

bool ComputeController::stopNF(string nf_id)
{
	ULOG_DBG_INFO("Stopping the NF with id \"%s\"",nf_id.c_str());

	if(nfs.count(nf_id) == 0)
	{
		ULOG_WARN("Unknown NF with id \"%s\"",nf_id.c_str());
		return false;
	}

	NF *nf = nfs[nf_id];
	NFsManager *nfsManager = nf->getSelectedDescription();

	StopNFIn sni(lsiID,nf_id);

	if(!nfsManager->stopNF(sni))
	{
		ULOG_ERR("An error occurred while stopping the NF with id \"%s\"",nf_id.c_str());
		return false;
	}

	map<string, NF*>::iterator it = nfs.begin();
	for(;it != nfs.end(); it++)
	{
		if(it->first == nf_id)
		{
			nfs.erase(it);
			break;
		}
	}
	assert(it != nfs.end());

	nf->setRunning(false);

	return true;
}

uint64_t ComputeController::calculateCoreMask(string coresRequried)
{
	if(coresRequried == "")
		return 0x0;

	int requiredCores;
	sscanf(coresRequried.c_str(),"%d",&requiredCores);

	pthread_mutex_lock(&nfs_manager_mutex);
	uint64_t mask = 0;
	for(int i = 0; i < requiredCores; i++)
	{
		mask |= cores[nextCore];
		nextCore = (nextCore+1)%cores.size();
	}
	pthread_mutex_unlock(&nfs_manager_mutex);

	ULOG_DBG_INFO("The NF requires %d cores. Its core mask is  \"%x\"",requiredCores,mask);

	return mask;
}

void ComputeController::printInfo(int graph_id)
{
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		nf_t type = nf->second->getSelectedDescription()->getNFType();
		string str = NFType::toString(type);
		if(graph_id == 2)
			coloredLogger(ANSI_COLOR_BLUE,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tID: '%s'\t-\tType: %s\t-\tStatus: %s",nf->second->getName().c_str(),(nf->first.length()<=7)? "\t" : "",nf->first.c_str(), str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else if(graph_id == 3)
			coloredLogger(ANSI_COLOR_RED,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tID: '%s'\t-\tType: %s\t-\tStatus: %s",nf->second->getName().c_str(),(nf->first.length()<=7)? "\t" : "",nf->first.c_str(), str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else
			ULOG_INFO("\t\tName: '%s'%s\t-\tID: '%s'\t-\tType: %s\t-\tStatus: %s",nf->second->getName().c_str(),(nf->first.length()<=7)? "\t" : "",nf->first.c_str(), str.c_str(),(nf->second->getRunning())?"running":"stopped");
	}
}
