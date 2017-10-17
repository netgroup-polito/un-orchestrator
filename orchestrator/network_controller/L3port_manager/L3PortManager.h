#ifndef PROJECT_L3PORTMANAGER_H
#define PROJECT_L3PORTMANAGER_H

#pragma once

#include <string>
#include <sstream>
#include "../../utils/logger.h"
#include "CreateNamespacePPPoE.h"

#define NAMESPACE_HELPER "helper"
#define CREATE_NAMESPACE_PPPoE "./network_controller/L3port_manager/scripts/create_namespacePPPoE.sh"
#define ADD_PORT_TO_NS "./network_controller/L3port_manager/scripts/add_port_to_ns.sh"

using namespace std;

class L3PortManager {
private:
    string L3Port;
public:
    L3PortManager();

    L3PortManager(string L3Port);

    ~L3PortManager();

    CreateNamespacePPPoE *createNamespacePPPoE();

    void setL3Port(string L3Port);
    string getL3Port();

    void addPortToNs(string port_name);
};

class L3PortManagerException: public std::exception
{
public:
    virtual const char* what() const throw()
    {
        return "L3PortManagerException";
    }
};

#endif //PROJECT_L3PORTMANAGER_H
