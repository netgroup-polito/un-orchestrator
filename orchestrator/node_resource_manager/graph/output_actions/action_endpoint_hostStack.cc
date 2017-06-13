#include "action_endpoint_hostStack.h"

ActionEndPointHostStack::ActionEndPointHostStack(string endpointID, string endpointName) :
	OutputAction(ACTION_ON_ENDPOINT_HOSTSTACK), endpointID(endpointID), endpointName(endpointName)
{
}

bool ActionEndPointHostStack::operator==(const ActionEndPointHostStack &other) const
{
	if((endpointID == other.endpointID))
		return true;

	return false;
}

string ActionEndPointHostStack::getInfo()
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

string ActionEndPointHostStack::getOutputEndpointID()
{
	return endpointID;
}

string ActionEndPointHostStack::toString()
{
	return endpointID;
}

Object ActionEndPointHostStack::toJSON()
{
	Object action;
	action[OUTPUTX] = endpointName.c_str();
	return action;
}
