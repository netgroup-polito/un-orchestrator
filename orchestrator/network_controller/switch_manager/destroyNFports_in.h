#ifndef DestroyNFportsIn_H_
#define DestroyNFportsIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <string>
#include <inttypes.h>

/**
* @file destroyNFports_in.h
*
* @brief Description of network function ports destroyed.
*/

using namespace std;

class DestroyNFportsIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the lsi to which the network function ports must be removed
	*/
	uint64_t dpid;

	/**
	*	@brief: id of the network functions whose ports must be destroyed
	*/
	string nf_id;

	/**
	*	@brief: network function ports to be destroyed
	*/
	set<string> nf_ports;

protected:
	DestroyNFportsIn(uint64_t dpid, string nf_id, set<string> nf_ports)
		: dpid(dpid), nf_id(nf_id), nf_ports(nf_ports)
	{
	}

public:

	uint64_t getDpid()
	{
		return dpid;
	}

	string getNfId()
	{
		return nf_id;
	}

	set<string> getNFports()
	{
		return nf_ports;
	}

};


#endif //DestroyNFportsIn_H_
