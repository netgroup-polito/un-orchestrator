#ifndef DPDK_DESCRIPTION_H_
#define DPDK_DESCRIPTION_H_ 1

#include "../../description.h"

class DPDKDescription : public Description {
private:
	std::string cores;

public:

	std::string getCores() const;
	DPDKDescription(nf_t type, std::string uri, std::string cores, std::map<unsigned int, PortType>& port_types);
	DPDKDescription(std::string type, std::string uri, std::string cores, std::map<unsigned int, PortType>& port_types);
	~DPDKDescription();
};

#endif //DPDK_DESCRIPTION_H_
