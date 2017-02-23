#ifndef AddEndpointHoststackOut_H_
#define AddEndpointHoststackOut_ 1

#pragma once

#include <string>
#include <list>

/**
* @file addEndpoint_out.h
*
* @brief Description of endpoint.
*/

using namespace std;

class AddEndpointHoststackOut
{

friend class GraphManager;

private:

	/**
	*	@brief: id of the endpoint that have been connected to the lsi
	*/
	string ep_id;

	/**
	*	@brief: openflow id of the connected endpoint
	*/
	unsigned int of_id;

	/**
	*	@brief: port name of the hoststack endpoint
	*/
	string port_name;

protected:

	string getEpId()
	{
		return ep_id;
	}

	unsigned int getOfId()
	{
		return of_id;
	}

	string getEpPortName()
	{
		return port_name;
	}
public:
	AddEndpointHoststackOut(string ep_id,unsigned int of_id, string port_name)
		: ep_id(ep_id), of_id(of_id), port_name(port_name)
	{
	}

};


#endif //AddEndpointOut_H_
