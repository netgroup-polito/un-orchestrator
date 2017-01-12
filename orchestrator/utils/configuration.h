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

using namespace std;

class Configuration {
public:
    bool init(string configurationFile);
    list<string> getPhisicalPorts();
    map<string,string> getBootGraphs();
    int getRestPort();
    bool getUserAuthentication();
    string getDdClientName();
    string getDdBrokerAddress();
    string getDdKeyPath();
    bool getOrchestratorInBand();
    string getUnInterface();
    string getUnAddress();
    string getIpsecCertificate();
    string getVnfRepoIp();
    int getVnfRepoPort();
    string getScriptPath();
    string getVnfImagesPath();

private:
    list<string> physicalPorts;
    map<string,string> bootGraphs;
    int restPort;
    bool userAuth;
    string ddClientName;
    string ddBrokerAddress;
    string ddKeyPath;
    bool orchestratorInBand;
    string unInterface;
    string unAddress;
    string ipsecCertificate;
    string vnfRepoIp;
    int vnfRepoPort;
    string scriptPath;
    string vnfImagesPath;

};


#endif //NODE_ORCHESTRATOR_CONFIGURATION_H
