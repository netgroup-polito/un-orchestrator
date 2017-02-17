#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H 1
#include "hw_manager.h"
#include "../compute_controller/description.h"

using namespace std;
class Description;
class HWManager;

class HardwareController
{
private:
	list<HWManager *> listOfHardware;
public:
	HardwareController();
	~HardwareController();
	bool checkHWAvailability(Description *);
};

#endif
