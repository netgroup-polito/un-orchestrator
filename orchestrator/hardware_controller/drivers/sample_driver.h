#ifndef SAMPLE_DRIVER_H
#define SAMPLE_DRIVER_H 1
#include "../hw_manager.h"

class SampleDriver : public HWManager
{
public:
	SampleDriver();
	~SampleDriver();
	bool attachHardware(NFsManager *);
        void detachHardware(NFsManager *);
        void initializeHardware();
	bool supportNetworkFunction(Description *);
};


#endif
