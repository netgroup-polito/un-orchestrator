
#ifndef DESCRIPTION_H_
#define DESCRIPTION_H_ 1
#pragma once
#include "nfs_manager.h"
#include "../utils/logger.h"
#include "../utils/constants.h"
#include "template/NFtemplate.h"

#include <string>
#include <vector>
#include <assert.h>
#include "port_technology.h"
#include "uri_type.h"
using namespace std;

class NFsManager;



PortTechnology portTechnologyFromString(const std::string& port);
string portTechnologyToString(PortTechnology port);
nf_t stringToVnfType(const std::string& vnfType); //convert docker in the nf_t DOCKER for example
string vnfTypeToString(nf_t vnfType);
uri_t stringToUriType(const std::string& uriType);
string uriTypeToString(uri_t uriType);
struct nf_port_info
{
	string port_name;
	PortTechnology port_technology;
	unsigned int port_id;
};
bool operator==(const nf_port_info& lhs, const nf_port_info& rhs);

class Description
{

private:
	NFtemplate* temp;
	bool supported;
	std::map<unsigned int, PortTechnology> port_technologies;

public:
	Description(NFtemplate* temp, std::map<unsigned int, PortTechnology>& port_technologies);
	virtual ~Description();
	NFtemplate * getTemplate();
	void setSupported(bool supported);
	bool isSupported();
	const std::map<unsigned int, PortTechnology>& getPortTechnologies() const { return port_technologies; }
	PortTechnology getPortTechnology(unsigned int port_id) const;
};

#endif //DESCRIPTION_H_
