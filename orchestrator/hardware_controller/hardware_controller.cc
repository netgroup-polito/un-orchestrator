#include "hardware_controller.h"
#include "manifest_list.h"

HardwareController::HardwareController()
{
	this->listOfHardware = getHardwareManifestList();
}

HardwareController::~HardwareController()
{
}


bool HardwareController::checkHWAvailability(Description *description)
{
	return true;
}
