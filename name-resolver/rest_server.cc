#include "rest_server.h"

set<NF*> RestServer::nfs;

bool RestServer::init(string fileName)
{
	set<std::string>::iterator it;
	xmlDocPtr schema_doc=NULL;
 	xmlSchemaParserCtxtPtr parser_ctxt=NULL;
	xmlSchemaPtr schema=NULL;
	xmlSchemaValidCtxtPtr valid_ctxt=NULL;
	xmlDocPtr doc=NULL;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Reading configuration file: %s",fileName.c_str());

	try {
		//Validate the configuration file with the schema
		schema_doc = xmlReadFile(NETWORK_FUNCTIONS_XSD, NULL, XML_PARSE_NONET);
		if (schema_doc == NULL)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "The schema cannot be loaded or is not well-formed.");
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			return false;
		}

		parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc);
		if (parser_ctxt == NULL)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to create a parser context for the schema.");
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			return false;
		}
	
		schema = xmlSchemaParse(parser_ctxt);
		if (schema == NULL)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "The XML schema is not valid.");
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			return false;
		}
	
		valid_ctxt = xmlSchemaNewValidCtxt(schema);
		if (valid_ctxt == NULL)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to create a validation context for the XML schema.");
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			return false;
		}

		doc = xmlParseFile(fileName.c_str()); /*Parse the XML file*/
		if (doc==NULL)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "XML file '%s' parsing failed.", fileName.c_str());
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			return false;
		}
	
		if(xmlSchemaValidateDoc(valid_ctxt, doc) != 0)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Configuration file '%s' is not valid", fileName.c_str());
			/*Free the allocated resources*/
			freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
			return false;
		}
		
		///Retrieve the names of the NFs
		xmlNodePtr root = xmlDocGetRootElement(doc);
	
		//Load the file describing NFs
		for(xmlNodePtr cur_root_child=root->xmlChildrenNode; cur_root_child!=NULL; cur_root_child=cur_root_child->next)
		{
			if ((cur_root_child->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(cur_root_child->name, (const xmlChar*)NETWORK_FUNCTION_ELEMENT)))
			{
				xmlChar* attr_name = xmlGetProp(cur_root_child, (const xmlChar*)NAME_ATTRIBUTE);
				xmlChar* attr_nports = xmlGetProp(cur_root_child, (const xmlChar*)NUM_PORTS_ATTRIBUTE);
				xmlChar* attr_description = xmlGetProp(cur_root_child, (const xmlChar*)DESCRIPTION_ATTRIBUTE);

				int nports = 0;

				assert(attr_name != NULL);
				if(attr_nports != NULL)
					sscanf((const char*)attr_nports,"%d",&nports);

				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Network function: %s",attr_name);
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Number of ports: %d",nports);
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Description: %s",attr_description);

				string name((const char*)attr_name);
				string description((const char*)attr_description);
				NF *nf = new NF(name,nports,description);

				xmlNodePtr nf_elem = cur_root_child;
				for(xmlNodePtr cur_impl = nf_elem->xmlChildrenNode; cur_impl != NULL; cur_impl = cur_impl->next)
				{
					if ((cur_impl->type == XML_ELEMENT_NODE) && (!xmlStrcmp(cur_impl->name, (const xmlChar*)IMPLEMENTATION_ELEMENT)))
					{
						xmlChar* attr_type = xmlGetProp(cur_impl, (const xmlChar*)TYPE_ATTRIBUTE);
						if (attr_type == NULL) {
							throw string(string("Missing NF implementation type for NF ") +  (char*)attr_name);
						}

						logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\ttype:%s", attr_type);

						nf->addImplementation(Implementation::create((char*)attr_type, cur_impl));
					}
				}
				nfs.insert(nf);
			}
		}

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Configuration file read!");
		return true;

	}
	catch (string errStr) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while loading configuration: %s", errStr.c_str());
	}
	catch (...) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while loading configuration");
	}
	
	freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
	return false;
}

void RestServer::request_completed (void *cls, struct MHD_Connection *connection,
						void **con_cls, enum MHD_RequestTerminationCode toe)
{
	struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);

	if (NULL == con_info)
		return;
	
	free (con_info);
	*con_cls = NULL;
}

