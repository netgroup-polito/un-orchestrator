#ifndef DestroyHoststackEndpointIn_H_
#define DestroyHoststackEndpointIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <string>
#include <inttypes.h>

/**
* @file destroyHoststackEndpoint_in.h
*
* @brief Description of the hoststack endpoint destroyed.
*/

using namespace std;

class DestroyHoststackEndpointIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the lsi to which the endpoint must be removed
	*/
	uint64_t dpid;

	/**
	*	@brief: id of the endpoint that must be destroyed
	*/
	string ep_id;

protected:
	DestroyHoststackEndpointIn(uint64_t dpid, string ep_id)
		: dpid(dpid), ep_id(ep_id)
	{
	}

public:

	uint64_t getDpid()
	{
		return dpid;
	}

	string getEpId()
	{
		return ep_id;
	}

};


#endif //DestroyHoststackEndpointIn_H_
