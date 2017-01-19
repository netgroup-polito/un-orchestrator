#ifndef LIBVIRT_H_
#define LIBVIRT_H_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <list>
#include <map>
#include <string.h>
#include <locale>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include "../../../utils/logger.h"
#include "../../../utils/constants.h"
#include "../../../utils/configuration.h"

#include "../../nfs_manager.h"
#include "../../startNF_in.h"

#include "ivshmem_cmdline_generator.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

using namespace std;

class Libvirt : public NFsManager
{
private:

	/**
	*	@bfief: Connection towards Libvirt
	*/
	virConnectPtr connection;

	/**
	*	@brief:	Open a connection with QEMU/KVM
	*/
	void connect();

	/**
	*	@brief: Disconnect from QEMU/KVM
	*/
	void disconnect();

	/**
	*	@brief: Custom error handler
	*/
	static void customErrorFunc(void *userdata, virErrorPtr err);

	/**
	*	@brief: Returns the path of the log file of the specified domain
	*/
	string getLogPath(char *domain_name);

	/**
	*	@brief: Create a disk containing the user_data information
	*/
	bool createUserDataDisk(string userData, string folder, string domainName);

	/**
	*	@brief: Create the copy-on-write disk starting from the image base
	*/
	bool createImgDisk(string imgBasePath, string folder, string domainName);

public:

	Libvirt();
	~Libvirt();

	bool isSupported(Description&);

	bool startNF(StartNFIn sni);
	bool stopNF(StopNFIn sni);
	bool updateNF(UpdateNFIn uni);
};

#endif //LIBVIRT_H_
