#ifndef NF_TYPE_H_
#define NF_TYPE_H_ 1

#include <string>
#include <assert.h>

using namespace std;

typedef enum{
	DPDK,
	DOCKER,
	KVM,
	NATIVE
	//[+] Add here other implementations for the execution environment
	}nf_t;

class NFType
{
public:
	static string toString(nf_t type)
	{
#if defined(ENABLE_DPDK_PROCESSES) || defined(VSWITCH_IMPLEMENTATION_XDPD)
		if(type == DPDK)
			return string("dpdk");
#endif
#if defined(ENABLE_DOCKER) || defined(VSWITCH_IMPLEMENTATION_XDPD)
		if(type == DOCKER)
			return string("docker");
#endif
#ifdef ENABLE_KVM
		if(type == KVM)
			return string("virtual-machine-kvm");
#endif
#ifdef ENABLE_NATIVE
		if(type == NATIVE)
			return string("native");
#endif

		//[+] Add here other implementations for the execution environment

		assert(0);
		return "";
	}

	static bool isValid(string type)
	{
#ifdef ENABLE_DPDK_PROCESSES
		if(type == "dpdk")
			return true;
#endif
#ifdef ENABLE_DOCKER
		if(type == "docker")
			return true;
#endif
#ifdef ENABLE_KVM
		if(type == "virtual-machine-kvm")
			return true;
#endif
#ifdef ENABLE_NATIVE
		if(type == "native")
			return true;
#endif

		//[+] Add here other implementations for the execution environment

		return false;
	}
};

#endif //NF_TYPE_H_
