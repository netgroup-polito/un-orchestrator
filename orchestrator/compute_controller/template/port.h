#include <string>
#include <cstring>
#include "../description.h"

using namespace std;

class Port{
private:
	string portsRange;
	PortType technology;

public:
	Port();
	void setPortsRange(string portsRange);
	void setTechnology(PortType technology);
	string getPortsRange();
	PortType getTechnology();
	void  splitPortsRangeInInt(int& begin, int& end);  //it splits portsRange in integers,so i can add in a map each port with the appropriate technology through a loop

};
