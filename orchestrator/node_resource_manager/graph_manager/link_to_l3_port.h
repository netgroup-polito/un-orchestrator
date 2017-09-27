#ifndef PROJECT_LINK_TO_L3_PORT_H
#define PROJECT_LINK_TO_L3_PORT_H

#pragma once

#include <inttypes.h>
#include <string>

using namespace std;

class LinkToL3Port
{
friend class LSI;

private:
    //FIXME: protect it with a mutex?
    static uint64_t nextID;

    uint64_t ID;

protected:
    /*the id of the LSI to which the link must be connected (LSI-0)*/
    uint64_t dpid = 0;
    /*name of the peer connected to the LSI*/
    string lsi_port_name;
    /*openFlow id of the peer connected to the LSI*/
    unsigned int lsi_port_id;
    /*name of the peer connected to the namespace*/
    string ns_port_name;

public:
    LinkToL3Port(string lsi_port_name, unsigned int lsi_port_id, string ns_port_name);
    unsigned int getLSIPortID();
    string getLSIPortName();
    string getNsPortName();
    uint64_t getID();
};


#endif //PROJECT_LINK_TO_L3_PORT_H
