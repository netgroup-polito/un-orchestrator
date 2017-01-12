#ifndef HW_MANAGER_H
#define HW_MANAGER_H 1
#include <string>
#include "../compute_controller/nfs_manager.h"
#include "../compute_controller/description.h"

using namespace std;

class NFsManager;
class Description;

class HWManager
{
private:

	string id;
	string functionalCapability;
	bool multiTenancy;

public:
	virtual ~HWManager();
	virtual bool attachHardware(NFsManager *) = 0;
	virtual void detachHardware(NFsManager *) = 0;
	virtual bool supportNetworkFunction(Description *) = 0;
	virtual void initializeHardware() = 0;
};

#endif
