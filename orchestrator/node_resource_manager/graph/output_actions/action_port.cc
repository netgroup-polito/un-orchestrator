#include "action_port.h"


TempActionPort::TempActionPort(string portName, string endpointName) :
        OutputAction(TEMP_ACTION_ON_PORT),portName(portName), endpointName(endpointName)
{

}

TempActionPort::~TempActionPort()
{

}

bool TempActionPort::operator==(const TempActionPort &other) const
{
    if(portName == other.portName && endpointName == other.endpointName)
        return true;

    return false;
}

string TempActionPort::getInfo()
{
    return portName;
}

string TempActionPort::toString()
{
    return portName;
}

Object TempActionPort::toJSON()
{
    Object action;
    action[OUTPUT] = endpointName.c_str();
    return action;
}