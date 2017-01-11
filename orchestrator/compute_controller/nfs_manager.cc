#include "nfs_manager.h"

void NFsManager::setDescription(Description *description)
{
	this->description = description;
}

nf_t NFsManager::getNFType()
{
	return description->getType();
}

string NFsManager::getCores()
{
	return "";
}

NFsManager::~NFsManager(){

}

#ifdef ENABLE_HARDWARE
void NFsManager::setHWAttached(HWManager *hwAttached)
{
	this->hwAttached = hwAttached;
}
#endif
