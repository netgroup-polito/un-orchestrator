#include "high_level_output_action_endpoint_management.h"

namespace highlevel
{

ActionEndPointManagement::ActionEndPointManagement(string endpointID, string endpointName) :
	Action(ACTION_ON_ENDPOINT_MANAGEMENT), endpointID(endpointID), endpointName(endpointName)
{
}

bool ActionEndPointManagement::operator==(const ActionEndPointManagement &other) const
{
	if((endpointID == other.endpointID))
		return true;

	return false;
}

string ActionEndPointManagement::getInfo()
{
	//Check the name of port
	char delimiter[] = ":";
	char * pnt;

	string str;

	char tmp[BUFFER_SIZE];
	strcpy(tmp,(char *)endpointName.c_str());
	pnt=strtok(tmp, delimiter);
	int i = 0;

	while( pnt!= NULL )
	{
		switch(i)
		{
			case 0:
				str = string(pnt);
				break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return str;
}

string ActionEndPointManagement::getOutputEndpointID()
{
	return endpointID;
}

string ActionEndPointManagement::toString()
{
	return endpointID;
}

Object ActionEndPointManagement::toJSON()
{
	Object action;
	action[OUTPUT] = endpointName.c_str();

	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
		(*ga)->toJSON(action);

	return action;
}

}
