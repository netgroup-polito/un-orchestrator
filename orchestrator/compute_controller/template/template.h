#pragma once

#include "port.h"
#include <string>
#include <list>

using namespace std;
class Template {
private:
    string name;
    bool expandable;
    string uri;
    string uriType;
    string vnfType;
    list<Port> ports;
    //map<unsigned int, PortType> port_types;string uriType;
    int cores;

public:
    Template();
    ~Template();
    void setName(string name);
    string getName();
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