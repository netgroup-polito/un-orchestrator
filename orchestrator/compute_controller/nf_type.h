#ifndef NF_TYPE_H
#define NF_TYPE_H
typedef enum{
	DPDK,
	DOCKER,
	KVM,
	NATIVE,
	IOMODULE,
	UNDEFINED
	//[+] Add here other implementations for the execution environment
}nf_t;


#endif
