#include "native.h"

std::map<std::string, Capability> *Native::capabilities;

Native::Native(){
	if(capabilities == NULL){
		capabilities = new std::map<std::string, Capability>;

		/*
			TODO:	IMPROVEMENTS:
				call a script that checks some available functions in the system (e.g. iptables) and fills the structure
		*/

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Reading capabilities from file %s", CAPABILITIES_FILE);

		std::set<std::string>::iterator it;
		xmlDocPtr schema_doc=NULL;
		xmlSchemaParserCtxtPtr parser_ctxt=NULL;
		xmlSchemaPtr schema=NULL;
		xmlSchemaValidCtxtPtr valid_ctxt=NULL;
		xmlDocPtr doc=NULL;

		//Validate the configuration file with the schema
		schema_doc = xmlReadFile(CAPABILITIES_XSD, NULL, XML_PARSE_NONET);
		if (schema_doc == NULL){
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "The schema cannot be loaded or is not well-formed.");
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			throw new NativeException();
		}

		parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc);
		if (parser_ctxt == NULL){
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to create a parser context for the schema \"%s\".",CAPABILITIES_XSD);
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			throw new NativeException();
		}

		schema = xmlSchemaParse(parser_ctxt);
		if (schema == NULL){
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "The XML \"%s\" schema is not valid.",CAPABILITIES_XSD);
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			throw new NativeException();
		}

		valid_ctxt = xmlSchemaNewValidCtxt(schema);
		if (valid_ctxt == NULL){
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to create a validation context for the XML schema \"%s\".",CAPABILITIES_XSD);
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			throw new NativeException();
		}

		doc = xmlParseFile(CAPABILITIES_FILE); /*Parse the XML file*/
		if (doc==NULL){
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "XML file '%s' parsing failed.", CAPABILITIES_FILE);
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			throw new NativeException();
		}

		if(xmlSchemaValidateDoc(valid_ctxt, doc) != 0){
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Configuration file '%s' is not valid", CAPABILITIES_FILE);
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			throw new NativeException();
		}

		xmlNodePtr root = xmlDocGetRootElement(doc);

		for(xmlNodePtr cur_root_child=root->xmlChildrenNode; cur_root_child!=NULL; cur_root_child=cur_root_child->next) {
			if ((cur_root_child->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(cur_root_child->name, (const xmlChar*)CAP_CAPABILITY_ELEMENT))){
				xmlChar* attr_name = xmlGetProp(cur_root_child, (const xmlChar*)CAP_NAME_ATTRIBUTE);
				assert(attr_name != NULL);
				xmlChar* attr_type = xmlGetProp(cur_root_child, (const xmlChar*)CAP_TYPE_ATTRIBUTE);
				assert(attr_type != NULL);
				xmlChar* attr_location = xmlGetProp(cur_root_child, (const xmlChar*)CAP_LOCATION_ATTRIBUTE);
				assert(attr_location != NULL);

				std::string name((const char*)attr_name);
				std::string path((const char*)attr_location);
				std::string typeString((const char*)attr_type);
				captype_t type = (typeString == TYPE_SCRIPT) ? EXECUTABLE : SCRIPT;

				capabilities->insert(std::pair<std::string, Capability>(name, Capability(name, path, type)));
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Read %s capability.", name.c_str());
			}
		}
		if(capabilities->empty()) {
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Native functions are not supported.");
		}
	}
}

