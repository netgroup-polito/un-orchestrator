#include "template.h"

using namespace std;
Template::Template() {

}

Template::~Template() {

}

void Template::setName(string name) {
    this->name = name;
}

void Template::setExpandable(bool expandable) {
    this->expandable = expandable;
}

void Template::setURI(string uri) {
    this->uri = uri;
}

void Template::setVnfType(string type) {
    this->vnfType = type;
}

void Template::addPort(Port port) {
    this->ports.push_back(port);
    //this->port_types.insert(std::map<unsigned int, PortType>::value_type(port_id, port_type));
}

bool Template::getExpandable() {
    return this->expandable;
}

string Template::getName() {
    return this->name;
}

string Template::getURI() {
    return this->uri;
}

string Template::getVnfType() {
    return this->vnfType;
}

list<Port>& Template::getPorts() {
    return this->ports;
}

void Template::setCores(int cores) {
    this->cores = cores;
}

int Template::getCores() {
    return this->cores;
}


