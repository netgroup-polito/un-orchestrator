#include "high_level_output_action_nf.h"

namespace highlevel
{

ActionNetworkFunction::ActionNetworkFunction(string nfId, string endpointPortName, unsigned int port) :
	Action(ACTION_ON_NETWORK_FUNCTION), nfId(nfId), endpointPortName(endpointPortName), port(port)
{

}

bool ActionNetworkFunction::operator==(const ActionNetworkFunction &other) const
{
	if((nfId == other.nfId) && (port == other.port) && (endpointPortName == other.endpointPortName))
		return true;

	return false;
}

string ActionNetworkFunction::getInfo()
{
	return nfId;
}

unsigned int ActionNetworkFunction::getPort()
{
	return port;
}

string ActionNetworkFunction::toString()
{
	stringstream ss;
	ss << nfId << ":" << port;

	return ss.str();
}

Object ActionNetworkFunction::toJSON()
{
	Object action;
	stringstream network_function;
	network_function << nfId << ":" << port;
	action[OUTPUT] = endpointPortName.c_str();

	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		(*ga)->toJSON(action);

	return action;
}

}
