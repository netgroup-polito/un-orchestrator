#ifndef VIRTUAL_LINK_H_
#define VIRTUAL_LINK_H_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string>

using namespace std;

class VLink
{
friend class LSI;

private:
	//FIXME: protect it with a mutex?
	static uint64_t nextID;

	uint64_t ID;

protected:
	uint64_t remote_dpid;
	unsigned int local_id;
	unsigned int remote_id;

public:
	VLink(uint64_t remote_dpid);
	uint64_t getRemoteDpid();
	unsigned int getLocalID();
	unsigned int getRemoteID();
	uint64_t getID();

};

#endif //VIRTUAL_LINK_H_
