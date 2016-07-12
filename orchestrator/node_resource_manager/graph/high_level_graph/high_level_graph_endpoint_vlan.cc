#include "high_level_graph_endpoint_vlan.h"

namespace highlevel
{

EndPointVlan::EndPointVlan(string id, string name, string vlan_id, string interface) :
	id(id), name(name), vlan_id(vlan_id), interface(interface)
{

}

EndPointVlan::~EndPointVlan()
{

}

bool EndPointVlan::operator==(const EndPointVlan &other) const
{
	if(id == other.id && name == other.name && interface == other.interface && vlan_id == other.vlan_id)
		return true;

	return false;
}

string EndPointVlan::getId()
{
	return id;
}

string EndPointVlan::getName()
{
	return name;
}

string EndPointVlan::getVlanId()
{
	return vlan_id;
}

string EndPointVlan::getInterface()
{
	return interface;
}

Object EndPointVlan::toJSON()
{
	Object EndPointVlan, vlan;

	EndPointVlan[_ID] = id.c_str();
	EndPointVlan[_NAME] = name.c_str();
	EndPointVlan[EP_TYPE] = VLAN;

	vlan[V_ID] = vlan_id.c_str();
	vlan[IF_NAME] = interface.c_str();

	EndPointVlan[VLAN] = vlan;

	return EndPointVlan;
}

}
