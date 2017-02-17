#ifndef HW_MANAGER_H
#define HW_MANAGER_H 1
#include <string>
#include "../compute_controller/nf.h"
#include "../compute_controller/description.h"

using namespace std;
class NF;
class Description;

class HWManager
{
private:

	string id;
	string functionalCapability;
	bool multiTenancy;

public:
	virtual ~HWManager();
	virtual bool attachHardware(NF *) = 0;
	virtual void detachHardware(NF *) = 0;
	virtual bool supportNetworkFunction(Description *) = 0;
	virtual void initializeHardware() = 0;
};

#endif
