#ifndef ADDRESS_VALIDATOR_H_
#define ADDRESS_VALIDATOR_H_ 1

#pragma once

#include <string>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

using namespace std;

class AddressValidator
{
public:
	static bool validateMac(const char* mac);
	static bool validateIpv4(const string &ipAddress);
	static bool validateIpv4Netmask(const string &netmask);
	static bool validateIpv6(const string &ipAddress);

	static bool isUnicastMac(const char* mac);
};

#endif // ADDRESS_VALIDATOR_H_
