#include "high_level_graph_endpoint_management.h"

namespace highlevel
{

EndPointManagement::EndPointManagement(string id, string name, bool isStatic, string ipAddress, string netmask) :
	id(id), name(name), is_static(isStatic), ipAddress(ipAddress), netmask(netmask)
{
}

EndPointManagement::~EndPointManagement()
{

}

bool EndPointManagement::operator==(const EndPointManagement &other) const
{
	if(id == other.id && name == other.name && ipAddress == other.ipAddress)
		return true;

	return false;
}

string EndPointManagement::getId()
{
	return id;
}

string EndPointManagement::getName()
{
	return name;
}

bool EndPointManagement::isStatic()
{
	return is_static;
}

string EndPointManagement::getIpAddress()
{
	return ipAddress;
}

string EndPointManagement::getIpNetmask()
{
	return netmask;
}

Object EndPointManagement::toJSON()
{
	Object EndPointManagement, management;

	EndPointManagement[_ID] = id.c_str();
	EndPointManagement[_NAME] = name.c_str();
	EndPointManagement[EP_TYPE] = EP_MANAGEMENT;

	management[STATIC_ADDRESS] = is_static;
	management[IP_ADDRESS] = ipAddress;
	management[NETMASK] = netmask;

	EndPointManagement[EP_MANAGEMENT] = management;

	return EndPointManagement;
}

}
