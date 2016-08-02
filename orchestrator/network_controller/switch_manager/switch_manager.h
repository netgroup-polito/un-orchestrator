#ifndef SwitchManager_H_
#define SwitchManager_H_ 1

#pragma once

#include "createLSIin.h"
#include "createLSIout.h"
#include "addNFports_in.h"
#include "addNFports_out.h"
#include "destroyNFports_in.h"
#include "addVirtualLink_in.h"
#include "addVirtualLink_out.h"
#include "destroyVirtualLink_in.h"
#include "checkPhysicalPorts_in.h"
#include "addEndpoint_in.h"
#include "addEndpoint_out.h"
#include "destroyEndpoint_in.h"

#include <stdio.h>

/**
* @file switch_manager.h
*
* @brief Switch manager interface. It must be properly implemented for each vSwitch supported by the node.
*/

class SwitchManager
{
public:

	/**
	*	@brief: Creates a new LSI
	*
	*	@param: cli		Description of the lsi to be created
	*	@return: 		Information related to the lsi created
	*/
	virtual CreateLsiOut *createLsi(CreateLsiIn cli) = 0;

	/**
	*	@brief: Create ports for a specific network function, on a specific lsi
	*
	*	@brief: anpi	Description of the ports to be created
	*	@return:		Information related to the ports created
	*/
	virtual AddNFportsOut *addNFPorts(AddNFportsIn anpi) = 0;
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
	/**
	*	@brief: Create endpoint, on a specific lsi
	*
	*	@brief: aepi	Description of the endpoint to be created
	*	@return:		Information related to the endpoint created
	*/
	virtual AddEndpointOut *addEndpoint(AddEndpointIn aepi) = 0;
#endif
	/**
	*	@brief: Connect together two lsis
	*
	*	@param: avli	Description of the connection to be created
	*	@return:		Information on the connection created
	*/
	virtual AddVirtualLinkOut *addVirtualLink(AddVirtualLinkIn avli) = 0;

	/**
	*	@brief: Destroy an existing lsi
	*
	*	@param: dpid	Identifier of the lsi to be destroyed
	*/
	virtual void destroyLsi(uint64_t dpid) = 0;

	/**
	*	@brief: Destroy ports of a specific network function
	*
	*	@brief: dnpi	Information related to the ports to be destroyed
	*/
	virtual void destroyNFPorts(DestroyNFportsIn dnpi) = 0;
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
	/**
	*	@brief: Destroy endpoint of a specific lsi
	*
	*	@brief: depi	Information related to the endpoint to be destroyed
	*/
	virtual void destroyEndpoint(DestroyEndpointIn depi) = 0;
#endif
	/**
	*	@brief: Destroy a connection between two lsis
	*
	*	@param: dvli	Information related to the connection to be destroyed
	*/
	virtual void destroyVirtualLink(DestroyVirtualLinkIn dvli) = 0;

	/**
	*	@brief: Check if the physical interfaces required are supported by the virtual switch
	*
	*	@param:	dppi	Description of the physical interfaces to be handled
	*					by the node orchestrator through the virtual switch
	*/
	virtual void checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi) = 0;

	/**
	 * virtual destructor is needed
	 */
	virtual ~SwitchManager(){};

};

class SwitchManagerException: public std::exception
{
public:
	virtual const char* what() const throw()
	{
		return "SwitchManagerException";
	}
};

#endif //SwitchManager_H_
