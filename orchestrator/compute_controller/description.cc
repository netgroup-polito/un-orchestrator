#include "description.h"

bool operator==(const nf_port_info& lhs, const nf_port_info& rhs)
{
    return (lhs.port_name.compare(rhs.port_name) == 0) && (lhs.port_technology == rhs.port_technology);
}



Description::Description(nf_t type, string uri, std::map<unsigned int, PortTechnology>& port_technologies) :
        type(type), uri(uri), port_technologies(port_technologies)
{
    supported = false;
}

Description::Description(string type, string uri, std::map<unsigned int, PortTechnology>& port_technologies) :
		 uri(uri), port_technologies(port_technologies)
{
	supported = false;
    this->type=stringToType(type);

    return;
}

Description::Description(string type, string uri,string capability,string uri_type ,std::map<unsigned int, PortTechnology>& port_technologies) :
	 uri(uri) ,capability(capability),uri_type(uri_type), port_technologies(port_technologies)
{
	supported = false;
    this->type = stringToType(type);
	return;
}

Description::~Description(){}

nf_t Description::getType() const
{
	return type;
}

string Description::getURI() const
{
	return uri;
}

void Description::setURI(string uri) {
	this->uri=uri;
}

string Description::getURIType() const {
	return uri_type;
}

string Description::getCapability() const {
	return capability;
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

std::string portTechnologyToString(PortTechnology t)
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



nf_t stringToType(const std::string& type){

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

	return UNDEFINED;


    //[+] Add here other implementations for the execution environment



}
