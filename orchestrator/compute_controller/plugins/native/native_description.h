#ifndef NATIVE_DESCRIPTION_H_
#define NATIVE_DESCRIPTION_H_ 1

#include <list>

#include "../../description.h"

class Description;

class NativeDescription : public Description {
private:
	std::string location;
	std::list<std::string> *requirements;
public:
	std::list<std::string>* getRequirements() const;
	std::string getLocation() const;
	NativeDescription(nf_t type, std::string uri,/* std::string location, std::list<std::string>* requirements,*/ std::map<unsigned int, PortType>& port_types);
	NativeDescription(std::string type, std::string uri,/* std::string location, std::list<std::string>* requirements,*/ std::map<unsigned int, PortType>& port_types);
	~NativeDescription();
};

#endif //NATIVE_DESCRIPTION_H_
