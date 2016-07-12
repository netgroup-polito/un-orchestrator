#include "high_level_graph_endpoint_internal.h"

namespace highlevel
{

EndPointInternal::EndPointInternal(string id, string name, string group) :
	id(id), name(name), group(group)
{

}

EndPointInternal::~EndPointInternal()
{

}

bool EndPointInternal::operator==(const EndPointInternal &other) const
{
	if(id == other.id && name == other.name && group == other.group)
		return true;

	return false;
}

string EndPointInternal::getId()
{
	return id;
}

string EndPointInternal::getName()
{
	return name;
}

string EndPointInternal::getGroup()
{
	return group;
}

Object EndPointInternal::toJSON()
{
	Object EndPointInternal, internal;

	EndPointInternal[_ID] = id.c_str();
	EndPointInternal[_NAME] = name.c_str();
	EndPointInternal[EP_TYPE] = EP_INTERNAL;

	internal[INTERNAL_GROUP] = group;

	EndPointInternal[EP_INTERNAL] = internal;

	return EndPointInternal;
}

}
