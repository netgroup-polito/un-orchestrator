#include "compute_controller.h"
#include "template/template_parser.h"


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

string ComputeController::buildUrl(highlevel::VNFs vnfDescription) {
	stringstream tmp;
	if(vnfDescription.checkVnfTemplateField()) {
		tmp << "GET " << VNF_REPOSITORY_TEMPLATE_URL << vnfDescription.getVnfTemplate() << "/ HTTP/1.1\r\n";
	}
	else
		tmp << "GET " << VNF_REPOSITORY_TEMPLATES_URL << vnfDescription.getName() << "/ HTTP/1.1\r\n";
	tmp << "Host: " << Configuration::instance()->getVnfRepoIp() << ":" << Configuration::instance()->getVnfRepoPort() << "\r\n";
	tmp << "Connection: close\r\n";
	tmp << "Accept: */*\r\n\r\n";
	return tmp.str();
}

list<string> ComputeController::retrieveFileList(string tenant_id, string graph_id, string vnf_id)
{
    string ip = Configuration::instance()->getConfigServiceIp().c_str();
    int port = Configuration::instance()->getConfigServicePort();
    string endpoint = "http://" + ip + ":" + to_string(port);

    string url = endpoint + CONFIG_SERVICE_FILELIST_URL+tenant_id+"/"+graph_id+"/"+vnf_id+"";
    http_response response;
    try{
        ULOG_DBG_INFO("retrieveFileList, perform get to url:  %s", url.c_str());
        http_client client(U(url));
        response = client.request(methods::GET).get();
    }catch(const std::exception &e)
    {
        throw;
    }
    list<string> fileList;
    if(response.status_code() == status_codes::OK){
        const json::value& json_resp = response.extract_json().get();
        for(auto iter = json_resp.as_array().begin(); iter != json_resp.as_array().end(); ++iter){
            string filename = (*iter).as_string();
            fileList.push_back(filename);
        }
    }
    return fileList;
}

