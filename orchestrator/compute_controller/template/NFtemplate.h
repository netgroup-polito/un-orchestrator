#pragma once
#include "port.h"
#include <string>
#include <list>

/**
*	@brief: This class contains a network function template such as the following
*
{
	"name": "switch",
	"functional-capability" : "l2-switch",
	"uri": "http://localhost:8081/v2/nf_image/1",
	"vnf-type": "virtual-machine-kvm",
	"uri-type": "remote-file",
	"ports": [
		{
			"position": "0-2",
			"label": "L2Port",
			"min": "1",
			"ipv4-config": "none",
			"ipv6-config": "none",
			"name": "eth",
			"technology":"vhost"
		}
	],
	"expandable": false,
	"memory-size": 2048,
	"root-file-system-size": 40,
	"ephemeral-file-system-size": 0,
	"swap-disk-size": 0,
	"CPUrequirements": {
		"platformType": "x86",
		"socket": [
			{
				"coreNumbers": 1
			}
		]
	}
}
*
*/

using namespace std;
class NFtemplate {
private:
	string name;
	string capability;
	string uri;
	string uriType;
	string vnfType;
	bool expandable;
	int memorySize;
	int coreNumbers;
	string platformType;
	list<Port> ports;
	int cores;

public:
	NFtemplate();
	~NFtemplate();
	void setName(string name);
	string getName();
	void setCapability(string capability);
	string getCapability();
	void setExpandable(bool expandable);
	bool getExpandable();
	void setURI(string uri);
	string getURI();
	void setVnfType(string type);
	string getVnfType();
	void addPort(Port port);
	list<Port>& getPorts();
	void setCores(int cores);
	int getCores();
	void setPlatform(string platform);
	string getPlatform();
	string getURIType();
	void setURIType(string uriType);
};
