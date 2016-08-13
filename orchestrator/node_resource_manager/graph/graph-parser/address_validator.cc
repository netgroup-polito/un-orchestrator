#include "address_validator.h"

/**
*	http://stackoverflow.com/questions/4792035/how-do-you-validate-that-a-string-is-a-valid-mac-address-in-c
*/
bool AddressValidator::validateMac(const char* mac)
{
	int i = 0;
	int s = 0;

	while (*mac)
	{
		if (isxdigit(*mac))
			i++;
		else if (*mac == ':' || *mac == '-')
		{
			if (i == 0 || i / 2 - 1 != s)
			break;

			++s;
		}
		else
			s = -1;

		++mac;
	}

	return (i == 12 && (s == 5 || s == 0));
}

/**
*	http://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
*/
bool AddressValidator::validateIpv4(const string &ipAddress)
{
	struct sockaddr_in sa;
	int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
	return result != 0;
}

bool AddressValidator::validateIpv6(const string &ipAddress)
{
	struct sockaddr_in sa;
	int result = inet_pton(AF_INET6, ipAddress.c_str(), &(sa.sin_addr));
	return result != 0;
}

bool AddressValidator::validateIpv4Netmask(const string &netmask)
{
	if(!validateIpv4(netmask))
		return false;

	bool zero = true;
	unsigned int mask;

	int first, second, third, fourth;
	sscanf(netmask.c_str(),"%d.%d.%d.%d",&first,&second,&third,&fourth);
	mask = (first << 24) + (second << 16) + (third << 8) + fourth;

	for(int i = 0; i < 32; i++)
	{
		if(((mask & 0x1) == 0) && !zero)
			return false;
		if(((mask & 0x1) == 1) && zero)
			zero = false;

		mask = mask >> 1;
	}

	return true;
}

/**
*	According to https://en.wikipedia.org/wiki/Multicast_address#Ethernet, 
*	"Ethernet frames with a value of 1 in the least-significant bit of the first
*	octet of the destination address are treated as multicast frames"
*
*	This function assumes that the MAC address is correct
*/
bool AddressValidator::isUnicastMac(const char* mac)
{
	uint8_t octet = 0;
	uint8_t aux = 0;

	assert(validateMac(mac));

	sscanf(mac,"%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 "",&octet,&aux,&aux,&aux,&aux,&aux);

	return ((octet & 0x1) == 0);
}
