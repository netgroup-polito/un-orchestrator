#ifndef StopNFIn_H_
#define StopNFIn_ 1

#pragma once

#include <string>

/**
* @file StopNFIn_in.h
*
* @brief Parameters to be used to stop the network function.
*/

using namespace std;

class StopNFIn
{

friend class ComputeController;

private:

	/**
	*	@brief: identifier of the LSI to which the VNF is connected
	*/
	uint64_t lsiID;

	/**
	*	@brief: id of the network function
	*/
	string nf_id;

protected:
	StopNFIn(uint64_t lsiID, string nf_id)
		: lsiID(lsiID), nf_id(nf_id)
	{
	}

public:

	uint64_t getLsiID()
	{
		return lsiID;
	}

	string getNfId()
	{
		return nf_id;
	}
};


#endif //StopNFIn_H_
