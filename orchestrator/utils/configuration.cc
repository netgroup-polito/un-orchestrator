#include "configuration.h"

static const char LOG_MODULE_NAME[] = "Configuration-object";

Configuration *Configuration::s_instance;

Configuration *Configuration::instance()
{
    if (!s_instance)
        s_instance = new Configuration();
    return s_instance;
}

Configuration::Configuration()
{

}

bool Configuration::init(string configurationFile)
{
    INIReader reader(configurationFile);

    if (reader.ParseError() < 0) {
        ULOG_ERR( "Can't load a default-config.ini file");
        return false;
    }

    // ports_name : optional
    string tmp_physical_ports = reader.Get("physical ports", "ports_name", "");
    if(tmp_physical_ports!="")
    {
        ULOG_DBG( "Physical ports read from configuation file: %s",tmp_physical_ports.c_str());
        //the string must start and terminate respectively with [ and ]
        if(tmp_physical_ports.at(0) != '[' || tmp_physical_ports.at(tmp_physical_ports.size()-1) != ']')
        {
            ULOG_ERR( "Wrong list of physical ports '%s'. It must be enclosed in '[...]'",tmp_physical_ports.c_str());
            return false;
        }

        stringstream ss(tmp_physical_ports.substr(1,tmp_physical_ports.size()-2));
        string item;
        while (getline(ss, item, ' ')) {
            ULOG_DBG( "\tphysical port: %s",item.c_str());
            physicalPorts.push_back(item);
        }

    }

    // nf-fgs : optional
    string nffgs = reader.Get("initial graphs", "nffgs", "");
    if(nffgs != "")
    {
        ULOG_DBG( "Initial graphs read from configuation file: %s",nffgs.c_str());
        //the string must start and terminate respectively with [ and ]
        if(nffgs.at(0)!='[' || nffgs.at(nffgs.length()-1)!=']')
        {
            ULOG_ERR( "Wrong list initial graphs '%s'. They must be enclosed in '[...]'",nffgs.c_str());
            return false;
        }
        nffgs=nffgs.substr(1,nffgs.length()-2);

        //the string just read must be tokenized
        istringstream iss(nffgs);
        string graph;
        while (getline(iss, graph, ' '))
        {
            istringstream iss(graph);
            string graphName,graphFile;
            getline(iss, graphName, '=');
            getline(iss, graphFile, '=');
            ULOG_DBG_INFO( "Boot Graph: '%s' - '%s'",graphName.c_str(),graphFile.c_str());
            bootGraphs[graphName]=graphFile;
        }
    }

    // server_port : mandatory
    restPort = reader.GetInteger("rest server", "server_port", -1);

    if(restPort == -1)
    {
        ULOG_ERR( "Error in configuration file '%'s. Mandatory parameter 'server_port' is missing.",configurationFile.c_str());
        return false;
    }

    // user_authentication : optional - false if not specified
    userAuth = reader.GetBoolean("user authentication", "user_authentication", false);

    /* description file to export*/
    descriptionFileName = reader.Get("resource-manager", "description_file", "");

#ifdef ENABLE_DOUBLE_DECKER_CONNECTION
    // client_name : mandatory
	ddClientName = reader.Get("double-decker", "client_name", "");

	// brocker_address : mandatory
	ddBrokerAddress = reader.Get("double-decker", "broker_address", "");

	// key_path : mandatory
	ddKeyPath = reader.Get("double-decker", "key_path", "");

	if(ddClientName=="")
	{
		ULOG_ERR( "Error in configuration file '%'s. Mandatory parameter 'client_name' is missing.",configurationFile.c_str());
		return false;
	}

	if(ddBrokerAddress=="")
	{
		ULOG_ERR( "Error in configuration file '%'s. Mandatory parameter 'brocker_address' is missing.",configurationFile.c_str());
		return false;
	}

	if(ddKeyPath=="")
	{
		ULOG_ERR( "Error in configuration file '%'s. Mandatory parameter 'key_path	' is missing.",configurationFile.c_str());
		return false;
	}
#endif

    // is_in_bande : optional - false if not specified
    orchestratorInBand = reader.GetBoolean("orchestrator", "is_in_band", false);

    /* universal node interface */
    unInterface = reader.Get("orchestrator", "un_interface", "");

    /* local ip */
    unAddress = reader.Get("orchestrator", "un_address", "");

    /* IPsec certificate */
    ipsecCertificate = reader.Get("GRE over IPsec", "certificate", "");

    vnfRepoIp = reader.Get("datastore", "ip_address", "");
    if(vnfRepoIp == ""){
        ULOG_ERR("Error in configuration file '%'s. Mandatory parameter 'ip_address' is missing",configurationFile.c_str());
        return false;
    }

    vnfRepoPort = reader.GetInteger("datastore", "port", -1);
    if(vnfRepoPort == -1) {
        ULOG_ERR("Error in configuration file '%'s. Mandatory parameter 'port' is missing",configurationFile.c_str());
        return false;
    }

    vnfImagesPath = reader.Get("misc", "IMAGE_DIR", "");
    if(vnfImagesPath == "") {
        ULOG_ERR("Error in configuration file '%'s. Mandatory parameter 'IMAGE_DIR' is missing",configurationFile.c_str());
        return false;
    }
    else
    if(vnfImagesPath.at(0) != '/') {
        ULOG_ERR("Error in configuration file '%'s. 'IMAGE_DIR' must be an absolute path",configurationFile.c_str());
        return false;
    }

    /* Path of the script file*/
    scriptPath = reader.Get("misc", "script_path", "./").c_str();
    setenv("un_script_path", scriptPath.c_str(), 1);

    return true;
}

list<string> Configuration::getPhisicalPorts()
{
    return physicalPorts;
}

map<string,string> Configuration::getBootGraphs()
{
    return bootGraphs;
}

int Configuration::getRestPort()
{
    return restPort;
}

bool Configuration::getUserAuthentication()
{
    return userAuth;
}

string Configuration::getDdClientName()
{
    return ddClientName;
}

string Configuration::getDdBrokerAddress()
{
    return ddBrokerAddress;
}

string Configuration::getDdKeyPath()
{
    return ddKeyPath;
}

bool Configuration::getOrchestratorInBand()
{
    return orchestratorInBand;
}

string Configuration::getUnInterface()
{
    return unInterface;
}

string Configuration::getUnAddress()
{
    return unAddress;
}

string Configuration::getIpsecCertificate()
{
    return ipsecCertificate;
}

string Configuration::getVnfRepoIp()
{
    return vnfRepoIp;
}

int Configuration::getVnfRepoPort()
{
    return vnfRepoPort;
}

string Configuration::getScriptPath()
{
    return scriptPath;
}

string Configuration::getVnfImagesPath()
{
    return vnfImagesPath;
}

string Configuration::getDescriptionFileName()
{
    return descriptionFileName;
}