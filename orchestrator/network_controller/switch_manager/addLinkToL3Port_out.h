#ifndef PROJECT_ADDLINKTOL3PORT_H
#define PROJECT_ADDLINKTOL3PORT_H

#pragma once

#include <string>
#include <list>

/**
* @file addLinkToL3Port_out.h
*
* @brief Description of endpoint.
*/

using namespace std;

class AddLinkToL3PortOut
{
private:

    /**
    *	@brief: id of the endpoint connected to the lsi
    */
    string ep_id_lsi;

    /**
    *	@brief: openflow id of the connected endpoint
    */
    unsigned int of_id;

    /**
    *	@brief: id of the endpoint that must be connected to the namespace
    */
    string ep_id_ns;

public:
    string getEpIdLsi()
    {
        return ep_id_lsi;
    }

    unsigned int getOfId()
    {
        return of_id;
    }

    string getEpIdNs()
    {
        return ep_id_ns;
    }

    AddLinkToL3PortOut(string ep_id_lsi, unsigned int of_id, string ep_id_ns)
    : ep_id_lsi(ep_id_lsi), of_id(of_id), ep_id_ns(ep_id_ns)
    {
    }

};

#endif //PROJECT_ADDLINKTOL3PORT_H
