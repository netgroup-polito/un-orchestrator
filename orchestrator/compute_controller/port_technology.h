#ifndef  PORT_TECHNOLOGY_H
#define PORT_TECHNOLOGY_H
enum PortTechnology {
	INVALID_PORT = -1,
	UNDEFINED_PORT = 0,
	//Ports used for virtual machines
	USVHOST_PORT,			//user space vhost port
	IVSHMEM_PORT,			//ivshmem port
	VHOST_PORT,				//(in kernel) vhost port
	//Ports used fro Docker containers
	VETH_PORT,				//veth pair port
	//Ports used for DPDK processes executed in the host
	DPDKR_PORT				//dpdkr port
};


#endif