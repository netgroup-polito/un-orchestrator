#include "high_level_graph_endpoint_gre.h"

namespace highlevel
{

EndPointGre::EndPointGre(string id, string name, string local_ip, string remote_ip, string gre_key, string ttl, bool is_safe) :
	id(id), name(name), local_ip(local_ip), remote_ip(remote_ip), gre_key(gre_key), ttl(ttl), is_safe(is_safe)
{

}

EndPointGre::~EndPointGre()
{

}

bool EndPointGre::operator==(const EndPointGre &other) const
{
	if(id == other.id && name == other.name && local_ip == other.local_ip && remote_ip == other.remote_ip && gre_key == other.gre_key)
		return true;

	return false;
}

string EndPointGre::getId()
{
	return id;
}

string EndPointGre::getName()
{
	return name;
}

string EndPointGre::getLocalIp()
{
	return local_ip;
}

string EndPointGre::getRemoteIp()
{
	return remote_ip;
}

string EndPointGre::getGreKey()
{
	return gre_key;
}

string EndPointGre::getTtl()
{
	return ttl;
}

bool EndPointGre::isSafe()
{
	return is_safe;
}

Object EndPointGre::toJSON()
{
	Object EndPointGre, iface;

	EndPointGre[_ID] = id.c_str();
	EndPointGre[_NAME] = name.c_str();
	EndPointGre[EP_TYPE] = EP_GRE;

	iface[LOCAL_IP] = local_ip.c_str();
	iface[REMOTE_IP] = remote_ip.c_str();
	iface[GRE_KEY] = gre_key.c_str();
	if(strcmp(ttl.c_str(), "") != 0)
		iface[TTL] = ttl.c_str();
	iface[SAFE] = is_safe;

	EndPointGre[EP_GRE] = iface;

	return EndPointGre;
}

}
