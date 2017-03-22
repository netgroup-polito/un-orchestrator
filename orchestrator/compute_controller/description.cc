#include "description.h"

bool operator==(const nf_port_info& lhs, const nf_port_info& rhs)
{
    return (lhs.port_name.compare(rhs.port_name) == 0) && (lhs.port_technology == rhs.port_technology);
}



Description::Description(NFtemplate * temp,std::map<unsigned int, PortTechnology>& port_technologies) :
	 temp(temp), port_technologies(port_technologies)
{
	supported = false;
	return;
}

Description::~Description(){
	delete temp;
}

NFtemplate* Description::getTemplate() {
	return temp;
}


bool Description::isSupported() {
	return supported;
}

void Description::setSupported(bool supported) {
	this->supported = supported;
}

PortTechnology Description::getPortTechnology(unsigned int port_id) const
{
	std::map<unsigned int, PortTechnology>::const_iterator it = port_technologies.find(port_id);
	if (it != port_technologies.end()) {
		return it->second;
	}
	return UNDEFINED_PORT;  // TODO: Should we make this INVALID_PORT to notify an error? Question is also: do we make the port specification in the NF description mandatory?
}

uri_t stringToUriType(const std::string& s){
	if (s.compare("remote-file") == 0)
		return REMOTE_FILE;
	else if (s.compare("local-file") == 0)
		return LOCAL_FILE;
	else if (s.compare("docker-registry") == 0)
		return DOCKER_REGISTRY;

	return UNDEFINED_URITYPE;
}

PortTechnology portTechnologyFromString(const std::string& s)
{
	if (s.compare("ivshmem") == 0)
		return IVSHMEM_PORT;
	else if (s.compare("usvhost") == 0)
		return USVHOST_PORT;
	else if (s.compare("vhost") == 0)
		return VHOST_PORT;

	return INVALID_PORT;
}

string portTechnologyToString(PortTechnology t)
{
	switch (t) {
	case IVSHMEM_PORT:
		return "ivshmem";
		break;
	case USVHOST_PORT:
		return "usvhost";
		break;
	case VHOST_PORT:
		return "vhost";
		break;
	case VETH_PORT:
		return "veth";
		break;
	case DPDKR_PORT:
		return "dpdkr";
		break;
	case UNDEFINED_PORT:
		return "undefined";
		break;
	default:
		break;
	}
	return "INVALID";
}





nf_t stringToVnfType(const std::string& type){

    if(!type.compare("dpdk"))
    {
        return DPDK;
    }

    else if(!type.compare("docker"))
    {
        return DOCKER;
    }

    else if(!type.compare("virtual-machine-kvm"))
    {
        return KVM;
    }

    else if(!type.compare("native"))
    {
        return NATIVE;
    }

	else if(!type.compare("iomodule"))
    {
        return IOMODULE;
    }	
	
	return UNDEFINED;


    //[+] Add here other implementations for the execution environment

}


string uriTypeToString(uri_t type){

	switch (type) {
		case REMOTE_FILE:
			return "remote-file";
			break;
		case LOCAL_FILE:
			return "local-file";
			break;
		case DOCKER_REGISTRY:
			return "docker-registry";
			break;
		default:
			break;
	}
	return "INVALID";
}




string vnfTypeToString(nf_t type)
{
#if defined(ENABLE_DPDK_PROCESSES) || defined(VSWITCH_IMPLEMENTATION_XDPD)
	if(type == DPDK)
			return string("dpdk");
#endif
#if defined(ENABLE_DOCKER) || defined(VSWITCH_IMPLEMENTATION_XDPD)
	if(type == DOCKER)
			return string("docker");
#endif
#ifdef ENABLE_KVM
	if(type == KVM)
			return string("virtual-machine-kvm");
#endif
#ifdef ENABLE_NATIVE
	if(type == NATIVE)
			return string("native");
#endif
#ifdef ENABLE_IOMODULE
	if(type == IOMODULE)
			return string("iomodule");
#endif

	//[+] Add here other implementations for the execution environment

	assert(0);
	return "";
}
