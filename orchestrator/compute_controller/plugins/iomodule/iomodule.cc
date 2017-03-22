#include "iomodule.h"

static const char LOG_MODULE_NAME[] = "Iomodule-Manager";

bool IOmodule::isSupported(Description&)
{
	int retVal;
	std::stringstream ss;
	ss << getenv("un_script_path") << CHECK;
	retVal = system(ss.str().c_str());
	retVal = retVal >> 8;

	ULOG_DBG("Script returned: %d\n",retVal);

	if(retVal == 0)
	{
		ULOG_DBG_INFO("One or more constraints to run iomodule are not satisfied");
		return false;
	}	

	return true;
}

bool IOmodule::updateNF(UpdateNFIn uni)
{
	/*
	uint64_t lsiID = sni.getLsiID();
	string nf_id = sni.getNfId();

	map<unsigned int, string> namesOfPortsOnTheSwitch = sni.getNamesOfPortsOnTheSwitch();
	unsigned int n_ports = namesOfPortsOnTheSwitch.size();	

	NFtemplate *temp = description->getTemplate();
	string uri_image = temp->getURI();
    string nf_name = temp->getCapability();
    string uri_type = uriTypeToString(temp->getURIType());

	stringstream command;
	command << getenv("un_script_path") << UPDATE_IOMODULE_NF << " " << lsiID << " " << nf_id << " " << uri_image << " " << nf_name << " " << uri_type << " " <<n_ports;
	for(std::map<unsigned int, std::string>::iterator pn = namesOfPortsOnTheSwitch.begin(); pn != namesOfPortsOnTheSwitch.end(); pn++)
		command << " " << pn->second;
	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;
	*/
	
	ULOG_DBG_INFO("For now is impossible to update a NF implemented by iomodule");
	return false;
}

bool IOmodule::startNF(StartNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_id = sni.getNfId();

	map<unsigned int, string> namesOfPortsOnTheSwitch = sni.getNamesOfPortsOnTheSwitch();
	unsigned int n_ports = namesOfPortsOnTheSwitch.size();	

	NFtemplate *temp = description->getTemplate();
	string uri_image = temp->getURI();
    string nf_name = temp->getCapability();
    string uri_type = uriTypeToString(temp->getURIType());

	stringstream command;
	command << getenv("un_script_path") << RUN_IOMODULE_NF << " " << lsiID << " " << nf_id << " " << uri_image << " " << nf_name << " " << uri_type << " " <<n_ports;
	for(std::map<unsigned int, std::string>::iterator pn = namesOfPortsOnTheSwitch.begin(); pn != namesOfPortsOnTheSwitch.end(); pn++)
		command << " " << pn->second;
	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());

	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}

bool IOmodule::stopNF(StopNFIn sni)
{
	uint64_t lsiID = sni.getLsiID();
	string nf_id = sni.getNfId();

	NFtemplate *temp = description->getTemplate();
	string uri_image = temp->getURI();
    string nf_name = temp->getCapability();

	stringstream command;
	command << getenv("un_script_path") << STOP_IOMODULE_NF << " " << lsiID << " " << nf_id << " " << nf_name;

	ULOG_DBG_INFO("Executing command \"%s\"",command.str().c_str());
	int retVal = system(command.str().c_str());
	retVal = retVal >> 8;

	if(retVal == 0)
		return false;

	return true;
}
