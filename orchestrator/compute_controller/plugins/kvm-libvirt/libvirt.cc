#include "libvirt.h"
#include "libvirt_constants.h"

#include <unistd.h>
#define GetCurrentDir getcwd
#include <memory>
#include <fstream>

static const char LOG_MODULE_NAME[] = "KVM-Manager";

void Libvirt::customErrorFunc(void *userdata, virErrorPtr err)
{
	ULOG_ERR("Failure of libvirt library call:");
	ULOG_ERR("\tCode: %d", err->code);
	ULOG_ERR("\tDomain: %d", err->domain);
	ULOG_ERR("\tMessage: %s", err->message);
	ULOG_ERR("\tLevel: %d", err->level);
	ULOG_ERR("\tstr1: %s", err->str1);
	ULOG_ERR("\tstr2: %s", err->str2);
	ULOG_ERR("\tstr3: %s", err->str3);
	ULOG_ERR("\tint1: %d", err->int1);
	ULOG_ERR("\tint2: %d", err->int2);
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

	ULOG_DBG_INFO("Connecting to Libvirt ...");
	connection = virConnectOpen("qemu:///system");
	if (connection == NULL)
		ULOG_ERR("Failed to open connection to qemu:///system");
	else
		ULOG_DBG_INFO("Open connection to qemu:///system successfull");
}

void Libvirt::disconnect()
{
	virConnectClose(connection);
	connection = NULL;
}

bool Libvirt::updateNF(UpdateNFIn uni)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr ifn = NULL; //root_node
	doc = xmlNewDoc(BAD_CAST "1.0");
	ifn = xmlNewNode(NULL, BAD_CAST "interface");
	xmlDocSetRootElement(doc, ifn);

	/* Create NICs */
	vector< pair<string, string> > ivshmemPorts; // name, alias

	string nf_name = uni.getNfId();
	map<unsigned int, string> namesOfPortsOnTheSwitch = uni.getNamesOfPortsOnTheSwitch();
	map<unsigned int, port_network_config_t > portsConfiguration = uni.getPortsConfiguration();
	list<unsigned int> newPortsToAdd = uni.getNewPortsToAdd();

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	list<port_mapping_t > control_ports = sni.getControlPorts();
	if(control_ports.size() != 0)
		UN_LOG(ORCH_WARNING, "Required %d control connections for VNF '%s'. Control connections are not supported by KVM type", control_ports.size(),nf_name.c_str());
	list<string> environment_variables = sni.getEnvironmentVariables();
	if(environment_variables.size() != 0)
		UN_LOG(ORCH_WARNING, "Required %d environment variables for VNF '%s'. Environment variables are not supported by KVM type", environment_variables.size(),nf_name.c_str());
#endif

	for(list<unsigned int>::iterator p = newPortsToAdd.begin(); p != newPortsToAdd.end(); p++)
	{
		const unsigned int port_id = *p;
		const string& port_name = namesOfPortsOnTheSwitch[port_id];

		PortTechnology port_technology = description->getPortTechnologies().at(port_id);
		ULOG_DBG_INFO("NF Port \"%s\":%d (%s) is of type %s", nf_name.c_str(), port_id, port_name.c_str(), portTechnologyToString(port_technology).c_str());

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		/* retrieve ip address */
		if(!portsConfiguration[port_id].ip_address.empty())
			UN_LOG(ORCH_WARNING, "Required ip address configuration for VNF '%s'. Ip address configuration are not supported by KVM type", control_ports.size(),nf_name.c_str());
#endif
		/* retrieve mac address */
		string port_mac_address = portsConfiguration[port_id].mac_address;

		ULOG_DBG("Interface \"%s\" associated with MAC address \"%s\"", port_name.c_str(), port_mac_address.c_str());

		if (port_technology == USVHOST_PORT) {
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
		else if (port_technology == IVSHMEM_PORT) {
			ULOG_INFO("Update not supported by functions with this type of ports: %s", IVSHMEM_PORT);
			return false;
		}
		else if (port_technology == VHOST_PORT) {
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
				ULOG_ERR("Something went wrong in the creation of the ports for the VNF...");
				return false;
			}
	}

	/* Get resulting document */
	xmlChar* xml_buf; int xml_bufsz; const char *xmlconfig = NULL;
	xmlDocDumpMemory(doc, &xml_buf, &xml_bufsz);
	xmlconfig = (const char *)xml_buf;

	char vm_name[64];
	sprintf(vm_name, "%" PRIu64 "_%s", uni.getLsiID(), uni.getNfId().c_str());
	/*update the VM*/
	if(virDomainAttachDevice(virDomainLookupByName(connection, vm_name),xmlconfig) != 0){
		ULOG_ERR("failed to update VM. %s", vm_name);
		return false;
	}
	return true;
}

