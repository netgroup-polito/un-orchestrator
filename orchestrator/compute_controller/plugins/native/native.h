#ifndef NATIVE_H_
#define NATIVE_H_ 1

#pragma once

#include "../../nfs_manager.h"
#include "native_constants.h"
#include "capability.h"
#include "native_description.h"

#include <set>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <typeinfo>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

class Native : public NFsManager
{
private:

	/**
	*	@brief: starting from a netmask, returns the /
	*
	*	@param:	netmask	Netmask to be converted
	*/
	unsigned int convertNetmask(string netmask);

	/**
	*	@brief: contains the list of capabilities available through the node
	*/
	static std::map<std::string, Capability> *capabilities;

	static void freeXMLResources(xmlSchemaParserCtxtPtr parser_ctxt, xmlSchemaValidCtxtPtr valid_ctxt, xmlDocPtr schema_doc, xmlSchemaPtr schema, xmlDocPtr doc);


public:

	Native();

	bool isSupported(Description& descr);
	bool startNF(StartNFIn sni);
	bool stopNF(StopNFIn sni);
	bool updateNF(UpdateNFIn uni);
};

class NativeException : public exception {
public:
	virtual const char* what() const throw()
	{
		return "NativeException";
	}
};

#endif //NATIVE_H_
