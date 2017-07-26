#include "action_endpoint_gre.h"


ActionEndpointGre::ActionEndpointGre(string endpointID, string endpointName) :
        OutputAction(ACTION_ON_ENDPOINT_GRE),endpointID(endpointID), endpointName(endpointName)
{
}

bool ActionEndpointGre::operator==(const ActionEndpointGre &other) const
{
    if((endpointID == other.endpointID))
        return true;

    return false;
}

string ActionEndpointGre::getInfo()
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

string ActionEndpointGre::getOutputEndpointID()
{
    return endpointID;
}

string ActionEndpointGre::toString()
{
    return endpointID;
}

Object ActionEndpointGre::toJSON()
{
    Object action;
    action[OUTPUTX] = endpointName.c_str();

    return action;
}
