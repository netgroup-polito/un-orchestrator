#include "InterfaceManager.h"

InterfaceManager::InterfaceManager()
{
}

InterfaceManager::~InterfaceManager()
{
    closeDhcpClients();
}

void InterfaceManager::setStaticIpAddress(string portName, string ipAddress)
{
    stringstream cmd_set_ip_address;
    cmd_set_ip_address << "./node_resource_manager/interface_manager/scripts/set_ip_address.sh" << " " << portName << " " << CONF_STATIC << " " << ipAddress;
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_set_ip_address.str().c_str());
    int retVal = system(cmd_set_ip_address.str().c_str());
    retVal = retVal >> 8;
    if(retVal == 0)
        logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Failed to set the ip address to port %s", portName.c_str() );
}

void InterfaceManager::getIpAddressFromDhcp(string portName)
{
    stringstream cmd_set_ip_address;
    cmd_set_ip_address << "./node_resource_manager/interface_manager/scripts/set_ip_address.sh" << " " << portName << " " << CONF_DHCP;
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_set_ip_address.str().c_str());
    int retVal = system(cmd_set_ip_address.str().c_str());
    retVal = retVal >> 8;
    if(retVal == 0)
    {
        logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Failed to get an ip address to port %s", portName.c_str() );
        return;
    }
    unsigned int pid;
    ifstream f("/var/run/dhclient.pid");
    f >> pid;
    dhclientPID[portName]=pid;
}

void InterfaceManager::getIpAddressFromPppoe(string portName)
{
    logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Getting address from PPPOE is not supported yey", portName.c_str() );
}

void InterfaceManager::closeDhcpClients()
{
    for(map<string,unsigned int>::iterator dhclient = dhclientPID.begin(); dhclient!=dhclientPID.end(); dhclient++)
        kill(dhclient->second,SIGINT);
}