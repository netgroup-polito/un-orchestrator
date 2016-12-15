#ifndef HW_MANAGER_H
#define HW_MANAGER_H 1
#include <string>
#include "../compute_controller/nfs_manager.h"

using namespace std;

class HWManager
{
private:

	string id;
	string functionalCapability;
	bool multiTenancy;

public:
	virtual ~HWManager();
	virtual void attachHardware(NFsManager *) = 0;
	virtual void detachHardware() = 0;
	virtual void initializeHardware() = 0;
};

#endif