bool Libvirt::startNF(StartNFIn sni)
{
	virDomainPtr dom = NULL;
	char domain_name[64];
	const char *xmlconfig = NULL;
	NFtemplate *temp = description->getTemplate();

	if(description->getTemplate()->getCores() == 0){
		ULOG_ERR("Core numbers have not been found in the template for implementation kvm");
		return false;
	}
	if(description->getTemplate()->getPlatform() == ""){
		ULOG_ERR("Platform type has not been found in the template for implementation kvm");
		return false;
	}
	string nf_name = sni.getNfId();
	string uri_image = temp->getURI();

	ULOG_DBG_INFO("Loading base libvirt template '%s'...",BASE_LIBVIRT_TEMPLATE);

	/* Domain name */
	sprintf(domain_name, "%" PRIu64 "_%s", sni.getLsiID(), nf_name.c_str());
	xmlInitParser();

	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	/* Load XML document */
	doc = xmlParseFile(BASE_LIBVIRT_TEMPLATE);
	if (doc == NULL) {
		ULOG_ERR("Unable to parse file \"%s\"", BASE_LIBVIRT_TEMPLATE);
		return 0;
	}

	/* xpath evaluation for Libvirt various elements we may want to update */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		ULOG_ERR("Unable to create new XPath context");
		xmlFreeDoc(doc);
		return 0;
	}
	const xmlChar* xpathExpr = BAD_CAST "/domain/devices/interface|/domain/name|/domain/devices/emulator";
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		ULOG_ERR("Error: unable to evaluate xpath expression \"%s\"", xpathExpr);
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
	ULOG_DBG("xpath return size: %d", size);
	int i;
	for(i = size - 1; i >= 0; i--) {
		xmlNodePtr node = nodes->nodeTab[i];

		if (node != NULL) {
			switch (node->type) {
				case XML_ELEMENT_NODE:
					if (xmlStrcmp(node->name, (xmlChar*)"name") == 0) {
						xmlNodeSetContent(node, BAD_CAST domain_name); //set the domain name, which is different for each network function
						update_flags |= DOMAIN_NAME_UPDATED;
					}
					else if (xmlStrcmp(node->name, (xmlChar*)"emulator") == 0) {
						if (QEMU_BIN_PATH) {
							xmlNodeSetContent(node, (xmlChar*)QEMU_BIN_PATH);
							update_flags |= EMULATOR_UPDATED;
						}
					}
#if 0
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
#endif
					break;
				case XML_ATTRIBUTE_NODE:
					ULOG_ERR("ATTRIBUTE found here");
					break;
				default:
					ULOG_ERR("Other type");
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

	ULOG_DBG_INFO("Base libvirt template loaded...");

#if 0
	/* Set the domain name */
	if (0 == (update_flags & DOMAIN_NAME_UPDATED)) {
		xmlNewTextChild(xmlDocGetRootElement(doc), NULL, BAD_CAST "name", BAD_CAST domain_name);
	}
#endif

	/* Create xpath evaluation context for Libvirt domain/devices */
	/* Evaluate xpath expression */
	const xmlChar* xpathExpr_devs = BAD_CAST "/domain/devices";
	xpathObj = xmlXPathEvalExpression(xpathExpr_devs, xpathCtx);
	if(xpathObj == NULL) {
		ULOG_ERR("Error: unable to evaluate xpath expression \"%s\"", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return 0;
	}
	nodes = xpathObj->nodesetval;
	if (!nodes || (nodes->nodeNr != 1)) {
		ULOG_DBG_INFO("xpath(devices) failed accessing <devices> node");
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

	ULOG_DBG_INFO("The network function image is available at '%s'...",uri_image.c_str());

	/* Create disk using the NF image specified in the uri */
	xmlNodePtr diskn = xmlNewChild(devices, NULL, BAD_CAST "disk", NULL);
	xmlNewProp(diskn, BAD_CAST "type", BAD_CAST "file");
	xmlNewProp(diskn, BAD_CAST "device", BAD_CAST "disk");
	
	xmlNodePtr drivern = xmlNewChild(diskn, NULL, BAD_CAST "driver", NULL);
	xmlNewProp(drivern, BAD_CAST "name", BAD_CAST "qemu");
	xmlNewProp(drivern, BAD_CAST "type", BAD_CAST "qcow2"); //FIXME: this must not be fixed, but it should depend on the disk image

	xmlNodePtr sourcen = xmlNewChild(diskn, NULL, BAD_CAST "source", NULL);
	xmlNewProp(sourcen, BAD_CAST "file", BAD_CAST uri_image.c_str());

	xmlNewChild(diskn, NULL, BAD_CAST "backingStore", NULL);

	xmlNodePtr targetn = xmlNewChild(diskn, NULL, BAD_CAST "target", NULL);
	xmlNewProp(targetn, BAD_CAST "dev", BAD_CAST "vda");
	xmlNewProp(targetn, BAD_CAST "bus", BAD_CAST "virtio");

	xmlNodePtr aliasn = xmlNewChild(diskn, NULL, BAD_CAST "alias", NULL);
	xmlNewProp(aliasn, BAD_CAST "name", BAD_CAST "virtio-disk0");

	xmlNodePtr addressn = xmlNewChild(diskn, NULL, BAD_CAST "address", NULL);
	xmlNewProp(addressn, BAD_CAST "type", BAD_CAST "pci");
	xmlNewProp(addressn, BAD_CAST "domain", BAD_CAST "0x0000");
	xmlNewProp(addressn, BAD_CAST "bus", BAD_CAST "0x00");
	xmlNewProp(addressn, BAD_CAST "slot", BAD_CAST "0x05");
	xmlNewProp(addressn, BAD_CAST "function", BAD_CAST "0x0");

	string user_data = sni.getUserData();
	if(user_data!="")
	{
		ULOG_DBG_INFO("A user_data information is provided to the network function");
#ifdef DEBUG_KVM
		ULOG_DBG_INFO("Content of user_data:\n'%s'",user_data.c_str());
#endif
		if(!createDisk(user_data,Configuration::instance()->getVnfImagesPath(),domain_name))
		{
			ULOG_DBG_INFO("An error occured during the disk creation");
			return false;
		}

		xmlNodePtr userDataDiskn = xmlNewChild(devices, NULL, BAD_CAST "disk", NULL);
		xmlNewProp(userDataDiskn, BAD_CAST "type", BAD_CAST "file");
		xmlNewProp(userDataDiskn, BAD_CAST "device", BAD_CAST "disk");

		xmlNodePtr userDataDrivern = xmlNewChild(userDataDiskn, NULL, BAD_CAST "driver", NULL);
		xmlNewProp(userDataDrivern, BAD_CAST "name", BAD_CAST "qemu");
		xmlNewProp(userDataDrivern, BAD_CAST "type", BAD_CAST "raw");

		string disk_path = Configuration::instance()->getVnfImagesPath() + string("/") + domain_name + string("_config.iso");
		xmlNodePtr userDataSourcen = xmlNewChild(userDataDiskn, NULL, BAD_CAST "source", NULL);
		xmlNewProp(userDataSourcen, BAD_CAST "file", BAD_CAST disk_path.c_str());

		xmlNodePtr userDataTargetn = xmlNewChild(userDataDiskn, NULL, BAD_CAST "target", NULL);
		xmlNewProp(userDataTargetn, BAD_CAST "dev", BAD_CAST "vdb");
		xmlNewProp(userDataTargetn, BAD_CAST "bus", BAD_CAST "virtio");
	}

	/* Create device that prints log information of the VM */
	xmlNodePtr logSerialn = xmlNewChild(devices, NULL, BAD_CAST "serial", NULL);
	xmlNewProp(logSerialn, BAD_CAST "type", BAD_CAST "file");

	string logPath = getLogPath(domain_name);
	if(logPath=="")
	{
		ULOG_ERR("Error getting current dir");
		return false;
	}
	ULOG_DBG_INFO("Log file path: %s", logPath.c_str());

	xmlNodePtr logSurcen = xmlNewChild(logSerialn, NULL, BAD_CAST "source", NULL);
	xmlNewProp(logSurcen, BAD_CAST "path", BAD_CAST logPath.c_str());

	xmlNodePtr logTargetn = xmlNewChild(logSerialn, NULL, BAD_CAST "target", NULL);
	xmlNewProp(logTargetn, BAD_CAST "port", BAD_CAST "0");

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

		PortTechnology port_technology = description->getPortTechnologies().at(port_id);
		ULOG_DBG_INFO("NF Port \"%s\":%d (%s) is of type %s", nf_name.c_str(), port_id, port_name.c_str(), portTechnologyToString(port_technology).c_str());

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		/* retrieve ip address */
		if(!portsConfiguration[port_id].ip_address.empty())
			UN_LOG(ORCH_WARNING, "Required ip address configuration for VNF '%s'. Ip address configuration are not supported by KVM type", control_ports.size(),nf_name.c_str());
#endif
		/* retrieve mac address */
		string port_mac_address = portsConfiguration[port_id].mac_address;

		ULOG_DBG("Interface \"%s\" associated with MAC address \"%s\"", port_name.c_str(), port_mac_address.c_str());

		if (port_technology == USVHOST_PORT) {
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
		else if (port_technology == IVSHMEM_PORT) {
			ostringstream local_name;  // Name of the port as known by the VNF internally - We set a convention here
			local_name << "p" << port_id;  // Will result in p<n>_tx and p<n>_rx rings

			ivshmemPorts.push_back(pair<string, string>(port_name, local_name.str()));
		}
		else if (port_technology == VHOST_PORT) {
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
				ULOG_ERR("Something went wrong in the creation of the ports for the VNF...");
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
		ULOG_DBG_INFO("Generating IVSHMEM QEMU command line using ERFS cmd: %s", cmd.str().c_str());

		ostringstream oss;
		oss << "echo " << cmd.str().c_str() << " | nc localhost 16632"; // FIXME: this should be a parameter later
		ULOG_DBG_INFO("final command: %s", oss.str().c_str());

		int r = system(oss.str().c_str());
		if(r == -1 || WEXITSTATUS(r) == -1) {
			ULOG_DBG_INFO("Error executing command line generator");
		}

		char name[256];
		sprintf(name, "/tmp/ivshmem_qemu_cmdline_%lu.%s", sni.getLsiID(), sni.getNfId().c_str());
		FILE *f = fopen(name, "r");
		if(f == NULL) {
			ULOG_DBG_INFO("Error opening file");
			return false;
		}
		if(fgets(cmdline, sizeof(cmdline), f) == NULL) {
			ULOG_DBG_INFO("Error in reading file");
			return false;
		}
		ULOG_DBG_INFO("commandline: %s", cmdline);
		ivshmemCmdElems.push_back(cmdline);
#else

		IvshmemCmdLineGenerator ivshmemCmdGenerator;

#if 1
		if(!ivshmemCmdGenerator.get_single_cmdline(cmdline, sizeof(cmdline), domain_name, ivshmemPorts)) {
			return false;
		}

		ULOG_DBG_INFO("Command line for ivshmem '%s'", cmdline);
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
					ULOG_ERR("Unexpected result from IVSHMEM command line generation: %s", it->c_str());
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
	ULOG_DBG_INFO("Dumping XML to %s", filename.str().c_str());
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
		ULOG_ERR("Domain definition failed");
		return false;
	}

	ULOG_DBG_INFO("VM has started");

	virDomainFree(dom);

	return true;
}

bool Libvirt::stopNF(StopNFIn sni)
{
	/*image_name*/
	char vm_name[64];
	sprintf(vm_name, "%" PRIu64 "_%s", sni.getLsiID(), sni.getNfId().c_str());

	assert(connection != NULL);

	//destroy user_data disk (if exists)
	string userDataDiskPath = Configuration::instance()->getVnfImagesPath() + string("/") + string(vm_name) + string("_config.iso");
	remove(userDataDiskPath.c_str());

	/*destroy the VM*/
	if(virDomainDestroy(virDomainLookupByName(connection, vm_name)) != 0){
		ULOG_ERR("failed to stop (destroy) VM. %s", vm_name);
		return false;
	}

	return true;
}

string Libvirt::getLogPath(char *domain_name)
{
	char cCurrentPath[500];
	if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
		return string();
	stringstream currentDir;
	currentDir << cCurrentPath << "/" << domain_name << ".log";
	return currentDir.str();
}

// Note: user-data and meta-data files must be called in this way to be recognized by cloud-init.
// For concurrence issues (VMs launched at the same times) I have to create an univocal folder where insert these files
bool Libvirt::createDisk(string userData, string folder, string domainName)
{
	string vmTempFolder = folder + string("/") + domainName;
	mkdir(vmTempFolder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	ULOG_DBG_INFO("Creating disk for user_data...");
	// creating file containing user_data and meta_data:
	string metaDataFilePath = vmTempFolder + string("/") + string("meta-data");
	ofstream metaFile(metaDataFilePath);
	metaFile << "instance-id: " << domainName << endl << "local-hostname: cloudimg";
	metaFile.close();
	string userDataFilePath = vmTempFolder + string("/") + string("user-data");
	ofstream userFile(userDataFilePath);
	userFile << userData;
	userFile.close();
	// generating disk:
	string userDataDiskPath = folder + string("/") + domainName + string("_config.iso");
	stringstream cmd_create_disk;
	cmd_create_disk << "genisoimage  -output " << userDataDiskPath << " -volid cidata -joliet -rock " << userDataFilePath << " " << metaDataFilePath;
	ULOG_DBG_INFO("Executing command \"%s\"", cmd_create_disk.str().c_str());
	int retVal = system(cmd_create_disk.str().c_str());
	retVal = retVal >> 8;
	if(retVal != 0)
		return false;
	//remove temp files hosting user_data and meta_data in textual format
	remove(metaDataFilePath.c_str());
	remove(userDataFilePath.c_str());
	remove(vmTempFolder.c_str());
	ULOG_DBG_INFO("Disk created successfully");
	return true;
}
