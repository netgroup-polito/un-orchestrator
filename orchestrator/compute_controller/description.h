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

enum PortType {
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

PortType portTypeFromString(const std::string& s);
std::string portTypeToString(PortType t);

struct nf_port_info
{
	string port_name;
	PortType port_type;
	unsigned int port_id;
};
bool operator==(const nf_port_info& lhs, const nf_port_info& rhs);

class Description
{

private:
	nf_t type;
	string uri;
	bool supported;
	std::map<unsigned int, PortType> port_types;

public:
	Description(nf_t type, string uri, std::map<unsigned int, PortType>& port_types);
	Description(string type, string uri, std::map<unsigned int, PortType>& port_types);
	virtual ~Description();

	string getURI() const;
	nf_t getType() const;
	void setSupported(bool supported);
	bool isSupported();
	const std::map<unsigned int, PortType>& getPortTypes() const { return port_types; }
	PortType getPortType(unsigned int port_id) const;
};

#endif //DESCRIPTION_H_
