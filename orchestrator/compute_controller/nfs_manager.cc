#include "nfs_manager.h"

void NFsManager::setDescription(Description *description)
{
	this->description = description;
}

nf_t NFsManager::getNFType()
{
	return description->getTemplate()->getVnfType();
}

string NFsManager::getCores()
{
	return "";
}

NFsManager::~NFsManager(){

}
