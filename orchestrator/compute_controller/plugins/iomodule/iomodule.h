#ifndef IOMODULE_H_
#define IOMODULE_H_1

#pragma once

#include "../../nfs_manager.h"
#include "iomodule_constants.h"

#include <string>
#include <sstream>
#include <stdlib.h>

using namespace std;

class IOmodule : public NFsManager
{

public:
	bool isSupported(Description&);

	bool startNF(StartNFIn sni);
	bool updateNF(UpdateNFIn uni);
	bool stopNF(StopNFIn sni);
};

#endif //IOMODULE_H_
