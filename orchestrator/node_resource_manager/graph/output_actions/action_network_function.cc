#include "action_network_function.h"


TempActionNetworkFunction::TempActionNetworkFunction(string nfId, string endpointPortName, unsigned int port) :
        OutputAction(TEMP_ACTION_ON_NETWORK_FUNCTION),nfId(nfId), endpointPortName(endpointPortName), port(port)
{

}

bool TempActionNetworkFunction::operator==(const TempActionNetworkFunction &other) const
{
    if((nfId == other.nfId) && (port == other.port) && (endpointPortName == other.endpointPortName))
        return true;

    return false;
}

string TempActionNetworkFunction::getInfo()
{
    return nfId;
}

unsigned int TempActionNetworkFunction::getPort()
{
    return port;
}

string TempActionNetworkFunction::toString()
{
    stringstream ss;
    ss << nfId << ":" << port;

    return ss.str();
}

Object TempActionNetworkFunction::toJSON()
{
    Object action;
    stringstream network_function;
    network_function << nfId << ":" << port;
    action[OUTPUT] = endpointPortName.c_str();
    return action;
}
