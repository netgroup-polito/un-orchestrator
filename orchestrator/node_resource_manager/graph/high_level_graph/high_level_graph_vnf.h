#ifndef HIGH_LEVEL_GRAPH_VNFS_H_
#define HIGH_LEVEL_GRAPH_VNFS_H_ 1

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include "nf_port_configuration.h"

#include <iostream>
#include <sstream>

#include <string.h>
#include <list>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;
using namespace std;

namespace highlevel
{

/**
*	This structure represents a VNF port as described in the NF-FG
*
*	{
*		"id": "inout:0",
*		"name": "data-port"
*		"mac": "aa:bb:cc:dd:ee:ff",
*		"unify-ip": "192.168.0.1"
*	}
*
**/
typedef struct
{
	string id;
	string name;
	port_network_config_t configuration;
}vnf_port_t;

class VNFs
{
private:
	/**
	*	@brief: the id of the VNF (e.g., 00000003)
	*/
	string id;

	/**
	*	@brief: the name of the VNF (e.g., example)
	*/
	string name;

	/**
	*	@brief: the groups of the VNF (e.g., example)
	*/
	list<string> groups;

	/**
	*	@brief: the vnf template of the VNF (e.g., example.json)
	*/
	string vnf_template;

	/**
	*	@brief: the list of ports  of the VNF
	*/
	list<vnf_port_t> ports;

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	/**
	*	@brief: the list of control ports configuration of the VNF
	*/
	list<port_mapping_t> control_ports;

	/**
	*	@brief: list of environment variables to be set to the VNF.
	*			Each element of the list is in the form "variable=value"
	*/
	list<string> environment_variables;
#endif

	/**
	*	@brief: starting from the ID of a VNF port, return its index (i.e., the
	*			number of at the end of the ID)
	*/
	unsigned int extract_number_from_id(string port_id);

	/**
	*	@brief: this variable is used during the graph update, and indicates if the VNF has been
	*			changed as a consequence of the update
	*/
	bool updated;

public:

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	VNFs(string id, string name, list<string> groups, string vnf_template, list<vnf_port_t> ports, list<port_mapping_t> control_ports, list<string> environment_variables);
#else
	VNFs(string id, string name, list<string> groups, string vnf_template, list<vnf_port_t> ports);
#endif

	/**
	*	@brief: add a new port to the network function
	*/
	bool addPort(vnf_port_t port);

	string getId();
	string getName();
	list<string> getGroups();
	string getVnfTemplate();

	/*
	*	@brief: return the list of ports of the VNF
	*/
	list<vnf_port_t> getPorts();

	/*
	*	@brief: only return the list of port ID of the VNF
	*/
	list<unsigned int> getPortsId();

	/*
	*	@brief: return a mapping of port ID - port configuration, for all the
	*			ports of the VNF
	*/
	map<unsigned int, port_network_config > getPortsID_configuration();

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	/*
	*	@brief: return the list of control connections associated with the VNF
	*/
	list<port_mapping_t> getControlPorts();

	/*
	*	@brief: return the list of environment variables associated with the VNF
	*/
	list<string> getEnvironmentVariables();
#endif

	~VNFs();

	/**
	*	Check if two VNFs are the same. Note that they are the same if they have the same name.
	**/
	bool operator==(const VNFs &other) const;

	/**
	*	Create the json representing the VNF, according to the NF-FG formalism
	**/
	Object toJSON();
};

}

#endif //HIGH_LEVEL_GRAPH_VNFS_H_
