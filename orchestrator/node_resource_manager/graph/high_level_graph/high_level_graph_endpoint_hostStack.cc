#include "high_level_graph_endpoint_hostStack.h"

namespace highlevel
{

EndPointHostStack::EndPointHostStack(string id, string name, hoststack_conf_t configuration, string ipAddress, string macAddress) :
	id(id), name(name), configuration(configuration), ipAddress(ipAddress), macAddress(macAddress)
{
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

string EndPointHostStack::getMacAddress()
{
	return macAddress;
}

hoststack_conf_t EndPointHostStack::getConfiguration()
{
	return configuration;
}

Object EndPointHostStack::toJSON()
{
	Object EndPointHoststack, hostStack;

	EndPointHoststack[_ID] = id.c_str();
	EndPointHoststack[_NAME] = name.c_str();
	EndPointHoststack[EP_TYPE] = EP_HOSTSTACK;

	switch (configuration)
	{
		case DHCP:
			hostStack[CONFIGURATION]=CONF_DHCP;
			break;
		case STATIC:
			hostStack[CONFIGURATION]=CONF_STATIC;
			hostStack[IP_ADDRESS] = ipAddress.c_str();
			break;
		case PPPOE:
			hostStack[CONFIGURATION]=CONF_PPPOE;
			break;
	}
	if(macAddress!="")
		hostStack[MAC_ADDRESS] = macAddress.c_str();
	EndPointHoststack[EP_HOSTSTACK] = hostStack;

	return EndPointHoststack;
}

}
