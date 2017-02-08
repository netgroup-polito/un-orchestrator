#ifndef AddEndpointOut_H_
#define AddEndpointOut_ 1

#pragma once

#include <string>
#include <list>

/**
* @file addEndpoint_out.h
*
* @brief Description of endpoint.
*/

using namespace std;

class AddEndpointOut
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

protected:

	string getEpId()
	{
		return ep_id;
	}

	unsigned int getOfId()
	{
		return of_id;
	}

public:
	AddEndpointOut(string ep_id,unsigned int of_id)
		: ep_id(ep_id), of_id(of_id)
	{
	}

};


#endif //AddEndpointOut_H_
