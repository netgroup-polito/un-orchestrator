#include "dpdk.h"

static const char LOG_MODULE_NAME[] = "DPDK-Process-Manager";

bool Dpdk::isSupported(Description&)
{
	//TODO: we are assuming that, if dpdk is enabled by compilation,
	//it is supported
	return true;
}

bool Dpdk::startNF(StartNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfId();
	uint64_t coreMask = sni.getCoreMask();

	map<unsigned int, string> namesOfPortsOnTheSwitch = sni.getNamesOfPortsOnTheSwitch();
	unsigned int n_ports = namesOfPortsOnTheSwitch.size();

	map<unsigned int, port_network_config_t > portsConfiguration = sni.getPortsConfiguration();
	for(map<unsigned int, port_network_config_t >::iterator configuration = portsConfiguration.begin(); configuration != portsConfiguration.end(); configuration++)
	{
		if(configuration->second.mac_address != "")
			ULOG_WARN("Required to assign MAC address to interface %s:%d. This feature is not supported by DPDK type",nf_name.c_str(),configuration->first);
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		if(configuration->second.ip_address != "")
			ULOG_WARN("Required to assign IP address to interface %s:%d. This feature is not supported by DPDK type",nf_name.c_str(),configuration->first);
#endif
	}

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	list<port_mapping_t > control_ports = sni.getControlPorts();
	if(control_ports.size() != 0)
		ULOG_WARN("Required %d control connections for VNF '%s'. Control connections are not supported by DPDK type", control_ports.size(),nf_name.c_str());
#endif

	string uri_image = description->getURI();

	stringstream uri;

	try {
		DPDKDescription& dpdkDescr = dynamic_cast<DPDKDescription&>(*description);
		if(dpdkDescr.getLocation() == "local")
			uri << "file://";
	} catch (exception& e) {
		ULOG_DBG_INFO("exception %s", e.what());
		return false;
	}

	uri << uri_image;

	stringstream command;
	command << getenv("un_script_path") << PULL_AND_RUN_DPDK_NF << " " << lsiID << " " << nf_name << " " << uri.str() << " " << coreMask <<  " " << NUM_MEMORY_CHANNELS << " " << n_ports;

	for(map<unsigned int, string>::iterator pn = namesOfPortsOnTheSwitch.begin(); pn != namesOfPortsOnTheSwitch.end(); pn++)
		command << " "  << pn->second;

	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

bool Dpdk::stopNF(StopNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfId();

	stringstream command;

	command  << getenv("un_script_path") << STOP_DPDK_NF << " " << lsiID << " " << nf_name;

	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;

}

bool Dpdk::updateNF(UpdateNFIn uni)
{
	ULOG_INFO("Update not supported by this type of functions");
	return false;
}

string Dpdk::getCores() {
	string cores;
	try {

		DPDKDescription& dpdkDescr = dynamic_cast<DPDKDescription&>(*description);
		cores = dpdkDescr.getCores();

	} catch (exception& e) {

		/*
		 * Bad cast
		 * It is not a DPDK description
		 */

		ULOG_WARN("Exception %s raised! Wrong description treated as dpdk description", e.what());
		return "";
	}
	return cores;
}

