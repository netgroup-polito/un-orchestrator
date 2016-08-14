#include "action_port.h"


ActionPort::ActionPort(string portName, string endpointName) :
        OutputAction(ACTION_ON_PORT),portName(portName), endpointName(endpointName)
{

}

ActionPort::~ActionPort()
{

}

bool ActionPort::operator==(const ActionPort &other) const
{
    if(portName == other.portName && endpointName == other.endpointName)
        return true;

    return false;
}

string ActionPort::getInfo()
{
    return portName;
}

string ActionPort::toString()
{
    return portName;
}

Object ActionPort::toJSON()
{
    Object action;
    action[OUTPUT] = endpointName.c_str();
    return action;
}