#include "sample_driver.h"

SampleDriver::SampleDriver()
{
}


SampleDriver::~SampleDriver()
{
}

bool SampleDriver::attachHardware(NF *nf)
{
	return true;
}


void SampleDriver::detachHardware(NF *nf)
{
}


void SampleDriver::initializeHardware()
{
}

bool SampleDriver::supportNetworkFunction(Description *description)
{
        return false;
}
