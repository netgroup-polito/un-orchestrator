#include "link_to_l3_port.h"

uint64_t LinkToL3Port::nextID = 0;

LinkToL3Port::LinkToL3Port(string lsi_port_name, unsigned int lsi_port_id, string ns_port_name) :
        lsi_port_name(lsi_port_name), lsi_port_id(lsi_port_id), ns_port_name(ns_port_name)
{
    ID = nextID;
    nextID++;
}

unsigned int LinkToL3Port::getLSIPortID()
{
    return lsi_port_id;
}

string LinkToL3Port::getLSIPortName()
{
    return lsi_port_name;
}

string LinkToL3Port::getNsPortName()
{
    return ns_port_name;
}

uint64_t LinkToL3Port::getID()
{
    return ID;
}