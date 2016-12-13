#include "port.h"
#include <sstream>

Port::Port() {}


void Port::setPortsRange(string portsRange){
	this->portsRange = portsRange;
}

void Port::setTechnology(string technology) {
	this->technology = technology;
}

string Port::getPortsRange() {
	return  this->portsRange;
}

string Port::getTechnology() {
	return this->technology;
}


void Port::splitPortsRangeInInt(int& begin, int& end){
	string token;
	stringstream is(this->portsRange);
	int i = 0;
	while(getline(is, token, '-')) {
		if (!i)
			begin = atoi(token.c_str());
		else
			end = atoi(token.c_str());
		i++;
	}
}
