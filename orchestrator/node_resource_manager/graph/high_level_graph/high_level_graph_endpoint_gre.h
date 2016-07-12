#ifndef HIGH_LEVEL_GRAPH_ENPOINT_GRE_H_
#define HIGH_LEVEL_GRAPH_ENPOINT_GRE_H_ 1

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include <iostream>
#include <sstream>

#include <string.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;
using namespace std;

namespace highlevel
{

class EndPointGre
{
private:
	/**
	*	@brief: the id of the endpoint (e.g., 00000003)
	*/
	string id;

	/**
	*	@brief: the name of the endpoint port (e.g., ingress)
	*/
	string name;

	/**
	*	@brief: the local ip of the gre tunnel (e.g., 10.0.0.1)
	*/
	string local_ip;

	/**
	*	@brief: the remote ip of the gre tunnel (e.g., 10.0.0.2)
	*/
	string remote_ip;

	/**
	*	@brief: the key of the gre tunnel (e.g., 1)
	*/
	string gre_key;

	/**
	*	@brief: the ttl of the gre tunnel (e.g., 2)
	*/
	string ttl;

	/**
	*	@brief: true (GRE tunnel over IPsec), false (GRE tunnel)
	*/
	bool is_safe;

public:

	EndPointGre(string id, string name, string local_ip, string remote_ip, string gre_key, string ttl, bool is_safe);

	/**
	*	@brief: return the ID of the endpoint
	*/
	string getId();

	/**
	*	@brief: return the name of the endpoint
	*/
	string getName();

	/**
	*	@brief: return the local IP of the endpoint
	*/
	string getLocalIp();

	/**
	*	@brief: return the remote IP of the endpoint
	*/
	string getRemoteIp();

	/**
	*	@brief: return the key of the endpoint
	*/
	string getGreKey();

	/**
	*	@brief: return the TTL of the endpoint
	*/
	string getTtl();

	/**
	*	@brief: return true in case secure GRE is required for this endpoint
	*/	
	bool isSafe();

	~EndPointGre();

	bool operator==(const EndPointGre &other) const;

	Object toJSON();
};

}

#endif //HIGH_LEVEL_GRAPH_ENPOINT_GRE_H_
