#include "docker.h"

bool Docker::isSupported(Description&)
{
	int retVal;
	std::stringstream ss;
	ss << getenv("un_script_path") << CHECK_DOCKER;
	retVal = system(ss.str().c_str());
	retVal = retVal >> 8;

	logger(ORCH_DEBUG, DOCKER_MODULE_NAME, __FILE__, __LINE__, "Script returned: %d\n",retVal);

	if(retVal > 0)
	{
		logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "Docker deamon is running.");
		return true;
	}

	logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "Docker deamon is not running (at least, it is not running with the LXC implementation).");
	return false;
}

bool Docker::updateNF(UpdateNFIn uni)
{
	uint64_t lsiID = uni.getLsiID();
	string nf_name = uni.getNfId();
	
	list<unsigned int> newPortsToAdd = uni.getNewPortsToAdd();
	unsigned int n_ports = newPortsToAdd.size();
	
	map<unsigned int, string> namesOfPortsOnTheSwitch = uni.getNamesOfPortsOnTheSwitch();
	map<unsigned int, port_network_config_t> portsConfiguration = uni.getPortsConfiguration();
	
	unsigned int num_old_port = namesOfPortsOnTheSwitch.size() - n_ports;
	stringstream command;
	command << getenv("un_script_path") << HOTPLUG_DOCKER_NF << " " << lsiID << " " << nf_name << " " << num_old_port << " " << n_ports;
	for(list<unsigned int>::iterator pn = newPortsToAdd.begin(); pn != newPortsToAdd.end(); pn++)
	{
		assert(portsConfiguration.find(*pn)!=portsConfiguration.end());
		assert(namesOfPortsOnTheSwitch.find(*pn)!=namesOfPortsOnTheSwitch.end());
		port_network_config_t configuration = portsConfiguration[(*pn)];

		command << " "  << namesOfPortsOnTheSwitch[(*pn)];
		command << " ";
		//TODO: consider also the IP address, in case the proper flag is enabled
		if(configuration.mac_address != "")
			command <<  configuration.mac_address;
		else
			command << 0;

		command << " ";
		command << 0;
	}

	logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	if(retVal == 0)
		return false;

	return true;
}

bool Docker::startNF(StartNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfId();

	string uri_image = description->getURI();

	map<unsigned int, string> namesOfPortsOnTheSwitch = sni.getNamesOfPortsOnTheSwitch();
	unsigned int n_ports = namesOfPortsOnTheSwitch.size();

	map<unsigned int, port_network_config_t > portsConfiguration = sni.getPortsConfiguration();
	for(map<unsigned int, port_network_config_t >::iterator configuration = portsConfiguration.begin(); configuration != portsConfiguration.end(); configuration++)
	{
		logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "Network configuration for port: %s:%d",nf_name.c_str(),configuration->first);

		if(configuration->second.mac_address != "")
			logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "\t MAC address: %s",(configuration->second.mac_address).c_str());
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		if(configuration->second.ip_address != "")
			logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "\t IP address: %s",(configuration->second.ip_address).c_str());
#endif
	}

	stringstream command;
	command << getenv("un_script_path") << PULL_AND_RUN_DOCKER_NF << " " << lsiID << " " << nf_name << " " << uri_image << " " << n_ports;
	assert(portsConfiguration.size() == namesOfPortsOnTheSwitch.size());
	//map<unsigned int, port_network_config_t >::iterator configuration = portsConfiguration.begin();
	for(map<unsigned int, string>::iterator pn = namesOfPortsOnTheSwitch.begin(); pn != namesOfPortsOnTheSwitch.end(); pn++)
	{
		port_network_config_t configuration = portsConfiguration[pn->first];

		command << " "  << pn->second;
		command << " ";
		if(configuration.mac_address != "")
			command <<  configuration.mac_address;
		else
			command << 0;

		command << " ";
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
		if(configuration.ip_address != "")
			command <<  configuration.ip_address;
		else
#endif
			command << 0;
	}

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	list<port_mapping_t >  control_ports = sni.getControlPorts();
	command << " " << control_ports.size();
	if(control_ports.size() != 0)
	{
		logger(ORCH_DEBUG, DOCKER_MODULE_NAME, __FILE__, __LINE__, "VNF '%s' requires %d control connections",nf_name.c_str(), control_ports.size());
		for(list<port_mapping_t >::iterator control = control_ports.begin(); control != control_ports.end(); control++)
		{
			logger(ORCH_DEBUG, DOCKER_MODULE_NAME, __FILE__, __LINE__, "\t host TCP port: %s - VNF TCP port: %s",(control->host_port).c_str(), (control->guest_port).c_str());
			command << " " << control->host_port << " " << control->guest_port;
		}
	}
#else
	command << " 0";
#endif

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
	list<string> environment_variables = sni.getEnvironmentVariables();
	command << " " << environment_variables.size();
	if(environment_variables.size() != 0)
	{
		logger(ORCH_DEBUG, DOCKER_MODULE_NAME, __FILE__, __LINE__, "VNF '%s' requires %d environment variables",nf_name.c_str(), environment_variables.size());
		for(list<string>::iterator ev = environment_variables.begin(); ev != environment_variables.end(); ev++)
		{
			logger(ORCH_DEBUG, DOCKER_MODULE_NAME, __FILE__, __LINE__, "\t%s",ev->c_str());
			command << " " << *ev;
		}
	}
#else
	command << " 0";
#endif

	logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

bool Docker::stopNF(StopNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_name = sni.getNfId();

	stringstream command;
	command << getenv("un_script_path") << STOP_DOCKER_NF << " " << lsiID << " " << nf_name;

	logger(ORCH_DEBUG_INFO, DOCKER_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());
	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

#if 0
unsigned int Docker::convertNetmask(string netmask)
{
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
#endif
