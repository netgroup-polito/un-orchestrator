#include "sample_driver.h"

SampleDriver::SampleDriver()
{
}


SampleDriver::~SampleDriver()
{
}

bool SampleDriver::attachHardware(NFsManager *nf)
{
	return true;
}


void SampleDriver::detachHardware(NFsManager *nf)
{
}


void SampleDriver::initializeHardware()
{
}

bool SampleDriver::supportNetworkFunction(Description *description)
{
        return false;
}
