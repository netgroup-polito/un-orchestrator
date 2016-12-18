#include "dpdk_description.h"

DPDKDescription::~DPDKDescription(){}

DPDKDescription::DPDKDescription(nf_t type, std::string uri, std::string cores, std::map<unsigned int, PortTechnology>& port_technologies) :
		Description(type, uri, port_technologies), cores(cores){}

DPDKDescription::DPDKDescription(std::string type, std::string uri, std::string cores, std::map<unsigned int, PortTechnology>& port_technologies) :
		Description(type, uri, port_technologies), cores(cores){}

std::string DPDKDescription::getCores() const {
	return cores;
}


