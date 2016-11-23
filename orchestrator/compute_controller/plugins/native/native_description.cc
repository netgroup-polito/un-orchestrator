#include "native_description.h"

NativeDescription::~NativeDescription(){
	delete requirements;
}

NativeDescription::NativeDescription(nf_t type, std::string uri,/* std::string location, std::list<std::string>* requirements,*/ std::map<unsigned int, PortType>& port_types)
		: Description(type, uri, port_types)/*, location(location), requirements(requirements)*/{}



NativeDescription::NativeDescription(std::string type, std::string uri,/* std::string location, std::list<std::string>* requirements,*/ std::map<unsigned int, PortType>& port_types)
		: Description(type, uri, port_types)/*, location(location), requirements(requirements)*/{}

std::list<std::string>* NativeDescription::getRequirements() const {
	return requirements;
}

std::string NativeDescription::getLocation() const {
	return location;
}
