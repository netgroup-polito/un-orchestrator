#include "dpdk_description.h"

DPDKDescription::~DPDKDescription(){}

DPDKDescription::DPDKDescription(nf_t type, std::string uri, std::string cores/*, std::string location*/, std::map<unsigned int, PortType>& port_types) :
		Description(type, uri, port_types), cores(cores)/*, location(location) */{}

DPDKDescription::DPDKDescription(std::string type, std::string uri, std::string cores/*, std::string location*/, std::map<unsigned int, PortType>& port_types) :
		Description(type, uri, port_types), cores(cores)/*, location(location) */{}

std::string DPDKDescription::getCores() const {
	return cores;
}

/*std::string DPDKDescription::getLocation() const {
	return location;
}*/
