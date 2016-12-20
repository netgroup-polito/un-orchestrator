#ifndef NATIVE_DESCRIPTION_H_
#define NATIVE_DESCRIPTION_H_ 1

#include <list>

#include "../../description.h"

class Description;

class NativeDescription : public Description {
private:
	std::string location;
	std::list<std::string> requirements;
public:
	std::list<std::string> getRequirements() const;
	std::string getLocation() const;
	NativeDescription(NFtemplate *temp, std::map<unsigned int, PortTechnology>& port_technologies);
	~NativeDescription();
};

#endif //NATIVE_DESCRIPTION_H_
