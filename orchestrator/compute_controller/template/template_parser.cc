#include "template_parser.h"
#include "../description.h"

static const char LOG_MODULE_NAME[] = "Template-Parser";

bool Template_Parser::parse(std::list<NFtemplate>& templates, string answer,bool checkSingleTemplate) {
	try
	{
		Value json;
		Object obj;
		read(answer, json);
		obj = json.getObject();
		//single template
		if(checkSingleTemplate) {
			NFtemplate temp;
			setTemplateFromJson(temp,obj);
			templates.push_back(temp);
		}
		//multiple templates
		else { //here we have to remove all that we don't need from the answer and we have to take the info about the templates
			for( Object::const_iterator rootElement = obj.begin(); rootElement != obj.end(); ++rootElement ) { //loop just once, take the object "list", info about template is inside
				const Value & arrayValue = rootElement->second;
				const Array& descriptions  = arrayValue.getArray();
				for( unsigned int i = 0; i < descriptions.size(); ++i)
				{
					Object description = descriptions[i].getObject();
					for( Object::const_iterator element = description.begin(); element != description.end(); ++element ) {
						NFtemplate temp;
						const string &name = element->first;
						const Value & value = element->second;
						if(name == "template"){  //it can be also id , but we re not interested
							setTemplateFromJson(temp,value.getObject());
							templates.push_back(temp);
						}
					}
				}
			}
		}
	}
	catch (std::runtime_error& e) {
		ULOG_WARN("JSON parse error: %s", e.what());
		return false;
	}
	return true;
}

void Template_Parser::setTemplateFromJson(NFtemplate &temp,Object obj){

	bool foundPorts = false;
	bool foundCapability = false;
	bool foundImplementations = false;
	bool foundURI = false;
	bool foundCores = false;
	bool validPortType = false;
	bool foundTypeURI = false;
	for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;

		if(name == "functional-capability")
		{
			foundCapability = true;
			temp.setCapability(value.getString()) ;
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
			temp.setVnfType(value.getString());
			foundImplementations = true;
		}
		else if(name == "uri-type"){
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
				throw new std::string("Empty ports list in implementation");
			}
			for( unsigned int i = 0; i < ports_array.size(); ++i)
			{
				Object port = ports_array[i].getObject();
				validPortType=parsePort(temp,port);
			}
		}
	}//end iteration on the answer

	if(!foundCapability || !foundImplementations || !foundURI || !foundPorts || !validPortType || !foundTypeURI)
	{
		throw new std::string("Key \"functional-capability\", and/or key \"vnf-type\", and/or key \"uri\" and/or key \"uri-type\" and/or key \"ports\" and/or valid ports has not been found in the answer ");
	}
	if(!foundCores && temp.getVnfType() == "dpdk"){
		throw new std::string("Core numbers have not been found in the template for implementation dpdk");
	}
}

bool Template_Parser::parsePort(NFtemplate& temp, Object obj) {
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
		temp.addPort(port);
		return true;

}

bool Template_Parser::parseCoreNumbers(NFtemplate& temp, Object CPUrequirements) {
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
						return true;
					}
				}
			}
		}
	}
	return false;
}
