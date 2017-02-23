#ifndef AddEndpointHoststackIn_H_
#define AddEndpointHoststackIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <string>
#include <list>
#include <inttypes.h>

/**
* @file addEndpointHoststack_in.h
*
* @brief Description of hoststack endpoint to be created.
*/

using namespace std;

class AddEndpointHoststackIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the lsi to which the endpoint must be connected
	*/
	uint64_t dpid;

	/**
	*	@brief: id of the endpoint that must be connected to the lsi
	*/
	string ep_id;

protected:
	AddEndpointHoststackIn(uint64_t dpid, string ep_id)
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


#endif //AddEndpointHoststackIn_H_
