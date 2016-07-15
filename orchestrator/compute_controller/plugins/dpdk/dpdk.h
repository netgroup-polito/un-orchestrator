#ifndef DPDK_H_
#define DPDK_H_ 1

#pragma once

#include "../../nfs_manager.h"
#include "dpdk_constants.h"
#include "dpdk_description.h"

#include <string>
#include <sstream>
#include <stdlib.h>

using namespace std;

class Dpdk : public NFsManager
{
public:

	bool isSupported(Description&);


	bool startNF(StartNFIn sni);
	bool stopNF(StopNFIn sni);
	bool updateNF(UpdateNFIn uni);

	string getCores();
};

#endif //DPDK_H_
