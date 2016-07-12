#ifndef AddNFportsIn_H_
#define AddNFportsIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <string>
#include <list>
#include <inttypes.h>

#include "../../compute_controller/description.h"

/**
* @file addNFports_in.h
*
* @brief Description of network function ports to be created.
*/

using namespace std;

class AddNFportsIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the lsi to which the network function ports must be connected
	*/
	uint64_t dpid;

	/**
	*	@brief: id of the network functions whose ports must be connected to the lsi
	*/
	string nf_id;

	/**
	*	@brief: type of the network function to be connected to the lsi
	*/
	nf_t type;

	/**
	*	@brief: list of ports of the network function to be attached to the lsi.
	*			Each element of the list is in the form "port name, port type"
	*/
	list<struct nf_port_info> nf_ports;

public:
	AddNFportsIn(uint64_t dpid, string nf_id, nf_t type, list<struct nf_port_info> nf_ports)
		: dpid(dpid), nf_id(nf_id), type(type), nf_ports(nf_ports)
	{
	}

	uint64_t getDpid()
	{
		return dpid;
	}

	string getNfId()
	{
		return nf_id;
	}

	nf_t getNFtype()
	{
		return type;
	}

	list<struct nf_port_info> getNetworkFunctionsPorts()
	{
		return nf_ports;
	}

};


#endif //AddNFportsIn_H_
