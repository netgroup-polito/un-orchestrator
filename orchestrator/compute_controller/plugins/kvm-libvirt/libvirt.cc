#include "libvirt.h"
#include "libvirt_constants.h"

#include <memory>

static const char LOG_MODULE_NAME[] = "KVM-Manager";

void Libvirt::customErrorFunc(void *userdata, virErrorPtr err)
{
	UN_LOG(ORCH_ERROR, "Failure of libvirt library call:");
	UN_LOG(ORCH_ERROR, "\tCode: %d", err->code);
	UN_LOG(ORCH_ERROR, "\tDomain: %d", err->domain);
	UN_LOG(ORCH_ERROR, "\tMessage: %s", err->message);
	UN_LOG(ORCH_ERROR, "\tLevel: %d", err->level);
	UN_LOG(ORCH_ERROR, "\tstr1: %s", err->str1);
	UN_LOG(ORCH_ERROR, "\tstr2: %s", err->str2);
	UN_LOG(ORCH_ERROR, "\tstr3: %s", err->str3);
	UN_LOG(ORCH_ERROR, "\tint1: %d", err->int1);
	UN_LOG(ORCH_ERROR, "\tint2: %d", err->int2);
}


Libvirt::Libvirt():
connection(NULL)
{
	virSetErrorFunc(NULL, customErrorFunc);
	connect();
}

Libvirt::~Libvirt()
{
	if(connection != NULL)
		disconnect();
}

bool Libvirt::isSupported(Description&)
{
	connect();

	if(connection == NULL)
		return false;

	return true;
}

void Libvirt::connect()
{
	if(connection != NULL)
		//The connection is already open
		return;

	UN_LOG(ORCH_DEBUG_INFO, "Connecting to Libvirt ...");
	connection = virConnectOpen("qemu:///system");
	if (connection == NULL)
		UN_LOG(ORCH_ERROR, "Failed to open connection to qemu:///system");
	else
		UN_LOG(ORCH_DEBUG_INFO, "Open connection to qemu:///system successfull");
}

void Libvirt::disconnect()
{
	virConnectClose(connection);
	connection = NULL;
}

bool Libvirt::updateNF(UpdateNFIn uni)
{
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Update not supported by this type of functions");
        return false;
}

