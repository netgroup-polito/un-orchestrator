#include "description.h"

bool operator==(const nf_port_info& lhs, const nf_port_info& rhs)
{
    return (lhs.port_name.compare(rhs.port_name) == 0) && (lhs.port_type == rhs.port_type);
}



Description::Description(nf_t type, string uri, std::map<unsigned int, PortType>& port_types) :
        type(type), uri(uri), port_types(port_types)
{
    supported = false;
}

Description::Description(string type, string uri, std::map<unsigned int, PortType>& port_types) :
		 uri(uri), port_types(port_types)
{
	supported = false;
    this->type=stringToType(type);

    return;
}

Description::Description(string type, string uri,string nf_name,string uri_type ,std::map<unsigned int, PortType>& port_types) :
	 uri(uri) ,nf_name(nf_name),uri_type(uri_type), port_types(port_types)
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

string Description::getName() const {
	return nf_name;
}

bool Description::isSupported() {
	return supported;
}

void Description::setSupported(bool supported) {
	this->supported = supported;
}

PortType Description::getPortType(unsigned int port_id) const
{
	std::map<unsigned int, PortType>::const_iterator it = port_types.find(port_id);
	if (it != port_types.end()) {
		return it->second;
	}
	return UNDEFINED_PORT;  // TODO: Should we make this INVALID_PORT to notify an error? Question is also: do we make the port specification in the NF description mandatory?
}

PortType portTypeFromString(const std::string& s)
{
	if (s.compare("ivshmem") == 0)
		return IVSHMEM_PORT;
	else if (s.compare("usvhost") == 0)
		return USVHOST_PORT;
	else if (s.compare("vhost") == 0)
		return VHOST_PORT;

	return INVALID_PORT;
}



std::string portTypeToString(PortType t)
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


    //[+] Add here other implementations for the execution environment

    assert(0);

}
