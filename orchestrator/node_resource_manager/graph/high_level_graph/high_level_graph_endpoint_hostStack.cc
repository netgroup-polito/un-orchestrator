#include "high_level_graph_endpoint_hostStack.h"

namespace highlevel
{

EndPointHostStack::EndPointHostStack(string id, string name, string configuration, string ipAddress) :
	id(id), name(name), ipAddress(ipAddress)
{
	if(configuration==CONF_STATIC)
		this->configuration=STATIC;
	else if(configuration==CONF_DHCP)
		this->configuration=DHCP;
	else if(configuration==CONF_PPPOE)
		this->configuration=PPPOE;
}

EndPointHostStack::~EndPointHostStack()
{

}

bool EndPointHostStack::operator==(const EndPointHostStack &other) const
{
	if(id == other.id && name == other.name && configuration == other.configuration && ipAddress == other.ipAddress)
		return true;

	return false;
}

string EndPointHostStack::getId()
{
	return id;
}

string EndPointHostStack::getName()
{
	return name;
}

string EndPointHostStack::getIpAddress()
{
	return ipAddress;
}

Object EndPointHostStack::toJSON()
{
	Object EndPointManagement, hostStack;

	EndPointManagement[_ID] = id.c_str();
	EndPointManagement[_NAME] = name.c_str();
	EndPointManagement[EP_TYPE] = EP_HOSTSTACK;

	hostStack[CONFIGURATION] = configuration;
	if(configuration == STATIC)
		hostStack[IP_ADDRESS] = ipAddress;

	EndPointManagement[EP_HOSTSTACK] = hostStack;

	return EndPointManagement;
}

}
