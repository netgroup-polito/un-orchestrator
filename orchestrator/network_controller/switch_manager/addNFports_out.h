#ifndef AddNFportsOut_H_
#define AddNFportsOut_ 1

#pragma once

#include <string>
#include <list>

/**
* @file addNFports_out.h
*
* @brief Description of network function ports destroyed.
*/

using namespace std;

class AddNFportsOut
{

friend class GraphManager;

private:

	/**
	*	@brief: name of the network functions whose ports have been connected to the lsi
	*/
	string nf_name;

	/**
	*	@brief: map of ports name, identifier within the lsi
	*		The name of the network function ports is that selected by the node resource manager based on the graph, and will
	*		be later used by the node resource manager to properly create the graph.
	*/
	map<string, unsigned int> ports;

	/**
	*	@brief: list of ports on the vSwitch that are associated with the network function
	*		The name of the port created on the switch may differ from the name used by the node resource manager to refer to
	*		such a port. Since the name of the ports created on the switch must be used by the compute controller and conneted
	*		to the network function, these names must be returned to the node resource manager.
	*		Note that the name of the ports defined by the node resource manager and the name of the ports connected to the
	*		vSwitch can be the same.
	*/
	list<string> ports_name_on_switch;

public:

	string getNFname()
	{
		return nf_name;
	}

	map<string, unsigned int> getPorts()
	{
		return ports;
	}

	list<string> getPortsNameOnSwitch()
	{
		return ports_name_on_switch;
	}

	AddNFportsOut(string nf_name,map<string, unsigned int> ports, list<string> ports_name_on_switch)
		: nf_name(nf_name), ports(ports.begin(),ports.end()), ports_name_on_switch(ports_name_on_switch.begin(),ports_name_on_switch.end())
	{
	}

};


#endif //AddNFportsOut_H_
