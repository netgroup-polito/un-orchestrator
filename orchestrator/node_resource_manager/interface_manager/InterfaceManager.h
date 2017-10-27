#ifndef NODE_ORCHESTRATOR_INTERFACEMANAGER_H
#define NODE_ORCHESTRATOR_INTERFACEMANAGER_H

#include <list>
#include <map>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <unistd.h>

#include "../../utils/constants.h"
#include "../../utils/logger.h"


using namespace std;

class InterfaceManager
{
private:

    /**
    *	port name - dhclient PID (for ports with a DHCP configuration)
    */
    list<unsigned int> dhclientPID;

    /**
    *	@brief: kill dhclient processes
    */
    void closeDhcpClients();

public:

    InterfaceManager();
    ~InterfaceManager();

    /**
    *	@brief: set a static IP address with netmask in slash notation
    */
    void setStaticIpAddress(string portName, string ipAddress);

    /**
    *	@brief: set a MAC address
    */
    void setMacAddress(string portName, string mac);

    /**
    *	@brief: obtain an IP address from DHCP
    */
    void getIpAddressFromDhcp(string portName);

};



#endif //NODE_ORCHESTRATOR_INTERFACEMANAGER_H
