#ifndef StartNFIn_H_
#define StartNFIn_ 1

#pragma once

#define __STDC_FORMAT_MACROS

#include <string>
#include <map>
#include <inttypes.h>
#include <list>

#include "../node_resource_manager/graph/high_level_graph/nf_port_configuration.h"

/**
* @file StartNFIn_in.h
*
* @brief Parameters to be used to start the network function.
*/

using namespace std;

class StartNFIn
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

	/**
	*	@brief: mapping of port_id to name of port on the vSwitch for ports associated with the network function
	*/
	map<unsigned int, string> namesOfPortsOnTheSwitch;

	/**
	*	@brief: mapping of port_id to configuration (mac_address, ip_address) associated with the network function
	*/
	map<unsigned int, port_network_config_t > portsConfiguration;

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	/**
	*	@brief: list of control ports associated with the network function
	*/
	list<port_mapping_t > controlPorts;

	/**
	*	@brief: list of environment variables to be configured in the network function
	*/
	list<string> environmentVariables;
#endif

	/**
	*	@brief: mask of the cores to be assigned to the network functon.
	*			0x0 means that no binding has to be done
	*/
	uint64_t coreMask;

protected:
	StartNFIn(uint64_t lsiID, string nf_id, map<unsigned int, string> namesOfPortsOnTheSwitch, map<unsigned int, port_network_config_t > portsConfiguration,
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	list<port_mapping_t > controlPorts, list<string> environmentVariables,
#endif
		uint64_t coreMask = 0x0)
			: lsiID(lsiID), nf_id(nf_id), namesOfPortsOnTheSwitch(namesOfPortsOnTheSwitch), portsConfiguration(portsConfiguration),
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
			controlPorts(controlPorts), environmentVariables(environmentVariables),
#endif
			coreMask(coreMask)
	{
	}

public:

	uint64_t getLsiID() const
	{
		return lsiID;
	}

	string getNfId() const
	{
		return nf_id;
	}

	const map<unsigned int, string>& getNamesOfPortsOnTheSwitch() const
	{
		return namesOfPortsOnTheSwitch;
	}

	const map<unsigned int, port_network_config_t >& getPortsConfiguration() const
	{
		return portsConfiguration;
	}

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	const list<port_mapping_t >& getControlPorts() const
	{
		return controlPorts;
	}

	const list<string>& getEnvironmentVariables() const
	{
		return environmentVariables;
	}
#endif

	uint64_t getCoreMask() const
	{
		return coreMask;
	}
};


#endif //StartNFIn_H_