bool Libvirt::startNF(StartNFIn sni)
{
	virDomainPtr dom = NULL;
	char domain_name[64];
	const char *xmlconfig = NULL;

	string nf_name = sni.getNfId();
	string uri_image = description->getURI();

	/* Domain name */
	sprintf(domain_name, "%" PRIu64 "_%s", sni.getLsiID(), nf_name.c_str());

	UN_LOG(ORCH_DEBUG_INFO, "Using Libvirt XML template %s", uri_image.c_str());
	xmlInitParser();

	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	/* Load XML document */
	doc = xmlParseFile(uri_image.c_str());
	if (doc == NULL) {
		UN_LOG(ORCH_ERROR, "Unable to parse file \"%s\"", uri_image.c_str());
		return 0;
	}

	/* xpath evaluation for Libvirt various elements we may want to update */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		UN_LOG(ORCH_ERROR, "Unable to create new XPath context");
		xmlFreeDoc(doc);
		return 0;
	}
	const xmlChar* xpathExpr = BAD_CAST "/domain/devices/interface|/domain/name|/domain/devices/emulator";
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		UN_LOG(ORCH_ERROR, "Error: unable to evaluate xpath expression \"%s\"", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return 0;
	}

	enum E_updates {
		EMULATOR_UPDATED = 0x01,
		DOMAIN_NAME_UPDATED = 0x02,
	};
	uint32_t update_flags = 0;

	xmlNodeSetPtr nodes = xpathObj->nodesetval;
	int size = (nodes) ? nodes->nodeNr : 0;
	UN_LOG(ORCH_DEBUG_INFO, "xpath return size: %d", size);
	int i;
	for(i = size - 1; i >= 0; i--) {
	  	xmlNodePtr node = nodes->nodeTab[i];

		if (node != NULL) {
			switch (node->type) {
				case XML_ELEMENT_NODE:
					if (xmlStrcmp(node->name, (xmlChar*)"name") == 0) {
						xmlNodeSetContent(node, BAD_CAST domain_name);
						update_flags |= DOMAIN_NAME_UPDATED;
					}
					else if (xmlStrcmp(node->name, (xmlChar*)"emulator") == 0) {
						if (QEMU_BIN_PATH) {
							xmlNodeSetContent(node, (xmlChar*)QEMU_BIN_PATH);
							update_flags |= EMULATOR_UPDATED;
						}
					}
					else if (xmlStrcmp(node->name, (xmlChar*)"interface") == 0) {
						// Currently we just remove any net interface device present in the template and re-create our own
						// with the exception of bridged interfaces which are handy for managing the VM.
						xmlChar* type = xmlGetProp(node, (xmlChar*)"type");
						if (xmlStrcmp(type, (xmlChar*)"bridge") == 0) {
							xmlFree(type);
							continue;
						}
						xmlUnlinkNode(node);
						xmlFreeNode(node);
					}
					break;
				case XML_ATTRIBUTE_NODE:
					UN_LOG(ORCH_ERROR, "ATTRIBUTE found here");
					break;
				default:
					UN_LOG(ORCH_ERROR, "Other type");
					break;
			}
		}

		/*
		 * All the elements returned by an XPath query are pointers to
		 * elements from the tree *except* namespace nodes where the XPath
		 * semantic is different from the implementation in libxml2 tree.
		 * As a result when a returned node set is freed when
		 * xmlXPathFreeObject() is called, that routine must check the
		 * element type. But node from the returned set may have been removed
		 * by xmlNodeSetContent() resulting in access to freed data.
		 * This can be exercised by running
		 *	   valgrind xpath2 test3.xml '//discarded' discarded
		 * There is 2 ways around it:
		 *   - make a copy of the pointers to the nodes from the result set
		 *	 then call xmlXPathFreeObject() and then modify the nodes
		 * or
		 *   - remove the reference to the modified nodes from the node set
		 *	 as they are processed, if they are not namespace nodes.
		 */
		if (nodes->nodeTab[i]->type != XML_NAMESPACE_DECL) {
			 nodes->nodeTab[i] = NULL;
		}
	}

	/* Cleanup of XPath data */
	xmlXPathFreeObject(xpathObj);

	/* Add domain name if not present */
	if (0 == (update_flags & DOMAIN_NAME_UPDATED)) {
		xmlNewTextChild(xmlDocGetRootElement(doc), NULL, BAD_CAST "name", BAD_CAST domain_name);
	}

	/* Create xpath evaluation context for Libvirt domain/devices */
	/* Evaluate xpath expression */
	const xmlChar* xpathExpr_devs = BAD_CAST "/domain/devices";
	xpathObj = xmlXPathEvalExpression(xpathExpr_devs, xpathCtx);
	if(xpathObj == NULL) {
		UN_LOG(ORCH_ERROR, "Error: unable to evaluate xpath expression \"%s\"", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return 0;
	}
	nodes = xpathObj->nodesetval;
	if (!nodes || (nodes->nodeNr != 1)) {
		UN_LOG(ORCH_DEBUG_INFO, "xpath(devices) failed accessing <devices> node");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return 0;
	}

	xmlNodePtr devices = nodes->nodeTab[0];

	/* Add emulator if not present and must be modified */
	if ((0 == (update_flags & EMULATOR_UPDATED)) && (QEMU_BIN_PATH != NULL)) {
		xmlNewTextChild(devices, NULL, BAD_CAST "emulator", BAD_CAST QEMU_BIN_PATH);
	}

	/* Create XML for VM */

	/* Create NICs */
	vector< pair<string, string> > ivshmemPorts; // name, alias

	map<unsigned int, string> namesOfPortsOnTheSwitch = sni.getNamesOfPortsOnTheSwitch();
	map<unsigned int, port_network_config_t > portsConfiguration = sni.getPortsConfiguration();
	map<unsigned int, port_network_config_t >::iterator pd = portsConfiguration.begin();

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	list<port_mapping_t > control_ports = sni.getControlPorts();
	if(control_ports.size() != 0)
		UN_LOG(ORCH_WARNING, "Required %d control connections for VNF '%s'. Control connections are not supported by KVM type", control_ports.size(),nf_name.c_str());
	list<string> environment_variables = sni.getEnvironmentVariables();
	if(environment_variables.size() != 0)
		UN_LOG(ORCH_WARNING, "Required %d environment variables for VNF '%s'. Environment variables are not supported by KVM type", environment_variables.size(),nf_name.c_str());
#endif

	for(map<unsigned int, string>::iterator p = namesOfPortsOnTheSwitch.begin(); p != namesOfPortsOnTheSwitch.end(); p++, pd++)
	{
		const unsigned int port_id = p->first;
		const string& port_name = p->second;

		PortType port_type = description->getPortTypes().at(port_id);
		UN_LOG(ORCH_DEBUG_INFO, "NF Port \"%s\":%d (%s) is of type %s", nf_name.c_str(), port_id, port_name.c_str(), portTypeToString(port_type).c_str());

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		/* retrieve ip address */
		if(!portsConfiguration[port_id].ip_address.empty())
			UN_LOG(ORCH_WARNING, "Required ip address configuration for VNF '%s'. Ip address configuration are not supported by KVM type", control_ports.size(),nf_name.c_str());
#endif
		/* retrieve mac address */
		string port_mac_address = portsConfiguration[port_id].mac_address;

		UN_LOG(ORCH_DEBUG, "Interface \"%s\" associated with MAC address \"%s\"", port_name.c_str(), port_mac_address.c_str());

		if (port_type == USVHOST_PORT) {
			xmlNodePtr ifn = xmlNewChild(devices, NULL, BAD_CAST "interface", NULL);
			xmlNewProp(ifn, BAD_CAST "type", BAD_CAST "vhostuser");

			xmlNodePtr srcn = xmlNewChild(ifn, NULL, BAD_CAST "source", NULL);
			ostringstream sock_path_os;
			sock_path_os << OVS_BASE_SOCK_PATH << port_name;
			xmlNewProp(srcn, BAD_CAST "type", BAD_CAST "unix");
			xmlNewProp(srcn, BAD_CAST "path", BAD_CAST sock_path_os.str().c_str());
			xmlNewProp(srcn, BAD_CAST "mode", BAD_CAST "client");

			xmlNodePtr modeln = xmlNewChild(ifn, NULL, BAD_CAST "model", NULL);
			xmlNewProp(modeln, BAD_CAST "type", BAD_CAST "virtio");

			xmlNodePtr drvn = xmlNewChild(ifn, NULL, BAD_CAST "driver", NULL);
			xmlNodePtr drv_hostn = xmlNewChild(drvn, NULL, BAD_CAST "host", NULL);
			xmlNewProp(drv_hostn, BAD_CAST "csum", BAD_CAST "off");
			xmlNewProp(drv_hostn, BAD_CAST "gso", BAD_CAST "off");
			xmlNodePtr drv_guestn = xmlNewChild(drvn, NULL, BAD_CAST "guest", NULL);
			xmlNewProp(drv_guestn, BAD_CAST "tso4", BAD_CAST "off");
			xmlNewProp(drv_guestn, BAD_CAST "tso6", BAD_CAST "off");
			xmlNewProp(drv_guestn, BAD_CAST "ecn", BAD_CAST "off");
		}
		else if (port_type == IVSHMEM_PORT) {
			ostringstream local_name;  // Name of the port as known by the VNF internally - We set a convention here
			local_name << "p" << port_id;  // Will result in p<n>_tx and p<n>_rx rings

			ivshmemPorts.push_back(pair<string, string>(port_name, local_name.str()));
		}
		else if (port_type == VHOST_PORT) {
			xmlNodePtr ifn = xmlNewChild(devices, NULL, BAD_CAST "interface", NULL);
			xmlNewProp(ifn, BAD_CAST "type", BAD_CAST "direct");

			if(!port_mac_address.empty())
			{
				xmlNodePtr mac_addr = xmlNewChild(ifn, NULL, BAD_CAST "mac", NULL);
				xmlNewProp(mac_addr, BAD_CAST "address", BAD_CAST port_mac_address.c_str());
			}

			xmlNodePtr srcn = xmlNewChild(ifn, NULL, BAD_CAST "source", NULL);
			xmlNewProp(srcn, BAD_CAST "dev", BAD_CAST port_name.c_str());
			xmlNewProp(srcn, BAD_CAST "mode", BAD_CAST "passthrough");

			xmlNodePtr modeln = xmlNewChild(ifn, NULL, BAD_CAST "model", NULL);
			xmlNewProp(modeln, BAD_CAST "type", BAD_CAST "virtio");

			xmlNodePtr virt = xmlNewChild(ifn, NULL, BAD_CAST "virtualport", NULL);
			xmlNewProp(virt, BAD_CAST "type", BAD_CAST "openvswitch");
			}
			else
			{
				assert(0 && "There is a BUG! You cannot be here!");
				UN_LOG(ORCH_ERROR, "Something went wrong in the creation of the ports for the VNF...");
				return false;
			}
	}

	if (! ivshmemPorts.empty()) {
		char cmdline[512];
		vector<string> ivshmemCmdElems;

#ifdef VSWITCH_IMPLEMENTATION_ERFS
		stringstream ports;

		ostringstream cmd;
		cmd << "group-ivshmems " << sni.getLsiID() << "." << sni.getNfId();
		for (vector< pair<string, string> >::iterator it = ivshmemPorts.begin(); it != ivshmemPorts.end(); ++it) {
			cmd << " IVSHMEM:" << sni.getLsiID() << "-" << it->first;
		}
		UN_LOG(ORCH_DEBUG_INFO, "Generating IVSHMEM QEMU command line using ERFS cmd: %s", cmd.str().c_str());

		ostringstream oss;
		oss << "echo " << cmd.str().c_str() << " | nc localhost 16632"; // FIXME: this should be a parameter later
		UN_LOG(ORCH_DEBUG_INFO, "final command: %s", oss.str().c_str());

		int r = system(oss.str().c_str());
		if(r == -1 || WEXITSTATUS(r) == -1) {
			UN_LOG(ORCH_DEBUG_INFO, "Error executing command line generator");
		}

		char name[256];
		sprintf(name, "/tmp/ivshmem_qemu_cmdline_%lu.%s", sni.getLsiID(), sni.getNfId().c_str());
		FILE *f = fopen(name, "r");
		if(f == NULL) {
			UN_LOG(ORCH_DEBUG_INFO, "Error opening file");
			return false;
		}
		if(fgets(cmdline, sizeof(cmdline), f) == NULL) {
			UN_LOG(ORCH_DEBUG_INFO,"Error in reading file");
			return false;
		}
		UN_LOG(ORCH_DEBUG_INFO,"commandline: %s", cmdline);
		ivshmemCmdElems.push_back(cmdline);
#else

		IvshmemCmdLineGenerator ivshmemCmdGenerator;

#if 1
		if(!ivshmemCmdGenerator.get_single_cmdline(cmdline, sizeof(cmdline), domain_name, ivshmemPorts)) {
			return false;
		}

		UN_LOG(ORCH_DEBUG_INFO, "Command line for ivshmem '%s'", cmdline);
		ivshmemCmdElems.push_back(cmdline);
#else
		// Mempool(s)
		if(!ivshmemCmdGenerator.get_mempool_cmdline(cmdline, sizeof(cmdline))) {
			return false;
		}
		ivshmemCmdElems.push_back(cmdline);
		// Port rings
		for (vector< pair<string,string> >::iterator it = ivshmemPorts.begin(); it != ivshmemPorts.end(); ++it) {
			if(!ivshmemCmdGenerator.get_port_cmdline((it->first).c_str(), (it->second).c_str(), cmdline, sizeof(cmdline))) {
				return false;
			}
			ivshmemCmdElems.push_back(cmdline);
		}
#endif // 1
#endif // ERFS

		if (! ivshmemCmdElems.empty()) {
			xmlNodePtr rootEl = xmlDocGetRootElement(doc);
			xmlNodePtr cmdLineEl = xmlNewChild(rootEl, NULL, BAD_CAST "qemu:commandline", NULL);
			for (vector<string>::iterator it = ivshmemCmdElems.begin(); it != ivshmemCmdElems.end(); ++it) {
				const char* START_KEY = "-device ";
				if (it->compare(0, sizeof(START_KEY), START_KEY) == 0) {
					xmlNodePtr argEl = xmlNewChild(cmdLineEl, NULL, BAD_CAST "qemu:arg", NULL);
					xmlNewProp(argEl, BAD_CAST "value", BAD_CAST it->substr(0, sizeof(START_KEY)-1).c_str());
					argEl = xmlNewChild(cmdLineEl, NULL, BAD_CAST "qemu:arg", NULL);
					xmlNewProp(argEl, BAD_CAST "value", BAD_CAST it->substr(sizeof(START_KEY)).c_str());
				}
				else {
					UN_LOG(ORCH_ERROR, "Unexpected result from IVSHMEM command line generation: %s", it->c_str());
					return false;
				}
			}
		}
	}

	/* Cleanup of XPath data */
	xmlXPathFreeContext(xpathCtx);

	/* Get resulting document */
	xmlChar* xml_buf; int xml_bufsz;
	xmlDocDumpMemory(doc, &xml_buf, &xml_bufsz);
	xmlconfig = (const char *)xml_buf;

	/* Final XML Cleanup */
	xmlFreeDoc(doc);

	/**
	*	IVANO: the following function MUST not be called here. In fact, according to the documentation
	*	"If your application is multithreaded or has a plugin support calling this may crash the application has
	*	another thread or plugin is still using libxml2."
	*/
	// xmlCleanupParser();

#ifdef DEBUG_KVM
	stringstream filename;
	filename << domain_name << ".xml";
	UN_LOG(ORCH_DEBUG_INFO, "Dumping XML to %s", filename.str().c_str());
	FILE* fp = fopen(filename.str().c_str(), "w");
	if (fp) {
		fwrite(xmlconfig, 1, strlen(xmlconfig), fp);
		fclose(fp);
	}
#endif

	assert(connection != NULL);

	dom = virDomainCreateXML(connection, xmlconfig, 0);
	if (!dom) {
		//virDomainFree(dom);
		UN_LOG(ORCH_ERROR, "Domain definition failed");
		return false;
	}

	UN_LOG(ORCH_DEBUG_INFO, "Boot guest");

	virDomainFree(dom);

	return true;
}

bool Libvirt::stopNF(StopNFIn sni)
{
	/*image_name*/
	char vm_name[64];
	sprintf(vm_name, "%" PRIu64 "_%s", sni.getLsiID(), sni.getNfId().c_str());

	assert(connection != NULL);

	/*destroy the VM*/
	if(virDomainDestroy(virDomainLookupByName(connection, vm_name)) != 0){
		UN_LOG(ORCH_ERROR, "failed to stop (destroy) VM. %s", vm_name);
		return false;
	}

	return true;
}

