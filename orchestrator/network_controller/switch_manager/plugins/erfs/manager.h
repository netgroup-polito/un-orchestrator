#ifndef ERFSManager_H_
#define ERFSManager_H_ 1

#pragma once

#include "../../switch_manager.h"

#include "../../../../utils/constants.h"
#include "../../../../utils/sockutils.h"
#include "../../../../utils/logger.h"

#include "constants.h"

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

class ERFSManager : public SwitchManager
{
    typedef map<string,unsigned int> PortsNameIdMap;

private:
    uint64_t nextLsi;
    map<uint64_t, unsigned int> nextPort; // ports are assigned per LSI
//	unsigned int m_NextPortId;

public:
	ERFSManager();

	~ERFSManager();

	//
	// SwitchManager interface
	//

	CreateLsiOut *createLsi(CreateLsiIn cli);

	AddNFportsOut *addNFPorts(AddNFportsIn anpi);

	AddVirtualLinkOut *addVirtualLink(AddVirtualLinkIn avli);

	void destroyLsi(uint64_t dpid);

	void destroyNFPorts(DestroyNFportsIn dnpi);

	void destroyVirtualLink(DestroyVirtualLinkIn dvli);

	void checkPhysicalInterfaces(set<CheckPhysicalPortsIn> cppi);
};

class ERFSManagerException: public SwitchManagerException
{
public:
	virtual const char* what() const throw()
	{
		return "ERFSManagerException";
	}
};

#endif //ERFSManager_H_

