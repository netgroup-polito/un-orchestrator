#ifndef DESCRIPTION_H_
#define DESCRIPTION_H_ 1

#include "nf_type.h"
#include "nfs_manager.h"

#pragma once

#include "../utils/logger.h"
#include "../utils/constants.h"

#include <string>
#include <vector>
#include <assert.h>

using namespace std;

class NFsManager;

enum PortTechnology {
	INVALID_PORT = -1,
	UNDEFINED_PORT = 0,
	//Ports used for virtual machines
	USVHOST_PORT,			//user space vhost port
	IVSHMEM_PORT,			//ivshmem port
	VHOST_PORT,				//(in kernel) vhost port
	//Ports used fro Docker containers
	VETH_PORT,				//veth pair port
	//Ports used for DPDK processes executed in the host
	DPDKR_PORT				//dpdkr port
};

PortTechnology portTechnologyFromString(const std::string& s);
std::string portTechnologyToString(PortTechnology t);
nf_t stringToType(const std::string& type); //convert docker in the nf_t DOCKER for example
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
	//FIXME-ENNIO: many infomation stored in this class are also stored in the class template. Please remove them from here and add a pointer to the proper template, in order
	//to avoid duplicate information
	nf_t type;
	string uri;
	string capability;
	string uri_type;
	bool supported;
	std::map<unsigned int, PortTechnology> port_technologies;

public:
	Description(nf_t type, string uri, std::map<unsigned int, PortTechnology>& port_technologies);
	Description(string type, string uri, std::map<unsigned int, PortTechnology>& port_technologies);
	Description(string type, string uri,string capability,string uri_type, std::map<unsigned int, PortTechnology>& port_technologies);//usefull for docker where uri type is required by the script
	virtual ~Description();
	void setURI(string uri);
	string getURI() const;
	string getURIType() const;
	string getCapability() const;
	nf_t getType() const;
	void setSupported(bool supported);
	bool isSupported();
	const std::map<unsigned int, PortTechnology>& getPortTechnologies() const { return port_technologies; }
	PortTechnology getPortTechnology(unsigned int port_id) const;
};

#endif //DESCRIPTION_H_
