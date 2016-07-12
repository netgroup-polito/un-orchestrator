#ifndef DestroyVirtualLinkIn_H_
#define DestroyVirtualLinkIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/**
* @file destroyVirtualLink_in.h
*
* @brief Description of the connection between two lsis that must be destroyed.
*/

using namespace std;

class DestroyVirtualLinkIn
{

friend class GraphManager;

private:

	/**
	*	@brief: identifier of the first lsi connected through the virtual link
	*/
	uint64_t dpid_a;

	/**
	*	@brief: identifier of the virtual link on the lsi A
	*/
	uint64_t idA;


	/**
	*	@brief: identifier of the second lsi connected through the virtual link
	*/
	uint64_t dpid_b;

	/**
	*	@brief: identifier of the virtual link on the lsi B
	*/
	uint64_t idB;

protected:
	DestroyVirtualLinkIn(uint64_t dpid_a, uint64_t idA, uint64_t dpid_b, uint64_t idB)
		: dpid_a(dpid_a), idA(idA), dpid_b(dpid_b), idB(idB)
	{
	}

public:

	uint64_t getDpidA()
	{
		return dpid_a;
	}

	uint64_t getIdA()
	{
		return idA;
	}

	uint64_t getDpidB()
	{
		return dpid_b;
	}

	uint64_t getIdB()
	{
		return idB;
	}

};


#endif //DestroyVirtualLinkIn_H_
