#ifndef LIBVIRT_CONSTANTS_H_
#define LIBVIRT_CONSTANTS_H_ 1

/* TODO - These should come from an orchestrator config file (curently, there is only one for the UN ports) */
static const char* QEMU_BIN_PATH = NULL; /* Can point to qemu bin or a wrapper script that tweaks the command line. If NULL, Libvirt default or path found in XML is used */

static const char* OVS_BASE_SOCK_PATH = "/usr/local/var/run/openvswitch/";

#ifdef ENABLE_KVM_IVSHMEM
	#define QEMU					"./compute_controller/plugins/kvm-libvirt/scripts/qemu_run.sh"
        #define QEMU_ERFS "./compute_controller/plugins/kvm-libvirt/scripts/qemu-run-ivshmem-erfs.sh"

	#define	FIRST_PORT_FOR_MONITOR	4000
	#define QUIT_COMMAND			"quit\n\r"
#endif

#endif //LIBVIRT_CONSTANTS_H_
