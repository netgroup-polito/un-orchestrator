#include "dpdk_description.h"

DPDKDescription::~DPDKDescription(){}

DPDKDescription::DPDKDescription(nf_t type, std::string uri, std::string cores, std::map<unsigned int, PortType>& port_types) :
		Description(type, uri, port_types), cores(cores){}

DPDKDescription::DPDKDescription(std::string type, std::string uri, std::string cores, std::map<unsigned int, PortType>& port_types) :
		Description(type, uri, port_types), cores(cores){}

std::string DPDKDescription::getCores() const {
	return cores;
}


