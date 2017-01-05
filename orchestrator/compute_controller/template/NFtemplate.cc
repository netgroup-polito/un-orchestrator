#include "NFtemplate.h"

using namespace std;
NFtemplate::NFtemplate() {
	this->cores=0;
}

NFtemplate::~NFtemplate() {
}

void NFtemplate::setName(string name) {
	this->name = name;
}

void NFtemplate::setCapability(string capability) {
	this->capability = capability;
}

void NFtemplate::setExpandable(bool expandable) {
	this->expandable = expandable;
}

void NFtemplate::setURI(string uri) {
	this->uri = uri;
}

void NFtemplate::setURIType(uri_t uriType) {
	this->uriType = uriType;
}

void NFtemplate::setVnfType(nf_t type) {
	this->vnfType = type;
}

void NFtemplate::setPlatform(string platform) {
	this->platformType = platform;
}

void NFtemplate::addPort(Port port) {
	this->ports.push_back(port);
}

string NFtemplate::getName() {
	return this->name;
}

bool NFtemplate::getExpandable() {
	return this->expandable;
}

string NFtemplate::getCapability() {
	return this->capability;
}

string NFtemplate::getURI() {
	return this->uri;
}

nf_t NFtemplate::getVnfType() {
	return this->vnfType;
}

list<Port>& NFtemplate::getPorts() {
	return this->ports;
}

void NFtemplate::setCores(int cores) {
	this->cores = cores;
}

int NFtemplate::getCores() {
	return this->cores;
}

uri_t NFtemplate::getURIType() {
	return this->uriType;
}

string NFtemplate::getPlatform() {
	return this->platformType;
}

list<string> NFtemplate::getRequirements(){
	return this->requirements;
}



