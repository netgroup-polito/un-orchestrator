#include <string>
#include <cstring>
#include "../port_technology.h"

using namespace std;

class Port{
private:
	string portsRange;
	PortTechnology technology;

public:
	Port();
	void setPortsRange(string portsRange);
	void setTechnology(PortTechnology technology);
	string getPortsRange();
	PortTechnology getTechnology();
	void  splitPortsRangeInInt(int& begin, int& end);  //it splits portsRange in integers,so i can add in a map each port with the appropriate technology through a loop

};

