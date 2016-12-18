#include "native_description.h"

NativeDescription::~NativeDescription(){

}

NativeDescription::NativeDescription(nf_t type, std::string uri, std::map<unsigned int, PortTechnology>& port_technologies)
		: Description(type, uri, port_technologies){}



NativeDescription::NativeDescription(std::string type, std::string uri,string nf_name,string uri_type, std::map<unsigned int, PortTechnology>& port_technologies)
		: Description(type, uri,nf_name,uri_type, port_technologies){}

std::list<std::string> NativeDescription::getRequirements() const {
	return requirements;
}

std::string NativeDescription::getLocation() const {
	return location;
}
