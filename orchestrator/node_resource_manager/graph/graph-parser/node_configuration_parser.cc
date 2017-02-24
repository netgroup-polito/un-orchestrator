#include "node_configuration_parser.h"

static const char LOG_MODULE_NAME[] = "Graph-Parser";

void NodeConfigurationParser::nodeConfigurationParser(Value value, highlevel::Graph &graph) {
    string nodeConfigDG_id;
    try{
        Object obj=value.getObject();

        //Iterates on the json received
        for (Object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
            bool nc_dg = false;
            string id, dgIP;
            const string &name = i->first;
            const Value &value = i->second;

            //Identify the node configuration
            if(name==NODE_CONFIG)
            {
                const Array& node_configurations_array = value.getArray();

                ULOG_DBG("\"%s\"",NODE_CONFIG);

                //Iterate on the node-configurations
                set<string> usedNodeConfigurationsID;
                for( unsigned int nc = 0; nc < node_configurations_array.size(); ++nc )
                {
                    try{
                        node_configurations_array[nc].getObject();
                    } catch(std::exception& e)
                    {
                        string error = string("The content does not respect the JSON syntax: element of ") + NODE_CONFIG + " should be an Object";
                        ULOG_WARN(error.c_str());
                        throw NodeConfigurationParserException(std::move(error));
                    }

                    //This is a node configuration, with a type, and a default gateway
                    Object node_configuration = node_configurations_array[nc].getObject();
                    //Iterate on the elements of a node configuration
                    for(Object::const_iterator nodeConfig = node_configuration.begin(); nodeConfig != node_configuration.end(); nodeConfig++)
                    {
                        const string& nc_name  = nodeConfig->first;
                        const Value&  nc_value = nodeConfig->second;

                        if(nc_name == _ID)
                        {
                            id = nc_value.getString();
                            //Two node configurations cannot have the same ID
                            if(usedNodeConfigurationsID.count(id) != 0)
                            {
                                string error = string("Found two node configurations with the same ID: ") + id.c_str() + ". This is not valid.";
                                ULOG_WARN(error.c_str());
                                throw NodeConfigurationParserException(std::move(error));
                            }
                            usedNodeConfigurationsID.insert(id);
                        }
                        else if(nc_name == NC_TYPE)
                        {
                            ULOG_DBG("\"%s\"->\"%s\": \"%s\"",NODE_CONFIG,NC_TYPE,nc_value.getString().c_str());
                            string type = nc_value.getString();
                        }
                            //identify default gateway node configuration
                        else if(nc_name == NC_DG)
                        {
                            try
                            {
                                nc_value.getObject();
                            } catch(std::exception& e)
                            {
                                string error = string("The content does not respect the JSON syntax: ") + NC_DG + " should be an Object";
                                ULOG_WARN(error.c_str());
                                throw NodeConfigurationParserException(std::move(error));
                            }

                            Object nc_defaultGateway = nc_value.getObject();
                            if(!nc_dg) {
                                nc_dg = true;

                                for (Object::const_iterator ncDG = nc_defaultGateway.begin(); ncDG != nc_defaultGateway.end(); ncDG++) {
                                    const string &ncDG_name = ncDG->first;
                                    const Value &ncDG_value = ncDG->second;

                                    if (ncDG_name == NC_DG_IP)
                                    {
                                        ULOG_DBG("\"%s\"->\"%s\": \"%s\"", NC_DG, NC_DG_IP,
                                                 ncDG_value.getString().c_str());
                                        dgIP = ncDG_value.getString();
                                        nodeConfigDG_id = ncDG_value.getString();
                                        ULOG_DBG("\"%s\"->\"%s\"", id.c_str(), nodeConfigDG_id.c_str());
                                    } else {
                                        string error =string("Invalid key ") + ncDG_name.c_str() + " inside " + NC_DG_IP;
                                        ULOG_WARN(error.c_str());
                                        throw NodeConfigurationParserException(std::move(error));
                                    }
                                }
                            } else
                            {
                                string error = string("More than one ") + nc_name.c_str() + " key present in an Object of " + NODE_CONFIG;
                                ULOG_WARN(error.c_str());
                                throw NodeConfigurationParserException(std::move(error));                            }
                        }
                        else
                        {
                            string error = string("Invalid key ") + nc_name.c_str() + " in an Object of " + NODE_CONFIG;
                            ULOG_WARN(error.c_str());
                            throw NodeConfigurationParserException(std::move(error));
                        }
                    }//End of iteration on the elements of a node configuration

                    //add default gateway node configuration
                    if(nc_dg)
                    {
                        highlevel::NodeConfigDefaultGateway nodeConfigDG(id, dgIP);
                        graph.addNodeConfigDefaultGateway(nodeConfigDG);
                    }
                }
            } else
            {
                string error = string("Invalid key ") + name.c_str() + " inside ROOT";
                ULOG_WARN(error.c_str());
                throw NodeConfigurationParserException(std::move(error));
            }
        }
    }catch(std::exception& e)
    {
        string error = string("The content does not respect the JSON syntax: ") + e.what();
        ULOG_WARN(error.c_str());
        throw NodeConfigurationParserException(std::move(error));
    }
}