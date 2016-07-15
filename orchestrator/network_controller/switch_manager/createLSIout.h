#ifndef CreateLsiOut_H_
#define CreateLsiOut_ 1

#pragma once

#include <string>
#include <list>
#include <map>

/**
* @file createLSIout.h
*
* @brief Description of an LSI created.
*/

using namespace std;

class CreateLsiOut
{

friend class GraphManager;

private:
	uint64_t dpid;

	/**
	* @brief: list of physical port name, identifier on the lsi
	*/
	map<string,unsigned int> physical_ports;

	/**
	*	@brief: map of network functions id, map of network function ports name in the graph, network function ports identifier on the lsi
	*		(i.e., the number of the Openflow port). The name of the network function ports is that selected by the node resource
	*		manager based on the graph, and will be later used by the node resource manager to properly create the graph.
	*		Example:
				"VNFs": [
				{
					"id": "00000001",
					"name": "firewall",
					"ports": [
					{
						"id": "inout:0",
						"name": "data-port"
					},
					{
						"id": "inout:1",
						"name": "data-port"
					}
					]
				}
	*
	*			Starting from the previous piece of graph, we have the following entry in the map "00000001 - (00000001_0 - 0x5), (00000001_1 - 0x6)", 
	*			where 0x5 and 0x6 are generated/selected by the switch plugin.
	*/
	map<string,map<string, unsigned int> >  network_functions_ports;

	/**
	*	@brief: map of endpoint name, identifier on the lsi //FIXME: this is actually endpoints gre
	*/
	map<string,unsigned int >  endpoints_ports;

	/**
	*	@brief: map of network functions id, list of ports names on the vSwitch that are associated with such a network function.
	*		The name of the port created on the switch may differ from the name used by the node resource manager to refer to
	*		such a port. Since the name of the ports created on the switch must be used by the compute controller and connected
	*		to the network function, these names must be returned to the node resource manager.
	*		Note that the name of the ports defined by the node resource manager and the name of the ports connected to the
	*		vSwitch can be the same.
	*		Example:
				"VNFs": [
				{
					"id": "00000001",
					"name": "firewall",
					"ports": [
					{
						"id": "inout:0",
						"name": "data-port"
					},
					{
						"id": "inout:1",
						"name": "data-port"
					}
					]
				}
	*
	*			Starting from the previous piece of graph, we have the following entry in the map "00000001 - 'abc','def' ", where 'abc' and 'def'
	*			generated, selected by the switch plugin and it is visible through ifconfig. The port name on the switch may be visible through
	*			the Linux command 'ifconfig'.
	*/
	map<string, list<string> > nf_ports_name_on_switch;

	/**
	*	@brief: list of virtual link identifier on the new lsi, virtual link identifier on the remote lsi
	*/
	list<pair<unsigned int, unsigned int> > virtual_links;

	/**
	*	@brief: list of network functions id, with the associated port names on the switch and the port id on the graph
	*		Example:
				"VNFs": [
				{
					"id": "00000001",
					"name": "firewall",
					"ports": [
					{
						"id": "inout:0",
						"name": "data-port"
					},
					{
						"id": "inout:1",
						"name": "data-port"
					}
					]
				}
	*
	*			Starting from the previous piece of graph, we have the following entry in the map "00000001 - ('abc' - 0), ('def',1) ", where 'abc' and 'def'
	*			generated, selected by the switch plugin and it is visible through ifconfig. The port name on the switch may be visible through
	*			the Linux command 'ifconfig'.
	*/
	map<string, map<string, unsigned int> > nf_ports_name_and_id;

protected:

	uint64_t getDpid()
	{
		return dpid;
	}

	map<string,unsigned int> getPhysicalPorts()
	{
		return physical_ports;
	}

	map<string,map<string, unsigned int> > getNetworkFunctionsPorts()
	{
		return network_functions_ports;
	}

	map<string,unsigned int > getEndpointsPorts()
	{
		return endpoints_ports;
	}

	map<string, list<string> > getNetworkFunctionsPortsNameOnSwitch()
	{
		return nf_ports_name_on_switch;
	}

	list<pair<unsigned int, unsigned int> > getVirtualLinks()
	{
		return virtual_links;
	}
	
	map<string, map<string, unsigned int> > getNetworkFunctionsPortsNameAndID()
	{
		return nf_ports_name_and_id;
	}

public:

	CreateLsiOut(uint64_t dpid, map<string,unsigned int> physical_ports, map<string,map<string, unsigned int> >  network_functions_ports, map<string,unsigned int> endpoints_ports, map<string,list<string> > nf_ports_name_on_switch, list<pair<unsigned int, unsigned int> > virtual_links, map<string, map<string, unsigned int> > nf_ports_name_and_id)
		: dpid(dpid), physical_ports(physical_ports.begin(),physical_ports.end()),network_functions_ports(network_functions_ports.begin(),network_functions_ports.end()), endpoints_ports(endpoints_ports.begin(),endpoints_ports.end()),nf_ports_name_on_switch(nf_ports_name_on_switch.begin(),nf_ports_name_on_switch.end()), virtual_links(virtual_links.begin(),virtual_links.end()), nf_ports_name_and_id(nf_ports_name_and_id.begin(), nf_ports_name_and_id.end())
	{}

};

#endif //CreateLsiOut_H_
