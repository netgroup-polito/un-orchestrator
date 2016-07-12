#ifndef AddEndpointIn_H_
#define AddEndpointIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <string>
#include <list>
#include <inttypes.h>

/**
* @file addEndpoint_in.h
*
* @brief Description of endpoint to be created.
*/

using namespace std;

class AddEndpointIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the lsi to which the endpoint must be connected
	*/
	uint64_t dpid;

	/**
	*	@brief: name of the endpoint that must be connected to the lsi
	*/
	string ep_name;

	/**
	*	@brief: parameters related to the endpoint [key, local_ip, remote_ip, interface]
	*/
	vector<string> ep_param;

protected:
	AddEndpointIn(uint64_t dpid, string ep_name, vector<string> ep_param)
		: dpid(dpid), ep_name(ep_name), ep_param(ep_param)
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

	vector<string> getEPparam()
	{
		return ep_param;
	}
};


#endif //AddEndpointIn_H_
