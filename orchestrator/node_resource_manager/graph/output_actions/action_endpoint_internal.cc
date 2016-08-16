#include "action_endpoint_internal.h"


ActionEndpointInternal::ActionEndpointInternal(string group, string endpointName) :
        OutputAction(ACTION_ON_ENDPOINT_INTERNAL),group(group), endpointName(endpointName)
{
}

bool ActionEndpointInternal::operator==(const ActionEndpointInternal &other) const
{
    if(/*(graphID == other.graphID) && */group == other.group)
        return true;

    return false;
}

string ActionEndpointInternal::getInfo()
{
    return group;
}

unsigned int ActionEndpointInternal::getGroup()
{
    unsigned int in_group;

    sscanf(group.c_str(), "%u", &in_group);

    return in_group;
}

string ActionEndpointInternal::getOutputEndpointID()
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
            case 1:
                str = string(pnt);
        }

        pnt = strtok( NULL, delimiter );
        i++;
    }

    return str;
}

string ActionEndpointInternal::toString()
{
    stringstream ss;
    ss << group;

    return ss.str();
}

Object ActionEndpointInternal::toJSON()
{
    Object action;
    action[OUTPUT] = endpointName.c_str();
    return action;
}
