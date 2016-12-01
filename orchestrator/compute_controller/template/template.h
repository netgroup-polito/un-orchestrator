#pragma once

#include "port.h"
#include <string>
#include <list>

using namespace std;
class Template {
private:
    string capability;
    bool expandable;
    string uri;
    string uriType;
    string vnfType;
    list<Port> ports;
    int cores;

public:
    Template();
    ~Template();
    void setCapability(string capability);
    string getCapability();
    void setExpandable(bool expandable);
    bool getExpandable();
    void setURI(string uri);
    string getURI();
    void setVnfType(string type);
    string getVnfType();
    void addPort(Port port);
    list<Port>& getPorts();
    void setCores(int cores);
    int getCores();
    string getURIType();
    void setURIType(string uriType);


};