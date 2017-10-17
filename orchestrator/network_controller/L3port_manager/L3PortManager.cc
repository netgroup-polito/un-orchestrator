#include <assign/std/vector.hpp>
#include "L3PortManager.h"

static const char LOG_MODULE_NAME[] = "L3Port-Manager";

//Constructor

L3PortManager::L3PortManager()
{
}

L3PortManager::L3PortManager(string L3Port):L3Port(L3Port)
{
}

//Destroyer
L3PortManager::~L3PortManager()
{
}

CreateNamespacePPPoE *L3PortManager::createNamespacePPPoE() {

    CreateNamespacePPPoE *cnPPPoE = new CreateNamespacePPPoE(NAMESPACE_HELPER, L3Port);

    stringstream cmd_create_namespace_pppoe;
    cmd_create_namespace_pppoe << getenv("un_script_path") << CREATE_NAMESPACE_PPPoE << " " << NAMESPACE_HELPER << " " << cnPPPoE->getPPPport();
    ULOG_DBG_INFO("Executing command \"%s\"", cmd_create_namespace_pppoe.str().c_str());

    int retVal = system(cmd_create_namespace_pppoe.str().c_str());
    retVal = retVal >> 8;
    if(retVal == 0) {
        ULOG_WARN("Failed to create or initialize namespace");
        throw L3PortManagerException();
    }

    return cnPPPoE;
}

void L3PortManager::setL3Port(string L3Port) {
    this->L3Port=L3Port;
}

string L3PortManager::getL3Port() {
    return L3Port;
}

void L3PortManager::addPortToNs(string port_name)
{
    stringstream cmd_add_port_to_ns;
    cmd_add_port_to_ns << getenv("un_script_path") << ADD_PORT_TO_NS << " " << NAMESPACE_HELPER << " " << port_name;
    ULOG_DBG_INFO("Executing command \"%s\"", cmd_add_port_to_ns.str().c_str());

    int retVal = system(cmd_add_port_to_ns.str().c_str());
    retVal = retVal >> 8;
    if(retVal == 0) {
        ULOG_WARN("Failed to add interface to the namespace");
        throw L3PortManagerException();
    }
}

