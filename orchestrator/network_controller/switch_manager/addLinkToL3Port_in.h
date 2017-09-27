#ifndef PROJECT_ADDLINKTOL3PORTIN_H
#define PROJECT_ADDLINKTOL3PORTIN_H
#include <string>
#include <list>
#include <inttypes.h>

/**
* @file addLinkToL3Port_in.h
*
* @brief Description of link that must be connected to the namespace fro the PPPoE connection.
*/

using namespace std;

class AddLinkToL3PortIn
{

friend class GraphManager;

private:

    /**
    *	@brief: identifier of the lsi to which the endpoint must be connected
    */
    uint64_t dpi0 = 1;

    /**
    *	@brief: name of the L3 port
    */
    //string L3PortName;

    /**
    *	@brief: Id of the graph that will use this link
    */
    string graphID;

public:

    AddLinkToL3PortIn(/*string L3PortName,*/ string graphID)
    : /*L3PortName(L3PortName),*/ graphID(graphID)
    {
    }

    uint64_t getDpid0()
    {
        return dpi0;
    }

    /*string getL3PortName()
    {
        return L3PortName;
    }*/

    string getGraphID()
    {
        return graphID;
    }
};

#endif //PROJECT_ADDLINKTOL3PORTIN_H
