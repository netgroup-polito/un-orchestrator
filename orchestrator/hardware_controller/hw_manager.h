#include <string>

using namespace std;

class HWManager
{
private:

string id;
string functionalCapability;

public:
	virtual ~HWManager();
	virtual attachHardware() = 0;
	virtual detachHardware() = 0;
}
