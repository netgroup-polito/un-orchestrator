#include "template_parser.h"
#include "../description.h"

static const char LOG_MODULE_NAME[] = "Template-Parser";

bool Template_Parser::parse(Template &temp, string answer) {
    try
    {
        Value json;
        read(answer, json);
        Object obj = json.getObject();
        bool foundPorts = true;
        bool foundName = false;
        bool foundImplementations = false;
        bool foundURI = false;
        bool foundCores = false;
        bool validPortType = false;
        bool foundTypeURI = false;
        for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
        {
            const string& name  = i->first;
            const Value&  value = i->second;

            if(name == "name")
            {
                foundName = true;
                temp.setName(value.getString()) ;
            }
            else if(name == "expandable")
            {
                temp.setExpandable(value.getBool());

            }
            else if(name == "uri")
            {
                temp.setURI(value.getString());
                foundURI = true;
            }
            else if(name == "vnf-type")
            {
                if(!NFType::isValid(value.getString()))
                {
                    ULOG_DBG_INFO("Invalid implementation type \"%s\". Skip it.", value.getString().c_str());
                    break;
                }
                temp.setVnfType(value.getString());
                foundImplementations = true;
            }
            else if(name == "type"){
                temp.setURIType(value.getString());
                foundTypeURI = true;
            }
            else if(name == "CPUrequirements")
            {
                foundCores = Template_Parser::parseCoreNumbers(temp,value.getObject());
            }
            else if(name == "ports"){
                foundPorts = true;
                const Array& ports_array = value.getArray();
                if (ports_array.size() == 0)
                {
                    ULOG_WARN("Empty ports list in implementation");
                    return false;
                }
                for( unsigned int i = 0; i < ports_array.size(); ++i)
                {
                    Object port = ports_array[i].getObject();
                    validPortType=parsePort(temp,port);
                }
            }
        }//end iteration on the answer

        if(!foundName || !foundImplementations || !foundURI || !foundPorts || !validPortType || !foundTypeURI)
        {
            ULOG_WARN("Key \"name\", and/or key \"vnf-type\", and/or key \"uri\" and/or key \"typeURI\" and/or key \"ports\" and/or valid ports has been found in the answer ");
            return false;
        }
        if(!foundCores && temp.getName() == "dpdk"){
            ULOG_WARN("Core numbers have not been found in the template for implementation dpdk");
            return false;
        }

    }
    catch (std::runtime_error& e) {
        ULOG_WARN("JSON parse error: %s", e.what());
        return false;
    }
    catch(...)
    {
        ULOG_WARN("The content does not respect the JSON syntax");
        return false;
    }

    return true;

}

bool Template_Parser::parsePort(Template& temp, Object obj) {
        PortType port_type = UNDEFINED_PORT;
        Port port;
        for( Object::const_iterator port_el = obj.begin(); port_el != obj.end(); ++port_el ) {
            const string &pel_name = port_el->first;
            const Value &pel_value = port_el->second;
            if (pel_name == "position") {
                port.setPortsRange(pel_value.getString());
            }
            else if (pel_name == "technology") {
                port_type = portTypeFromString(pel_value.getString());
                if (port_type == INVALID_PORT) {
                    ULOG_WARN("Invalid port type \"%s\" for implementation port", pel_value.getString().c_str());
                    return false;
                }
                port.setTechnology(pel_value.getString());
            }

        }
        /*if (port_type == UNDEFINED_PORT) {
            ULOG_WARN("Missing port \"type\" attribute for implementation");
            return false;
        }*/
        ULOG_DBG_INFO(" Port  range=%s type=%s",  port.getPortsRange().c_str(),port.getTechnology().c_str());

        temp.addPort(port);
        return true;

}

bool Template_Parser::parseCoreNumbers(Template& temp, Object CPUrequirements) {
    for( Object::const_iterator iterator_CPUrequirements = CPUrequirements.begin(); iterator_CPUrequirements != CPUrequirements.end(); ++iterator_CPUrequirements )
    {
        const string& name  = iterator_CPUrequirements->first;
        const Value&  value = iterator_CPUrequirements->second;
        if(name == "socket")
        {
            const Array& sockets = value.getArray();
            for( unsigned int i = 0; i < sockets.size(); ++i)
            {
                Object socket = sockets[i].getObject();
                for( Object::const_iterator iterator_socket = socket.begin(); iterator_socket != socket.end(); ++iterator_socket ) //per eventuali espansioni template
                {
                    const string& socket_elementName  = iterator_socket->first;
                    const Value&  socket_value = iterator_socket->second;
                    if(socket_elementName== "coreNumbers"){
                        temp.setCores(socket_value.getInt());
                        ULOG_DBG_INFO("cores : %d", socket_value.getInt());
                        return true;
                    }
                }
            }
        }
    }
    return false;
}