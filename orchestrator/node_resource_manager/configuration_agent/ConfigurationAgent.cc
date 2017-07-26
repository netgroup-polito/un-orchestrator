#include "ConfigurationAgent.h"
#include "constants.h"

static const char LOG_MODULE_NAME[] = "ConfigurationAgent";

ConfigurationAgent::ConfigurationAgent()
{

}

ConfigurationAgent::~ConfigurationAgent()
{

}

void ConfigurationAgent::on_configurationEvent(char *configuration){
    ULOG_DBG_INFO("Received: '%s'", configuration);
    highlevel::Graph *graph = new highlevel::Graph("");
    Value value;
    read(configuration, value);
    NodeConfigurationParser::nodeConfigurationParser(value, *graph);
    if(!setNodeConfiguration(graph)){
        ULOG_ERR("An error occurred setting the node configuration");
        throw ConfigurationAgentException();
    }
    //FIXME: the setting received from the DD is not added to the graph
}

bool ConfigurationAgent::setDefaultGateway(string ipAddress)
{
    stringstream cmd_set_default_gateway;
    cmd_set_default_gateway << SET_DEFAULT_GATEWAY << " " << ipAddress;
    ULOG_DBG_INFO("Executing command \"%s\"", cmd_set_default_gateway.str().c_str());
    int retVal = system(cmd_set_default_gateway.str().c_str());

    retVal = retVal >> 8;
    if(retVal != 0){
        ULOG_WARN("Failed to set the default gateway");
        return false;
    }
    return true;
}

bool ConfigurationAgent::setNodeConfiguration(highlevel::Graph *graph)
{
    //set the default gateway of the node
    highlevel::NodeConfigDefaultGateway ncDG = graph->getNodeConfigDefaultGateway();
    if(ncDG.getId()!="")
    {
        string ipAddress = ncDG.getDefaultGateway();
        if (ipAddress!="")
        {
            bool set = this->setDefaultGateway(ipAddress);
            if (!set)
                return false;
        }
    }
    return true;
}