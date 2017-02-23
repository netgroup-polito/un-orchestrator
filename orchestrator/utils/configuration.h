#ifndef NODE_ORCHESTRATOR_CONFIGURATION_H
#define NODE_ORCHESTRATOR_CONFIGURATION_H

#include "constants.h"
#include "logger.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <istream>
#include <sstream>
#include <INIReader.h>
#include <string.h>

using namespace std;

class Configuration {
public:
    bool init(string configurationFile);
    list<string> getPhisicalPorts();
    map<string,string> getBootGraphs();
    int getRestPort();
    bool getUserAuthentication();
    char *getDdClientName();
    char *getDdBrokerAddress();
    char *getDdKeyPath();
    bool getOrchestratorInBand();
    string getUnInterface();
    string getUnAddress();
    string getIpsecCertificate();
    string getVnfRepoIp();
    int getVnfRepoPort();
    string getConfigServiceIp();
    int getConfigServicePort();
    string getScriptPath();
    string getVnfImagesPath();
    string getDescriptionFileName();
    string getExternalNetwork();
    string getStartFloatingIpPool();
    string getEndFloatingIpPool();
    string getWanPort();
    static Configuration *instance();

private:
    list<string> physicalPorts;
    map<string,string> bootGraphs;
    int restPort;
    bool userAuth;
    char ddClientName[128];
    char ddBrokerAddress[128];
    char ddKeyPath[1024];
    bool orchestratorInBand;
    string unInterface;
    string unAddress;
    string ipsecCertificate;
    string vnfRepoIp;
    int vnfRepoPort;
    string configServiceIp;
    int configServicePort;
    string scriptPath;
    string vnfImagesPath;
    string descriptionFileName;
    string externalNetwork;
    string startFloatingIpPool;
    string endFloatingIpPool;
    string wanPort;
    static Configuration *s_instance;
    Configuration();
};


#endif //NODE_ORCHESTRATOR_CONFIGURATION_H
