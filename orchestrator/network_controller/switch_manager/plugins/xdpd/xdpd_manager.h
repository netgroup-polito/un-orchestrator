#ifndef XDPDManager_H_
#define XDPDManager_ 1

#pragma once

#include "../../switch_manager.h"

#include "../../../../utils/constants.h"
#include "../../../../utils/sockutils.h"
#include "../../../../utils/logger.h"

#include "xdpd_constants.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include <string>
#include <list>
#include <sstream>
#include <getopt.h>

using namespace std;
using namespace json_spirit;

class LSI;

class XDPDManager : public SwitchManager
{
private:
	/**
	*	@brief: remote TCP port to be used to send commands to xDPd
	*/
	string xDPDport;

	/**
	*	@brief: keeps the addrinfo chain; required to open a new socket
	*/
	struct addrinfo *AddrInfo;


	/**
	*	@brief: set of ethernet interfaces available
	*/
	set<string> ethernetInterfaces;

	/**
	*	@brief: set of wireless interfaces available
	*/
	set<string> wirelessInterfaces;

	/**
	*	@brief: map of dpid, list of wireless interfaces commected
	*/
	map<uint16_t,list<string> > dpdiWirelessInterfaces;

	/**
	*	@brief: Send a message to xDPd
	*
	*	@param: message	Message to be sent
	*/
	string sendMessage(string message);

	/**
	*	@brief: the following methods are used to interact with xDPd
	*/

	string prepareCreateLSIrequest(CreateLsiIn cli, map<string,unsigned int> &port_name_id_in_graph);
	CreateLsiOut *parseCreateLSIresponse(CreateLsiIn cli, Object message, map<string,unsigned int> port_name_id_in_graph);

	string prepareDestroyLSIrequest(uint64_t dpid);
	void parseDestroyLSIresponse(Object message);

	string prepareCreateVirtualLinkRequest(AddVirtualLinkIn avli);
	AddVirtualLinkOut *parseCreateVirtualLinkResponse(AddVirtualLinkIn avli, Object message);

	string prepareDestroyVirtualLinkRequest(DestroyVirtualLinkIn dvli);
	void parseDestroyVirtualLinkResponse(Object message);

	string prepareCreateNFPortsRequest(AddNFportsIn anpi);
	AddNFportsOut *parseCreateNFPortsResponse(AddNFportsIn anpi, Object message, map<string, unsigned int> port_name_id_in_graph);

	string prepareDestroyNFPortsRequest(DestroyNFportsIn dnpi);
	void parseDestroyNFPortsResponse(Object message);

	bool findCommand(Object message, string expected);
	bool findStatus(Object message);

	/**
	*	@brief: attach the wireless interface of an lsi to a physical wireless port, by means of a Linux bridge.
	*		The bridge is created by this function.
	*
	*	@param: lsi	LSI with the port to manage
	*/
	bool attachWirelessPort(uint64_t dpid, string wirelessInterfaceName);

	/**
	*	@brief: detach the wireless interface of an lsi from a physical wireless port, by destroyng the Linux bridge
	*
	*	@param: lsi	LSI with the port to manage
	*/
	void detachWirelessPort(uint64_t dpid, string wirelessInterfaceName);

	/**
	*	@brief: check the command line provided to the xDPd module
	*/
	bool parseInputParams(int argc, char *argv[], char **file_name);

public:
	XDPDManager();

	~XDPDManager();

	CreateLsiOut *createLsi(CreateLsiIn cli);

	AddNFportsOut *addNFPorts(AddNFportsIn anpi);

	AddVirtualLinkOut *addVirtualLink(AddVirtualLinkIn avli);

	void destroyLsi(uint64_t dpid);

	void destroyNFPorts(DestroyNFportsIn dnpi);

	void destroyVirtualLink(DestroyVirtualLinkIn dvli);

	void checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi);
};

class XDPDManagerException: public SwitchManagerException
{
public:
	virtual const char* what() const throw()
	{
		return "xDPDManagerException";
	}
};

#endif //XDPDManager_H_

