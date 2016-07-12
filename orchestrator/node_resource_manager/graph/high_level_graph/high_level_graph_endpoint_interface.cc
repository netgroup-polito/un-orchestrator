#include "high_level_graph_endpoint_interface.h"

namespace highlevel
{

EndPointInterface::EndPointInterface(string id, string name, string interface) :
	id(id), name(name), interface(interface)
{
}

EndPointInterface::~EndPointInterface()
{
}

bool EndPointInterface::operator==(const EndPointInterface &other) const
{
	if(id == other.id && name == other.name && interface == other.interface)
		return true;

	return false;
}

string EndPointInterface::getId()
{
	return id;
}

string EndPointInterface::getName()
{
	return name;
}

string EndPointInterface::getInterface()
{
	return interface;
}

Object EndPointInterface::toJSON()
{
	Object EndPointInterface, iface;

	EndPointInterface[_ID] = id.c_str();
	EndPointInterface[_NAME] = name.c_str();
	EndPointInterface[EP_TYPE] = EP_IFACE;

	iface[IF_NAME] = interface.c_str();

	EndPointInterface[EP_IFACE] = iface;

	return EndPointInterface;
}

}
