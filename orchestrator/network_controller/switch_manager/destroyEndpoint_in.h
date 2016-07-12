#ifndef DestroyEndpointIn_H_
#define DestroyEndpointIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <string>
#include <inttypes.h>

/**
* @file destroyEndpoint_in.h
*
* @brief Description of endpoint destroyed.
*/

using namespace std;

class DestroyEndpointIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the lsi to which the endpoint must be removed
	*/
	uint64_t dpid;

	/**
	*	@brief: name of the endpoint that must be destroyed
	*/
	string ep_name;

protected:
	DestroyEndpointIn(uint64_t dpid, string ep_name)
		: dpid(dpid), ep_name(ep_name)
	{
	}

public:

	uint64_t getDpid()
	{
		return dpid;
	}

	string getEPname()
	{
		return ep_name;
	}

};


#endif //DestroyEndpointIn_H_
