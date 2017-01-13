#ifndef SAMPLE_DRIVER_H
#define SAMPLE_DRIVER_H 1
#include "../hw_manager.h"

class SampleDriver : public HWManager
{
public:
	SampleDriver();
	~SampleDriver();
	bool attachHardware(NF *);
        void detachHardware(NF *);
        void initializeHardware();
	bool supportNetworkFunction(Description *);
};


#endif
