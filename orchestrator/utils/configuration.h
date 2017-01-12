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
    string getScriptPath();
    string getVnfImagesPath();
    string getDescriptionFileName();
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
    string scriptPath;
    string vnfImagesPath;
    string descriptionFileName;
    static Configuration *s_instance;
    Configuration();
};


#endif //NODE_ORCHESTRATOR_CONFIGURATION_H