bool Native::isSupported(Description& descr) {
	try{
		NativeDescription& nativeDescr = dynamic_cast<NativeDescription&>(descr);
		std::list<std::string> *requirements = nativeDescr.getRequirements();
		for(std::list<std::string>::iterator i = requirements->begin(); i != requirements->end(); i++){

			if(capabilities->find(*i) == capabilities->end()){
				//not found
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Capability %s required by native function is not supported", i->c_str());
				return false;
			}
		}
	} catch(exception& exc) {
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "exception %s", exc.what());
		return false;
	}
	return true;
}
bool Native::updateNF(UpdateNFIn uni)
{
	uint64_t lsiID = uni.getLsiID();
	std::string nf_name = uni.getNfId();
	map<unsigned int, string> namesOfPortsOnTheSwitch = uni.getNamesOfPortsOnTheSwitch();
	list<unsigned int> newPorts = uni.getNewPortsToAdd();
	unsigned int n_ports = newPorts.size();

	std::stringstream uri;

	try {
		NativeDescription& nativeDescr = dynamic_cast<NativeDescription&>(*description);
		if(nativeDescr.getLocation() == "local")
			uri << "file://";
	} catch (exception& e) {
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "exception %s", e.what());
		return false;
	}

	std::string uri_script = description->getURI();
	uri << uri_script;

	std::stringstream command;
	command << UPDATE_NATIVE_NF << " " << lsiID << " " << nf_name << " " << uri.str() << " " << n_ports;

	//create the names of the ports
	for(list<unsigned int>::iterator pn = newPorts.begin(); pn != newPorts.end(); pn++)
	{
		assert(namesOfPortsOnTheSwitch.find(*pn)!=namesOfPortsOnTheSwitch.end());
		command << " " << namesOfPortsOnTheSwitch[(*pn)];
	}

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

bool Native::startNF(StartNFIn sni) {

	uint64_t lsiID = sni.getLsiID();
	std::string nf_name = sni.getNfId();
	map<unsigned int, string> namesOfPortsOnTheSwitch = sni.getNamesOfPortsOnTheSwitch();
	unsigned int n_ports = namesOfPortsOnTheSwitch.size();

	std::stringstream uri;

	try {
		NativeDescription& nativeDescr = dynamic_cast<NativeDescription&>(*description);
		if(nativeDescr.getLocation() == "local")
			uri << "file://";
	} catch (exception& e) {
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "exception %s", e.what());
		return false;
	}

	std::string uri_script = description->getURI();
	uri << uri_script;

	std::stringstream command;
	command << PULL_AND_RUN_NATIVE_NF << " " << lsiID << " " << nf_name << " " << uri.str() << " " << n_ports;

	//create the names of the ports
	for(std::map<unsigned int, std::string>::iterator pn = namesOfPortsOnTheSwitch.begin(); pn != namesOfPortsOnTheSwitch.end(); pn++)
		command << " " << pn->second;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

bool Native::stopNF(StopNFIn sni) {

	uint64_t lsiID = sni.getLsiID();
	std::string nf_name = sni.getNfId();

	std::stringstream command;
	command << STOP_NATIVE_NF << " " << lsiID << " " << nf_name;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());
	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

unsigned int Native::convertNetmask(string netmask) {

	unsigned int slash = 0;
	unsigned int mask;

	int first, second, third, fourth;
	sscanf(netmask.c_str(),"%d.%d.%d.%d",&first,&second,&third,&fourth);
	mask = (first << 24) + (second << 16) + (third << 8) + fourth;

	for(int i = 0; i < 32; i++)
	{
		if((mask & 0x1) == 1)
			slash++;
		mask = mask >> 1;
	}

	return slash;
}

void Native::freeXMLResources(xmlSchemaParserCtxtPtr parser_ctxt, xmlSchemaValidCtxtPtr valid_ctxt, xmlDocPtr schema_doc, xmlSchemaPtr schema, xmlDocPtr doc)
{
	if(valid_ctxt!=NULL)
		xmlSchemaFreeValidCtxt(valid_ctxt);

	if(schema!=NULL)
		xmlSchemaFree(schema);

	if(parser_ctxt!=NULL)
	    xmlSchemaFreeParserCtxt(parser_ctxt);

	if(schema_doc!=NULL)
		xmlFreeDoc(schema_doc);

	if(doc!=NULL)
		xmlFreeDoc(doc);

	xmlCleanupParser();
}

