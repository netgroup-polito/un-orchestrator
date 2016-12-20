#include "native_description.h"

NativeDescription::~NativeDescription(){

}

NativeDescription::NativeDescription(NFtemplate * temp, std::map<unsigned int, PortTechnology>& port_technologies)
		: Description(temp, port_technologies){}

std::list<std::string> NativeDescription::getRequirements() const {
	return requirements;
}

std::string NativeDescription::getLocation() const {
	return location;
}
