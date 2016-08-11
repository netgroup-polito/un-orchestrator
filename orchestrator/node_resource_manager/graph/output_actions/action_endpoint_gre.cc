#include "action_endpoint_gre.h"


TempActionEndPointGre::TempActionEndPointGre(string endpointID, string endpointName) :
        endpointID(endpointID), endpointName(endpointName)
{
}

bool TempActionEndPointGre::operator==(const TempActionEndPointGre &other) const
{
    if((endpointID == other.endpointID))
        return true;

    return false;
}

string TempActionEndPointGre::getInfo()
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

string TempActionEndPointGre::getOutputEndpointID()
{
    return endpointID;
}

string TempActionEndPointGre::toString()
{
    return endpointID;
}

Object TempActionEndPointGre::toJSON()
{
    Object action;
    action[OUTPUT] = endpointName.c_str();

    return action;
}
