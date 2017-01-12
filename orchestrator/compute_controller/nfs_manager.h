#ifndef NFsManager_H_
#define NFsManager_H_ 1

#pragma once

#include "nf_type.h"
#include "startNF_in.h"
#include "updateNF_in.h"
#include "stopNF_in.h"
#include "description.h"

#include <typeinfo>

#ifdef ENABLE_HARDWARE
#include "../hardware_controller/hardware_controller.h"
#endif

/**
* @file nfs_manager.h
*
* @brief Network functions manager interface. It must be properly implemented for each vSwitch supported by the node.
*/

using namespace std;

class Description;

#ifdef ENABLE_HARDWARE
class HWManager;
#endif

class NFsManager
{
protected:
	/**
	*	@brief: Description of the network function associated with this manager
	*/
	Description *description;

#ifdef ENABLE_HARDWARE
	/**
	*	@brief: Object that describe the Hardware attached to the NF it is equal
			to NULL if no hardware associated
	**/
	HWManager *hwAttached;
#endif

public:

	virtual ~NFsManager();

	/**
	*	@brief: check whether the execution environment is supported or not
	*/
	virtual bool isSupported(Description& description) = 0;

	/**
	*	@brief:	Retrieve and start the network function
	*
	*	@param: cli	Description of the network function to be started
	*/
	virtual bool startNF(StartNFIn sni) = 0;

	/**
	*	@brief:	Retrieve and update the network function
	*
	*	@param: cli	Description of the network function to be updated
	*/
	virtual bool updateNF(UpdateNFIn uni) = 0;

	/**
	*	@brief: stop the network function
	*
	*	@param: cli	Description of the network function to be stopped
	*/
	virtual bool stopNF(StopNFIn sni) = 0;

	/**
	*	@brief: set the description of the network function to be handled by the manager
	*/
	void setDescription(Description *description);

	/**
	*	@brief: set the description of the network function to be handled by the manager
	*/
	const Description* getDescription() { return description; }

	/**
	*	@brief: provide the type of the network function handled by the manager
	*/
	nf_t getNFType();

	/**
	*	@brief: returns the number of cores to be associated with the network function
	*			handled by the manager. "" means that no core has to be bound to the
	*			network function.
	*			If an execution environment needs this information this method must be overridden
	*/
	virtual string getCores();

#ifdef ENABLE_HARDWARE
	/**
	*	@brief: returns the object of the hardware associated to this NF
	**/
	const HWManager* getHWAttached(){ return hwAttached;}

	/**
	*       @brief: set the hardware attached to the NF
	**/
	void setHWAttached(HWManager *);
#endif
};

class NFsManagerException: public std::exception
{
public:
	virtual const char* what() const throw()
	{
		return "NFsManagerException";
	}
};

#endif //NFsManager_H_
