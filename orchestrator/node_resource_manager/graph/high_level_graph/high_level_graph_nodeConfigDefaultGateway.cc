#include "high_level_graph_nodeConfigDefaultGateway.h"

static const char LOG_MODULE_NAME[] = "High-Level-Graph-NodeConfigDefaultGateway";

namespace highlevel
{
NodeConfigDefaultGateway::NodeConfigDefaultGateway(){

}
NodeConfigDefaultGateway::NodeConfigDefaultGateway(string id, string defaultGateway) : id(id), defaultGateway(defaultGateway)
{
}

NodeConfigDefaultGateway::~NodeConfigDefaultGateway()
{
}

bool NodeConfigDefaultGateway::operator==(const NodeConfigDefaultGateway &other) const
{
    if(id == other.id && defaultGateway == other.defaultGateway)
        return true;

    return false;
}

string NodeConfigDefaultGateway::getId() {
    return id;
}

string NodeConfigDefaultGateway::getDefaultGateway() {
    return defaultGateway;
}

Object NodeConfigDefaultGateway::toJSON()
{
    Object NodeConfigDefaultGateway, nodeConfigDG;

    NodeConfigDefaultGateway[_ID] = id.c_str();
    NodeConfigDefaultGateway[NC_TYPE] = NC_DG;

    if(defaultGateway!="")
        nodeConfigDG[NC_DG_IP] = defaultGateway.c_str();

    NodeConfigDefaultGateway[NC_DG] = nodeConfigDG;
    return NodeConfigDefaultGateway;
}
}