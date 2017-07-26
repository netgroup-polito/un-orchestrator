#include "port.h"
#include <sstream>
#include <stdlib.h>
TPort::TPort() {}


void TPort::setPortsRange(string portsRange){
	this->portsRange = portsRange;
}

void TPort::setTechnology(PortTechnology technology) {
	this->technology = technology;
}

string TPort::getPortsRange() {
	return  this->portsRange;
}

PortTechnology TPort::getTechnology() {
	return this->technology;
}


void TPort::splitPortsRangeInInt(int& begin, int& end){
	string token;
	stringstream is(this->portsRange);
	int i = 0;
	while(getline(is, token, '-')) {
		if (!i)
			begin = atoi(token.c_str());
		else
			if(!token.compare("N"))
				end = -1;
			else
				end = atoi(token.c_str());
		i++;
	}
}
