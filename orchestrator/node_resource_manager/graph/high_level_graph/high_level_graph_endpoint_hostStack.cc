#include "high_level_graph_endpoint_hostStack.h"

namespace highlevel
{

EndPointHostStack::EndPointHostStack(string id, string name, hoststack_conf_t configuration, string ipAddress) :
	id(id), name(name), configuration(configuration), ipAddress(ipAddress)
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

	hostStack[CONFIGURATION] = configuration;
	if(configuration == STATIC)
		hostStack[IP_ADDRESS] = ipAddress;

	EndPointHoststack[EP_HOSTSTACK] = hostStack;

	return EndPointHoststack;
}

}
