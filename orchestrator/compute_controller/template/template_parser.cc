#include "template_parser.h"
#include "../description.h"

static const char LOG_MODULE_NAME[] = "Template-Parser";

bool Template_Parser::parse(std::list<NFtemplate*>& templates, string answer,bool checkSingleTemplate) {
	ULOG_DBG_INFO("Starting to parse the NF template");
	try
	{
		Value json;
		Object obj;
		read(answer, json);
		obj = json.getObject();
		//single template
		if(checkSingleTemplate) {
			NFtemplate* temp = new NFtemplate();
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
						NFtemplate* temp = new NFtemplate();
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
	catch (const std::exception& e) {
		ULOG_WARN("JSON parse error: %s", e.what());
		return false;
	}

	ULOG_DBG_INFO("NF template parsed correctly");
	return true;
}

void Template_Parser::setTemplateFromJson(NFtemplate *temp,Object obj)
{
	//The following parameters are mandatory in the template
	bool foundPorts = false;
	bool foundCapability = false;
	bool foundVnfType = false;
	bool foundURI = false;
	bool validPortTechnology = false;
	bool foundTypeURI = false;

	ULOG_DBG("Starting iteration on elements of the template");
	for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
	{
		const string& name  = i->first;
		const Value&  value = i->second;

		if(name == "name")
		{
			//Optional field
			ULOG_DBG("Parsing 'name'");
			temp->setName(value.getString()) ;
		}
		if(name == "functional-capability")
		{
			ULOG_DBG("Parsing 'functional-capability'");
			foundCapability = true;
			temp->setCapability(value.getString()) ;
		}
		else if(name == "expandable")
		{
			//Optional field
			ULOG_DBG("Parsing 'expandable'");
			temp->setExpandable(value.getBool());
		}
		else if(name == "uri")
		{
			ULOG_DBG("Parsing 'uri'");
			temp->setURI(value.getString());
			foundURI = true;
		}
		else if(name == "vnf-type")
		{
			ULOG_DBG("Parsing 'vnf-type'");
			if(stringToVnfType(value.getString()) != UNDEFINED){
				temp->setVnfType(stringToVnfType(value.getString()));
				foundVnfType = true;
			}
			else{
				ULOG_ERR("Invalid vnf type \"%s\"", value.getString().c_str());
				throw new std::string("Invalid vnf type \"%s\"", value.getString().c_str());
			}

		}
		else if(name == "uri-type"){
			ULOG_DBG("Parsing 'uri-type'");
			if(stringToUriType(value.getString()) != UNDEFINED_URITYPE){
				temp->setURIType(stringToUriType(value.getString()));
				foundTypeURI = true;
			}
			else{
				ULOG_ERR("Invalid uri type \"%s\"", value.getString().c_str());
				throw new std::string("Invalid uri type \"%s\"", value.getString().c_str());
			}

		}
		else if(name == "CPUrequirements")
		{
			//Optional field
			ULOG_DBG("Parsing 'CPUrequirements'");
			if(!Template_Parser::parseCPUrequirements(temp,value.getObject()))
			{
				ULOG_WARN("Wrong element 'CPUrequirements'");
				throw new std::string("Wrong element 'CPUrequirements'");
			}
		}
		else if(name == "ports"){
			ULOG_DBG("Parsing 'ports'");
			foundPorts = true;
			const Array& ports_array = value.getArray();
			if (ports_array.size() == 0)
			{
				throw new std::string("Empty ports list in implementation");
			}
			for( unsigned int i = 0; i < ports_array.size(); ++i)
			{
				Object port = ports_array[i].getObject();
				validPortTechnology=parsePort(temp,port);
			}
		}
	}//end iteration on the answer

	ULOG_DBG("Iteration on elements of the template terminated");

	//Check that those fields manatory for each NF implementation are set
	if(!foundCapability || !foundVnfType || !foundURI || !foundPorts || !validPortTechnology || !foundTypeURI)
	{
		ULOG_WARN("Key \"functional-capability\", and/or key \"vnf-type\", and/or key \"uri\" and/or key \"uri-type\" and/or key \"ports\" and/or valid ports has not been found in the answer ");
		throw new std::string("Key \"functional-capability\", and/or key \"vnf-type\", and/or key \"uri\" and/or key \"uri-type\" and/or key \"ports\" and/or valid ports has not been found in the answer ");
	}
}

bool Template_Parser::parsePort(NFtemplate* temp, Object obj) {
	PortTechnology port_technology = UNDEFINED_PORT;
	Port port;
	for( Object::const_iterator port_el = obj.begin(); port_el != obj.end(); ++port_el ) {
		const string &pel_name = port_el->first;
		const Value &pel_value = port_el->second;
		if (pel_name == "position") { //FIXME-ENNIO: if the template specifies an unbounded number of ports, the UN crashes when trying to deploy the network function
			ULOG_DBG("Parsing 'position'");
			port.setPortsRange(pel_value.getString());
		}
		else if (pel_name == "technology") {
			ULOG_DBG("Parsing 'technology'");
			port_technology = portTechnologyFromString(pel_value.getString());
			if (port_technology == INVALID_PORT) {
				ULOG_WARN("Invalid port type \"%s\" for implementation port", pel_value.getString().c_str());
				return false;
			}
		}
	}
	port.setTechnology(port_technology);
	temp->addPort(port);
	return true;

}

bool Template_Parser::parseCPUrequirements(NFtemplate* temp, Object CPUrequirements) {
	for( Object::const_iterator iterator_CPUrequirements = CPUrequirements.begin(); iterator_CPUrequirements != CPUrequirements.end(); ++iterator_CPUrequirements )
	{
		const string& name  = iterator_CPUrequirements->first;
		const Value&  value = iterator_CPUrequirements->second;
		if(name == "socket")
		{
			ULOG_DBG("Parsing 'socket'");
			const Array& sockets = value.getArray();
			if(sockets.size() != 1)
			{
				ULOG_ERR("Too much elements inside the element 'socket'.");
				return  false; 
			}
			for( unsigned int i = 0; i < sockets.size(); ++i)
			{
				Object socket = sockets[i].getObject();
				for( Object::const_iterator iterator_socket = socket.begin(); iterator_socket != socket.end(); ++iterator_socket )
				{
					const string& socket_elementName  = iterator_socket->first;
					const Value&  socket_value = iterator_socket->second;
					if(socket_elementName== "coreNumbers"){
						temp->setCores(socket_value.getInt());
						break;
					}
				}
			}
		}
		else if(name == "platformType")
		{
			ULOG_DBG("Parsing 'platformType'");
			temp->setPlatform(value.getString());
		}
	}
	return true;
}

