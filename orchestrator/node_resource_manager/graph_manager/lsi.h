#ifndef LSI_H_
#define LSI_H_ 1

#pragma once

#include "virtual_link.h"
#include "../../compute_controller/description.h"
#include "../graph/high_level_graph/high_level_graph_endpoint_gre.h"
#include "../graph/high_level_graph/high_level_graph_vnf.h"

#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

class VLink;

class LSI
{
//XXX: this class is a mess!

/**
*	@brief: This class is intended to represent the current situation on the LSI.
*		The it MUST only contain information related to the LSI used to implement
*		a single graph (and not information related to the configuration of the
*		VNFs).
*/

friend class GraphManager;

private:

	/**
	*	@brief: this is the address of the OF controller for this LSI
	*/
	string controllerAddress;

	/**
	*	@brief: this is the port used by the OF controller for the LSI
	*/
	string controllerPort;

	/**
	*	@brief: data plane identifier
	*/
	uint64_t dpid;

	/**
	*	@brief: the pair is <port name, port id>. It contains IDs assigned by the switch to the physical ports
	*/
	map<string,unsigned int> physical_ports;

	/**
	*	@brief: Data related to a specific NF
	*/
	struct nfData {
		/**
		*	@brief: Types of the NF ports.
		*		The map is
		*			<nf port name, port_type>
		*/
		map<string, PortType> ports_type;

		/**
		*	@brief: Names of the ports connected to the LSI and related to the network function
		*		The map is <port id, port name on switch>
		*/
		map<unsigned int,string> ports_name_on_switch;

		/**
		 * 	@brief: port ids (OpenFlow) assigned to the ports by switch
		 *		The map is
		 *			<nf port name, switch_id>
		 */
		map<string, unsigned int> ports_switch_id;

		/**
		 * 	@brief: port ids as they are designated in the NF-FG and NF description
		 */
		list<unsigned int> nf_ports_id;
	};

	/**
	*	@brief: list of gre-tunnel endpoints connected to the LSI
	*/
	list<highlevel::EndPointGre> gre_endpoints_ports;

	/**
	*	@brief: the pair is <endpoint name, endpoint id>
	*/
	map<string,unsigned int > endpoints_ports_id;

	/**
	*	@brief: NFs connected to the LSI.
	*		The map is
	*			<nf id, nfData >
	*/
	map<string, struct nfData>  network_functions;

	/**
	*	@brief: virtual links attached to the LSI
	*	FIXME although supported in this class VLink, the code does not support vlinks connected to multiple LSIs
	*/
	vector<VLink> virtual_links;

	/**
	*	@brief: the map is <nf name, vlink id>
	*		A NF port generates a vlink if it is defined in the action part of a rule
	*/
	map<string, uint64_t> nfs_vlinks;

	/**
	*	@brief: the map is <port name, vlink id>
	*		A physical port generates a vlink if it is defined in the action part of a rule
	*/
	map<string, uint64_t> ports_vlinks;

	/**
	*	@brief: the map is <endpoint name, vlink id>
	*		An endpoint generates a vlink if it is defined in the action part of a rule
	*/
	map<string, uint64_t> endpoints_vlinks;

	/**
	*	@brief: the map is <endpoint name, vlink id>
	*		An endpoint generates a vlink if it is defined in the action part of a rule
	*/
	map<string, uint64_t> endpoints_gre_vlinks;

public:

	LSI(string controllerAddress, string controllerPort, set<string> physical_ports, list<highlevel::VNFs> network_functions,
		list<highlevel::EndPointGre> gre_endpoints_ports, vector<VLink> virtual_links, map<string, map<unsigned int, PortType> > nfs_ports_type);

	string getControllerAddress();
	string getControllerPort();

	list<highlevel::EndPointGre> getEndpointsPorts();

	map<string,unsigned int > getEndpointsPortsId();

	uint64_t getDpid();

	list<string> getPhysicalPortsName();
	map<string,unsigned int> getPhysicalPorts();

	set<string> getNetworkFunctionsId();
	map<string,unsigned int> getNetworkFunctionsPorts(string nf_id);
	list<string> getNetworkFunctionsPortNames(string nf_id);
	PortType getNetworkFunctionPortType(string nf_id, string port);
	map<string, list< struct nf_port_info> > getNetworkFunctionsPortsInfo();
	map<unsigned int, string> getNetworkFunctionsPortsNameOnSwitchMap(string nf_id);

	list<uint64_t> getVirtualLinksRemoteLSI();
	vector<VLink> getVirtualLinks();
	VLink getVirtualLink(uint64_t ID);
	map<string, uint64_t> getNFsVlinks();
	map<string, uint64_t> getPortsVlinks();
	map<string, uint64_t> getEndPointsVlinks(); //TODO: rename in getEndPointsInternalVlinks
	map<string, uint64_t> getEndPointsGreVlinks();

	//FIXME: public is not a good choice
	void setNFsVLinks(map<string, uint64_t> nfs_vlinks);
	void addNFvlink(string NF, uint64_t vlinkID);
	void removeNFvlink(string nf_port);

	void setPortsVLinks(map<string, uint64_t> ports_vlinks);
	void addPortvlink(string port, uint64_t vlinkID);
	void removePortvlink(string port);

	void setEndPointsVLinks(map<string, uint64_t> endpoints_vlinks);
	void addEndpointInternalvlink(string endpoint, uint64_t vlinkID);
	void removeEndPointvlink(string endpoint);
	void setEndPointsGreVLinks(map<string, uint64_t> gre_endpoints_vlinks);
	void addEndpointGrevlink(string endpoint, uint64_t vlinkID);
	void removeEndPointGrevlink(string endpoint);

protected:
	void setDpid(uint64_t dpid);
	bool setPhysicalPortID(string port, uint64_t id);
	bool setNfSwitchPortsID(string nf_id, map<string, unsigned int>);
	void setVLinkIDs(unsigned int position, unsigned int localID, unsigned int remoteID);
	bool setEndpointPortID(string ep, uint64_t id);

	void setNetworkFunctionsPortsNameOnSwitch(string nf_id, map<string, unsigned int> names);

	int addVlink(VLink vlink);
	void removeVlink(uint64_t ID);

	bool addNF(string id, list< unsigned int> ports, const map<unsigned int, PortType>& nf_ports_type);
	void removeNF(string nf_id);

	void addEndpoint(highlevel::EndPointGre);
	void removeEndpoint(string ep);
};

#endif //LSI_H_
