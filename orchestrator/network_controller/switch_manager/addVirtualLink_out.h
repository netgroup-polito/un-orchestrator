#ifndef AddVirtualLinkOut_H_
#define AddVirtualLinkOut_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/**
* @file addVirtualLink_out.h
*
* @brief Description of the connection created between two lsis.
*/

using namespace std;

class AddVirtualLinkOut
{
private:
	/**
	*	@brief: identifier of the virtual link on the lsi A
	*/
	uint64_t idA;

	/**
	*	@brief: identifier of the virtual link on the lsi B
	*/
	uint64_t idB;

public:
	uint64_t getIdA()
	{
		return idA;
	}

	uint64_t getIdB()
	{
		return idB;
	}

	AddVirtualLinkOut(uint64_t idA, uint64_t idB)
		: idA(idA), idB(idB)
	{
	}

};


#endif //AddVirtualLinkOut_H_
