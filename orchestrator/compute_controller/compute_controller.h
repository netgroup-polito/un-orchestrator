#ifndef COMPUTE_CONTROLLER_H_
#define COMPUTE_CONTROLLER_H_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <list>

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <pthread.h>
#include <inttypes.h>

#include "../utils/logger.h"
#include "../utils/constants.h"
#include "../utils/sockutils.h"
#include "nf.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "nfs_manager.h"

#ifdef ENABLE_DPDK_PROCESSES
	#include "plugins/dpdk/dpdk.h"
#endif
#ifdef ENABLE_DOCKER
	#include "plugins/docker/docker.h"
#endif
#ifdef ENABLE_KVM
	#include "plugins/kvm-libvirt/libvirt.h"
#endif
#ifdef ENABLE_NATIVE
	#include "plugins/native/native.h"
#endif
//[+] Add here other implementations for the execution environment

using namespace std;
using namespace json_spirit;

#define NAME_RESOLVER_ADDRESS		"localhost"
#define NAME_RESOLVER_PORT			"2626"
#define NAME_RESOLVER_BASE_URL		"/nfs/"
#define NAME_RESOLVER_DIGEST_URL	"digest/"

#define CODE_POSITION				9
#define CODE_METHOD_NOT_ALLLOWED	"405"
#define CODE_OK						"200"

class Implementation;

typedef enum{NFManager_OK,NFManager_SERVER_ERROR, NFManager_NO_NF}nf_manager_ret_t;

class ComputeController
{
private:

	/**
	*	@brief: mutex used to select the core to be allocated to a (DPDK) NF
	*/
	static pthread_mutex_t nfs_manager_mutex;

	/**
	*	@brief: the CPU cores that can be allocated to DPDK NFs
	*/
	static map<int,uint64_t> cores;

	/**
	*	@brief: position, in the vector above, of the next CPU core to be
	*	allocated to a DPDK NF
	*/
	static int nextCore;

	/**
	* 	@brief: the pair is <network function id, network function>
	**/
	map<string, NF*> nfs;

	/**
	*	@brief: identifier of the LSI attached to the NFs
	**/
	uint64_t lsiID;

	/**
	*	@brief: parse the JSON answer received from the name translator database
	*
	*	@param:	answer	Answer to be parsed
	*	@param:	nf		Name of the network function whose description must be in the asnwer
	*/
	bool parseAnswer(string answer, string nf_name, string nf_id);

	/**
	*	@brief: calculate the core mask for a DPDK NF
	*
	*	@param:	coresRequired	Number of cores required
	*/
	uint64_t calculateCoreMask(string coresRequired);

	/**
	*	@brief: For all the NF without an already selected implementation, select
	*	an implementation of the desired type, if at least one implementation
	*	of such a type is available
	*
	*	@param:	desiredType	Type of the implementation to be selected
	*/
	void selectImplementation(nf_t desiredType);

	/**
	*	@brief: Check if an implementation for all the NFs has been selected. In this
	*	case the return value is true, otherwise it is false
	*/
	bool allSelected();

	/**
	 * 	@brief: Check if each description is supported or not
	 */
	void checkSupportedDescriptions();

	NFsManager* selectNFImplementation(list<Description*> descriptions);

public:
	ComputeController();
	~ComputeController();

	/**
	*	@brief: Retrieve, from the name resolver, the information for a specific NF
	*
	*	@param:	nf	Name of a network function
	*/
	nf_manager_ret_t retrieveDescription(string nf_id, string nf_name, string name_resolver_ip, int name_resolver_port);

	/**
	*	@brief: For each NF, select an implementation. Currently, if a Docker implementation
	*	is available and Docker is running with the LXC engine, Docker is selected.
	*	Otherwise, a DPDK implementation is selected. Only in case Docker and DPDK
	*	implementations are not available, it selects a KVM implementation (if KVM is
	*	running and a KVM implementation is available).
	*	Summarizing, the priority of the implementations is the following
	*		- Docker
	*		- DPDK
	*		- KVM
	*/
	bool selectImplementation();

	/**
	*	@brief: Return the type selected for a specific NF
	*
	*	@param:	name	Name of a network function
	*/
	nf_t getNFType(string id);

	/**
	 *	@brief: Returns the description of the selected NF implementation.
	 *
	 *	@param: name	Name of the network function
	 */
	const Description* getNFSelectedImplementation(string id);

	/**
	*	@brief: Set the identifier of the identifier of the LSI attached to the NFs
	*
	*	@param:	lsiID	Identifier of an LSI
	*/
	void setLsiID(uint64_t lsiID);

	/**
	*	@brief: Start the NF with a specific name, with a proper number of ports.
	*
	*	@param:	nf_name					Name of the network function to be started
	*	@param: namesOfPortsOnTheSwitch	Names of ports on the vSwitch that are related to the network function to be started
	*	@param: portsConfiguration		Ports configuration (mac address, ip address)
	*	@param: controlConfiguration	Control ports configuration (host TCP port, VNF TCP port)
	*	@param: environmentVariables	Environment variables to be set to the VNF
	*/
	bool startNF(string nf_id, map<unsigned int, string> namesOfPortsOnTheSwitch, map<unsigned int, port_network_config_t > portsConfiguration
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		, list<port_mapping_t > controlConfiguration, list<string> environmentVariables
#endif
		);
	
	/**
	*	@brief: Update the NF with a specific name, doing the hotplug of the new ports.
	*
	*	@param:	nf_name					Name of the network function to be started
	*	@param: namesOfPortsOnTheSwitch	Names of ports on the vSwitch that are related to the network function to be started
	*	@param: portsConfiguration		Ports configuration (mac address, ip address)
	*	@param: newPortsToAdd			Ports that needs to be added on the NF

	*/
	bool updateNF(string nf_name, map<unsigned int, string> namesOfPortsOnTheSwitch, map<unsigned int, port_network_config_t > portsConfiguration, list<unsigned int> newPortsToAdd);
	/**
	*	@brief: Stop all the running NFs
	*/
	void stopAll();

	/**
	*	@brief: Stop a specific NF
	*
	*	@param:	nf_name	Name of the network function to be stopped
	*/
	bool stopNF(string nf_id);

	/**
	*	@brief: Set the core mask representing the cores to be used for DPDK processes. The available cores will
	*	be allocated to DPDK NFs in a round robin fashion, and each DPDK network functions will have just one
	*	core
	*
	*	@param:	core_mask	Mask representing the cores to be allocated to DPDK network functions
	*/
	static void setCoreMask(uint64_t core_mask);

	/**
	*	@brief: prints information on the VNFs deployed
	*/
	void printInfo(int graph_id);
};

#endif //COMPUTE_CONTROLLER_H_
