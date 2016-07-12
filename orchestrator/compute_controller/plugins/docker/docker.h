#ifndef DOCKER_H_
#define DOCKER_H_ 1

#pragma once

#include "../../nfs_manager.h"
#include "docker_constants.h"

#include <string>
#include <sstream>
#include <stdlib.h>

using namespace std;

class Docker : public NFsManager
{
private:

#if 0
	/**
	*	@brief: starting from a netmask, returns the /
	*
	*	@param:	netmask	Netmask to be converted
	*/
	unsigned int convertNetmask(string netmask);
#endif

public:
	bool isSupported(Description&);

	bool startNF(StartNFIn sni);
	bool updateNF(UpdateNFIn uni);
	bool stopNF(StopNFIn sni);
};

#endif //DOCKER_H_