string ComputeController::retrieveFile(string tenant_id, string graph_id, string vnf_id, string filename, string dst_path){

    string ip = Configuration::instance()->getConfigServiceIp().c_str();
    int port = Configuration::instance()->getConfigServicePort();
    string endpoint = "http://" + ip + ":" + to_string(port);

    string path_file = dst_path+"/"+filename;
    string url = endpoint + CONFIG_SERVICE_FILE_URL+tenant_id+"/"+graph_id+"/"+vnf_id+"/"+filename+"";

    auto fileStream = std::make_shared<concurrency::streams::ostream>();

    // Open stream to output file.
    pplx::task<void> requestTask = concurrency::streams::fstream::open_ostream(path_file).then([=](concurrency::streams::ostream outFile)
    {
        *fileStream = outFile;

        // Create http_client to send the request.
        http_client client(url);
        ULOG_DBG_INFO("retrieveFile, perform get to url:  %s", url.c_str());
        return client.request(methods::GET);
    })
    // Handle response headers arriving.
    .then([=](http_response response)
    {
        if(response.status_code() == status_codes::OK){
            // Write response body into the file.
            return response.body().read_to_end(fileStream->streambuf());
        }
        else
            throw runtime_error("Received response status code: " + to_string(response.status_code()));
    })
    // Close the file stream.
    .then([=](size_t)
    {
        return fileStream->close();

    });

    // Wait for all the outstanding I/O to complete and handle any exceptions
    try
    {
        requestTask.wait();
        return path_file;
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

nf_manager_ret_t ComputeController::retrieveDescription(highlevel::VNFs vnfDescription){

        string resp;
		list<NFtemplate *> templates;
		nf_manager_ret_t retVal;
		ULOG_DBG_INFO("Considering the NF with ID \"%s\"",vnfDescription.getId().c_str());

		string ip = Configuration::instance()->getVnfRepoIp().c_str();
        int port = Configuration::instance()->getVnfRepoPort();
        string endpoint = "http://" + ip + ":" + to_string(port);

        string url = endpoint;
        if(vnfDescription.checkVnfTemplateField())
            url += VNF_REPOSITORY_TEMPLATE_URL + vnfDescription.getVnfTemplate()+"/";
	    else
		    url += VNF_REPOSITORY_TEMPLATES_URL + vnfDescription.getName()+"/";

        http_response response;
        try{
            ULOG_DBG_INFO("retrieveDescription, perform get to url:  %s", url.c_str());
            http_client client(U(url));
            response = client.request(methods::GET).get();
        }catch(const std::exception &e)
        {
            return NFManager_SERVER_ERROR;
        }

        resp = response.extract_string().get();
        //ULOG_DBG_INFO("resp: %s",resp.c_str());
        retVal = Template_Parser::parse(templates,resp,vnfDescription.checkVnfTemplateField() /*add eventual additional checks with ||*/);
        if(retVal != NFManager_OK)
            return retVal;

        if(!addImplementations(templates,vnfDescription.getId(),vnfDescription.getPortsId().size()))
            return NFManager_NO_NF;

        return NFManager_OK;

}


nf_manager_ret_t ComputeController::retrieveDescriptionOLD(highlevel::VNFs vnfDescription)
{

		string translation;
		list<NFtemplate *> templates;
		nf_manager_ret_t retVal;
		ULOG_DBG_INFO("Considering the NF with ID \"%s\"",vnfDescription.getId().c_str());

		char ErrBuf[BUFFER_SIZE];
		struct addrinfo Hints;
		struct addrinfo *AddrInfo;
		int socket;							// keeps the socket ID for this connection
		int WrittenBytes;					// Number of bytes written on the socket
		int ReadBytes;						// Number of bytes received from the socket
		char DataBuffer[DATA_BUFFER_SIZE];	// Buffer containing data received from the socket
		string url;
		memset(&Hints, 0, sizeof(struct addrinfo));

		Hints.ai_family= AF_INET;
		Hints.ai_socktype= SOCK_STREAM;
		ostringstream oss;
		oss << Configuration::instance()->getVnfRepoPort();

		if (sock_initaddress (Configuration::instance()->getVnfRepoIp().c_str(), oss.str().c_str(), &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
		{
			ULOG_ERR("Error resolving given address/port (%s/%d): %s",  Configuration::instance()->getVnfRepoIp().c_str(), Configuration::instance()->getVnfRepoPort(), ErrBuf);
			return NFManager_SERVER_ERROR;
		}
		url = buildUrl(vnfDescription);
		char command[url.size()+1];
		command[url.size()]=0;
		memcpy(command,url.c_str(),url.size());

		if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			// AddrInfo is no longer required
			ULOG_ERR("Cannot contact the vnf repository at \"%s:%d\"", Configuration::instance()->getVnfRepoIp().c_str(), Configuration::instance()->getVnfRepoPort());
			ULOG_ERR("%s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		WrittenBytes = sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
		if (WrittenBytes == sockFAILURE)
		{
			ULOG_ERR("Error sending data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;

		}
		sleep(1);
		ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			ULOG_ERR("Error reading data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		// Terminate buffer, just for printing purposes
		// Warning: this can originate a buffer overflow
		DataBuffer[ReadBytes]= 0;

		ULOG_DBG_INFO("Data received %d: " , ReadBytes);
		ULOG_DBG_INFO("%s",DataBuffer);
		shutdown(socket,SHUT_WR);
		sock_close(socket,ErrBuf,sizeof(ErrBuf));

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_METHOD_NOT_ALLLOWED,3) == 0 || strncmp(&DataBuffer[CODE_POSITION],CODE_HTTP_NOT_FOUND,3) == 0)
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

		retVal = Template_Parser::parse(templates,translation,vnfDescription.checkVnfTemplateField() /*add eventual additional checks with ||*/);
		if(retVal != NFManager_OK)
			return retVal;

		if(!addImplementations(templates,vnfDescription.getId(),vnfDescription.getPortsId().size())){
			return NFManager_NO_NF;
		}


	return NFManager_OK;
}



void ComputeController::checkSupportedDescriptions() {

	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++){

		NF *current = nf->second;

		list<Description*> descriptions = current->getAvailableDescriptions();

		ULOG_DBG_INFO("%d descriptions available for NF \"%s\".", descriptions.size(), (current->getName()).c_str());

		list<Description*>::iterator descr;
		for(descr = descriptions.begin(); descr != descriptions.end(); descr++){

			switch((*descr)->getTemplate()->getVnfType()){

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
						if(nativeManager->isSupported(**descr)){ //segmentation fault
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
					ULOG_DBG_INFO("No available execution environments for description type %s", vnfTypeToString((*descr)->getTemplate()->getVnfType()).c_str());
			}

		}

	}

}

bool ComputeController::addImplementations(list<NFtemplate *>& templates, string nf_id, int number_of_ports){
	map<unsigned int, PortTechnology> port_technologies; // port_id -> port_technology
	list<Description*> possibleDescriptions;
	string capability =templates.front()->getCapability(); //it s the same for all
	int original_number_of_ports = number_of_ports;  //save the original value , so i can decrease it below to check the remaining number of ports to instantiate
	for(list<NFtemplate*>::iterator temp = templates.begin(); temp != templates.end(); temp++){
		if ((*temp)->getVnfType() == DPDK) {
#ifdef ENABLE_DPDK_PROCESSES
			for(list<Port>::iterator port = (*temp)->getPorts().begin(); port != (*temp)->getPorts().end(); port++) {
				int begin, end;
				port->splitPortsRangeInInt(begin, end);
				if(end != -1){
					for(int i = begin;i<=end;i++){
						//In case of DPDK process, the port type is fixed
						port_technologies.insert(map<unsigned int, PortTechnology>::value_type(i, DPDKR_PORT));
					}
					number_of_ports -= (end-begin)+1;
				}
				else
					for(int i = begin; i <= number_of_ports; i++){  //case UNBOUNDED
						//In case of DPDK process, the port type is fixed
						port_technologies.insert(map<unsigned int, PortTechnology>::value_type(i, DPDKR_PORT));
					}
			}
			possibleDescriptions.push_back(new Description(*temp,port_technologies));
#endif
		} else if ((*temp)->getVnfType() == NATIVE) {
#ifdef ENABLE_NATIVE
			for(list<Port>::iterator port = (*temp)->getPorts().begin(); port != (*temp)->getPorts().end(); port++) {
					int begin, end;
					port->splitPortsRangeInInt(begin, end);
					if(end != -1){
						for(int i = begin;i<=end;i++){
							//In case of native function, the port type is fixed
							port_technologies.insert(map<unsigned int, PortTechnology>::value_type(i, VETH_PORT));
						}
						number_of_ports -= (end-begin)+1;
					}
					else //case UNBOUNDED
						for(int i = begin; i <= number_of_ports; i++){
							//In case of native function, the port type is fixed
							port_technologies.insert(map<unsigned int, PortTechnology>::value_type(i, VETH_PORT));
						}
			}
			possibleDescriptions.push_back(new Description(*temp,port_technologies));
#endif
		}

		if ((*temp)->getVnfType() == DOCKER) {
#ifdef ENABLE_DOCKER
				for(list<Port>::iterator port = (*temp)->getPorts().begin(); port != (*temp)->getPorts().end(); port++) {
					int begin, end;
					port->splitPortsRangeInInt(begin, end);
					if(end != -1){
						for (int i = begin; i <= end; i++) {
							//In case of docker, the port type is fixed
							port_technologies.insert(map<unsigned int, PortTechnology> ::value_type(i, VETH_PORT));
						}
						number_of_ports -= (end-begin)+1;
					}
					else {//case UNBOUNDED
						for (int i = begin; i <= number_of_ports ; i++) {
							//In case of docker, the port type is fixed
							port_technologies.insert(map<unsigned int, PortTechnology> ::value_type(i, VETH_PORT));
						}
					}


				}
				Description *descr = new Description(*temp, port_technologies);
				possibleDescriptions.push_back(descr);
#endif
		}
		if ((*temp)->getVnfType() == KVM) {
#ifdef ENABLE_KVM
				for(list<Port>::iterator port = (*temp)->getPorts().begin(); port != (*temp)->getPorts().end(); port++) {
					int begin, end;
					port->splitPortsRangeInInt(begin, end);
					if(end != -1){
						for (int i = begin; i <= end; i++) {
							port_technologies.insert(map <unsigned int, PortTechnology> ::value_type(i, port->getTechnology()));
						}
						number_of_ports -= (end-begin) +1;
					}
					else
						for (int i = begin; i <= number_of_ports ; i++) {
							port_technologies.insert(map <unsigned int, PortTechnology> ::value_type(i, port->getTechnology()));
						}
					}
				Description *descr = new Description(*temp, port_technologies);
				possibleDescriptions.push_back(descr);
#endif
		}
		//[+] insert other implementations

	if (original_number_of_ports > (int)port_technologies.size())
	{
	//TODO: when we select the implementation, we should take into account the number of ports supported by the VNF!
	ULOG_WARN("Number of ports from (%d) graph is greater then the number of ports from NF description (%d) for VNF with id \"%s\"",number_of_ports,port_technologies.size(), nf_id.c_str());
	return false;
	}

}


	assert(possibleDescriptions.size() != 0);

	if(possibleDescriptions.size() == 0)
	{
		ULOG_WARN("Cannot find a supported implementation for the network function \"%s\"",capability.c_str());
		return false;
	}

	NF *new_nf = new NF(capability);
	for(list<Description*>::iterator impl = possibleDescriptions.begin(); impl != possibleDescriptions.end(); impl++)
		new_nf->addDescription(*impl);

	nfs[nf_id] = new_nf;
	return true;

}


bool ComputeController::downloadImage(Description * description,string vnfImagePath) {

	ULOG_DBG_INFO("Downloading image for NF at path '%s'",vnfImagePath.c_str());
	
	stringstream command;
	stringstream pathImage;
	unsigned char hash_token[HASH_SIZE];
	char hash_uri [BUFFER_SIZE] ;
	char tmp[HASH_SIZE] ;
	strcpy(tmp, "");
	strcpy(hash_uri, "");
	SHA256((const unsigned char *) description->getTemplate()->getURI().c_str(), strlen(description->getTemplate()->getURI().c_str()), hash_token);

	for (int i = 0; i < HASH_SIZE; i++) {
		sprintf(tmp, "%.2x", hash_token[i]);
		strcat(hash_uri, tmp);
	}
	command << getenv("un_script_path") << PULL_NF << " " << description->getTemplate()->getCapability() << " " << description->getTemplate()->getURI() << " "
			<< hash_uri << " " << vnfImagePath;
	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());
	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;
	if (retVal == 0)
		return false;
	pathImage << vnfImagePath << "/" << description->getTemplate()->getCapability() << "_" << hash_uri ;
	description->getTemplate()->setURI(pathImage.str());
	ULOG_DBG_INFO("NF image properly downloaded");

	return true;
}

NFsManager* ComputeController::selectNFImplementation(list<Description*> descriptions) {

	list<Description*>::iterator descr;

	/*
	 * TODO: descriptions.sort({COMPARATOR})
	 */

	bool selected = false;
	bool downloadSuccess;
	for(descr = descriptions.begin(); descr != descriptions.end() && !selected; descr++) {

		if((*descr)->isSupported()){

			switch((*descr)->getTemplate()->getVnfType()){

#ifdef ENABLE_DOCKER
				//Manage Docker execution environment
			case DOCKER:{

				NFsManager *dockerManager = new Docker();
				dockerManager->setDescription(*descr);
				selected = true;
				ULOG_DBG_INFO("Docker description has been selected.");
				if((*descr)->getTemplate()->getURIType() == REMOTE_FILE){  //other checks are in the script called by the plugin
					downloadSuccess=downloadImage(*descr,Configuration::instance()->getVnfImagesPath());
					assert(downloadSuccess);
					if(!downloadSuccess){
						ULOG_ERR("Download failed!");
						return NULL;
					}
				}
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
				if((*descr)->getTemplate()->getURIType() == REMOTE_FILE){
					downloadSuccess = downloadImage(*descr,Configuration::instance()->getVnfImagesPath());
					assert(downloadSuccess);
					if(!downloadSuccess){
						ULOG_ERR("Download failed!");
						return NULL;
					}
				}

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
				if((*descr)->getTemplate()->getURIType() == REMOTE_FILE){
					downloadSuccess = downloadImage(*descr,Configuration::instance()->getVnfImagesPath());
					assert(downloadSuccess);
					if(!downloadSuccess){
						ULOG_ERR("Download failed!");
						return NULL;
					}
				}
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
					if((*descr)->getTemplate()->getURIType() == REMOTE_FILE){
						downloadSuccess = downloadImage(*descr,Configuration::instance()->getVnfImagesPath());
						assert(downloadSuccess);
						if(!downloadSuccess){
							ULOG_ERR("Download failed!");
							return NULL;
						}
					}
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

Description* ComputeController::getNFSelectedImplementation(string id)
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
    ,string dir_to_mount, string dst_path
	)
{
	ULOG_INFO("Starting the NF with id \"%s\"", nf_id.c_str());

	if(dir_to_mount=="" && dst_path==""){
	    dir_to_mount = "None";
	    dst_path = "None";
	}
	else{
	    ULOG_DBG_INFO("\tThere is a datadisk to mount");
	    ULOG_DBG_INFO("\tHost dir to mount: %s", dir_to_mount.c_str());
	    ULOG_DBG_INFO("\tDestination path: %s", dst_path.c_str());
	}

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

	StartNFIn sni(lsiID, nf_id, namesOfPortsOnTheSwitch, portsConfiguration, dir_to_mount, dst_path,
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
		string str = vnfTypeToString(type);
		if(graph_id == 2)
			coloredLogger(ANSI_COLOR_BLUE,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tID: '%s'\t-\tType: %s\t-\tStatus: %s",nf->second->getName().c_str(),(nf->first.length()<=7)? "\t" : "",nf->first.c_str(), str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else if(graph_id == 3)
			coloredLogger(ANSI_COLOR_RED,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tID: '%s'\t-\tType: %s\t-\tStatus: %s",nf->second->getName().c_str(),(nf->first.length()<=7)? "\t" : "",nf->first.c_str(), str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else
			ULOG_INFO("\t\tName: '%s'%s\t-\tID: '%s'\t-\tType: %s\t-\tStatus: %s",nf->second->getName().c_str(),(nf->first.length()<=7)? "\t" : "",nf->first.c_str(), str.c_str(),(nf->second->getRunning())?"running":"stopped");
	}
}