int RestServer::answer_to_connection (void *cls, struct MHD_Connection *connection,
			const char *url, const char *method, const char *version,
			const char *upload_data,
			size_t *upload_data_size, void **con_cls)
{
	
	if(NULL == *con_cls)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "New %s request for %s using version %s", method, url, version);
		if(LOGGING_LEVEL <= ORCH_DEBUG)
			MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);
	
		struct connection_info_struct *con_info;
		con_info = (struct connection_info_struct*)malloc (sizeof (struct connection_info_struct));

		if (NULL == con_info)
			return MHD_NO;
		
		if (0 != strcmp (method, GET))
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Method \"%s\" not implemented",method);
			struct MHD_Response *response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
			int ret = MHD_queue_response (connection, MHD_HTTP_NOT_IMPLEMENTED, response);
			MHD_destroy_response (response);
			return ret;
		}
			
		*con_cls = (void*) con_info;
		return MHD_YES;
	}

	if (0 == strcmp (method, GET))
		return doGet(connection,url);
		
	//XXX: just for the compiler
	return MHD_YES;
}


int RestServer::print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s: %s", key, value);
	return MHD_YES;
}

int RestServer::doGet(struct MHD_Connection *connection, const char *url)
{
	struct MHD_Response *response;
	int ret;
	bool digest = false;
	
	//Check the URL
	char delimiter[] = "/";
 	char * pnt;

	char nf_name[BUFFER_SIZE];
	char tmp[BUFFER_SIZE];
	strcpy(tmp,url);
	pnt=strtok(tmp, delimiter);
	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 0:
				if(strcmp(pnt,BASE_URL) != 0)
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Resource \"%s\" does not exist", url);
					response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
					ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
					MHD_destroy_response (response);
					return ret;
				}
				break;
			case 1:
				if(strcmp(pnt,DIGEST_URL) == 0)
					digest = true;
				else
					strcpy(nf_name,pnt);
		}
		
		pnt = strtok( NULL, delimiter );
		i++;
	}
	if(i == 0 || i > 2)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Malformed URL \"%s\"", url);
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
		MHD_destroy_response (response);
		return ret;
	}
	
	if(MHD_lookup_connection_value (connection,MHD_HEADER_KIND, "Host") == NULL)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\"Host\" header not present in the request");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		int ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response (response);
		return ret;
	}
	
	try
	{
		Object json ;

		//Create the json according to the request
		if(i == 1)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Required all the resources");
		
			Array networkFunctions;
			for(set<NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
				networkFunctions.push_back((*nf)->toJSON());
			json["network-functions"] = networkFunctions;
		}
		else
		{
			if(digest)
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Required a summary of the resources");
				
				Array networkFunctions;
				for(set<NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
				{
					Object name;
					name["name"] = (*nf)->getName();
					networkFunctions.push_back(name);
				}
				json["network-functions"] = networkFunctions;
			}
			else
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Required resource: %s",nf_name);
				for(set<NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
				{
					if((*nf)->getName() == nf_name)
					{
						json = (*nf)->toJSON();
						goto ok;
					}
				}
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Method GET is not supported for resource '%s'",nf_name);
				response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
				ret = MHD_queue_response (connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
				MHD_destroy_response (response);
				return ret;	
			}
		}
		
ok:		
		stringstream ssj;
 		write_formatted(json, ssj );
 		string sssj = ssj.str();
 		char *aux = (char*)malloc(sizeof(char) * (sssj.length()+1));
 		strcpy(aux,sssj.c_str());
		response = MHD_create_response_from_buffer (strlen(aux),(void*) aux, MHD_RESPMEM_PERSISTENT);		
		MHD_add_response_header (response, "Content-Type",JSON_C_TYPE);
		MHD_add_response_header (response, "Cache-Control",NO_CACHE);
		ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);
		return ret;
	}catch(...)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while retrieving the json!");
		response = MHD_create_response_from_buffer (0,(void*) "", MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
		MHD_destroy_response (response);
		return ret;
	}	
}


/*
 *   Free all allocated resources used to validate the xml configuration files
 */
void RestServer::freeXMLResources(xmlSchemaParserCtxtPtr parser_ctxt, xmlSchemaValidCtxtPtr valid_ctxt, xmlDocPtr schema_doc, xmlSchemaPtr schema, xmlDocPtr doc)
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
