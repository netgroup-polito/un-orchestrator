#ifndef NF_PORT_CONFIGURATION_H_
#define NF_PORT_CONFIGURATION_H_ 1

#include <string>

using namespace std;

/**
*	@brief: information related to the network configuration of a VNF port
*/
typedef struct port_network_config
{
	/**
	*	@brief: MAC address to be assigned to the port
	*		E.g., aa:bb:cc:dd:ee:ff
	*/
	string mac_address;

	/**
	*	@brief: indicate whether the port is trusted or not
	*/
	bool trusted;
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	/**
	*	@brief: IPv4/netmask to be assigned to the port
	*		E.g., 10.0.0.1/24
	*/
	string ip_address;
#endif
}port_network_config_t;

/**
*	@brief: information between a port mapping that has to be set up between
*		a TCP port of the guest and a TCP port of the host
*/
struct port_mapping_t
{
	string host_port;
	string guest_port;
};

inline bool operator<(const port_mapping_t &lhs, const port_mapping_t &rhs)
{
	return false;
}

#endif //NF_PORT_CONFIGURATION_H_
