#include "action_endpoint_internal.h"


TempActionEndPointInternal::TempActionEndPointInternal(string group, string endpointName) :
        group(group), endpointName(endpointName)
{
}

bool TempActionEndPointInternal::operator==(const TempActionEndPointInternal &other) const
{
    if(/*(graphID == other.graphID) && */group == other.group)
        return true;

    return false;
}

string TempActionEndPointInternal::getInfo()
{
    return group;
}

unsigned int TempActionEndPointInternal::getGroup()
{
    unsigned int in_group;

    sscanf(group.c_str(), "%u", &in_group);

    return in_group;
}

string TempActionEndPointInternal::getOutputEndpointID()
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

string TempActionEndPointInternal::toString()
{
    stringstream ss;
    ss << group;

    return ss.str();
}

Object TempActionEndPointInternal::toJSON()
{
    Object action;
    action[OUTPUT] = endpointName.c_str();
    return action;
}
