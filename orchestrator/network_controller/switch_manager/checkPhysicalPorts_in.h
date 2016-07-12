#ifndef CheckPhysicalPorts_in_H_
#define CheckPhysicalPorts_in_ 1

#pragma once

#define __STDC_FORMAT_MACROS

#include <string>
#include <list>
#include <inttypes.h>
#include <assert.h>

/**
* @file checkPhysicalPorts_in.h
*
* @brief Description of a physical port to be handled by the node orchestrator through the
*		 virtual switch.
*/

using namespace std;

/**
*	@brief: different virtual switches may handle differently ethernet and wifi ports
*/
enum physicalPortType_t {ETHERNET_PORT,WIFI_PORT};

class CheckPhysicalPortsIn
{

friend class GraphManager;

private:

	/**
	*	@brief: name of the physical port
	*/
	string name;

	/**
	*	@brief: type of the physical port
	*/
	physicalPortType_t type;


protected:
	CheckPhysicalPortsIn(string name, physicalPortType_t type = ETHERNET_PORT)
		: name(name), type(type)
	{
	}

public:

	string getPortName() const
	{
		return name;
	}

	physicalPortType_t getPortType() const
	{
		return type;
	}

	string getPortTypeToString()
	{
		switch(type)
		{
			case ETHERNET_PORT:
				return string("ethernet");
				break;
			case WIFI_PORT:
				return string("wifi");
				break;
			default:
				assert(0);
				return "";
		}
	}

	//XXX this operator is required to put an object of this class into a set
	bool operator< (const CheckPhysicalPortsIn& lhs) const
	{
		return true;
	}
};

#endif //CheckPhysicalPorts_in_
