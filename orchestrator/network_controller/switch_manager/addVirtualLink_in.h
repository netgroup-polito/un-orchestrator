#ifndef AddVirtualLinkIn_H_
#define AddVirtualLinkIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/**
* @file addVirtualLink_in.h
*
* @brief Description of the connection that must be created between two lsis.
*/

using namespace std;

class AddVirtualLinkIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the first lsi connected through the virtual link
	*/
	uint64_t dpid_a;

	/**
	*	@brief: identifier of the second lsi connected through the virtual link
	*/
	uint64_t dpid_b;

public:

	AddVirtualLinkIn(uint64_t dpid_a, uint64_t dpid_b)
		: dpid_a(dpid_a), dpid_b(dpid_b)
	{
	}

	uint64_t getDpidA()
	{
		return dpid_a;
	}

	uint64_t getDpidB()
	{
		return dpid_b;
	}
};

#endif //AddVirtualLinkIn_H_
