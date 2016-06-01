# How to launch the un-orchestrator

The full list of command line parameters for the un-orchestrator can be
retrieved by the following command:

    $ sudo ./node-orchestrator --h

Please refer to the help provided by the un-orchestrator itself in order to
understand how to use the different options.

The un-orchestrator requires a virtual switch up and running in the server,
which is completely independent from this software.
Therefore you need to start your preferred vSwitch first, before running
the un-orchestrator. Proper instructions for xDPd, ERFS and Open vSwich are provided
below.

Similarly, the un-orchestrator requires that the Name Resolver is already running; 
please, refer to the instructions provided in [../name-resolver/README.md](../name-resolver/README.md) 
to understand how to execute this component.

### Configuration file examples

Folder `config` contains some configuration file examples that can be used
to configure/test the un-orchestrator.

  * [config/default-config.ini](config/default-config.ini): 
    configuration file for the un-orchestrator. It allows to specify information such 
    as the TCP port used by the REST server, the file describing the physical 
    ports of the Universal Node, and more;
  * [config/universal-node-example.xml](config/universal-node-example.xml): 
    file describing the physical ports to be handled by the 
    un-orchestrator, as well as the amount of CPU, memory and storage provided 
    to the Universal Node;
  * [config/simple_passthrough_nffg.json](config/simple_passthrough_nffg.json): 
    simple graph that implements a simple passthrough function, i.e., traffic is 
    received from a first physical port and sent out from a second physical port, 
    after having been handled to the vswitch;
  * [config/passthrough_with_vnf_nffg.json](config/passthrough_with_vnf_nffg.json): 
    graph that includes a VNF. Traffic is received from a first physical port, provided
    to a network function, and then sent out from a second physical port.

## How to start the proper virtual switch

As stated above, the proper vSwitch must be started on the Universal Node before the boot of the
un-orchestrator; in the following the instructions to run the supported vSwitches are provided.

### How to start OvS (managed through OVSDB) to work with the un-orchestrator

Start OVS:

    $ sudo /usr/share/openvswitch/scripts/ovs-ctl start

Start ovsdb-server:

    $ sudo ovs-appctl -t ovsdb-server ovsdb-server/add-remote ptcp:6632
	
### How to start OvS (managed through OVSDB) with DPDK support to work with the un-orchestrator

Configure the system (after each reboot of the physical machine):

    $ sudo su
    ; Set the huge pages of 2MB; 4096 huge pages should be reasonable.
    $ echo 4096 > /proc/sys/vm/nr_hugepages
	
    ; Umount previous hugepages dir
    $ umount /dev/hugepages
    $ rm -r /dev/hugepages
	
    ; Mount huge pages directory
    $ mkdir /dev/hugepages
    $ mount -t hugetlbfs nodev /dev/hugepages
    
    $ exit
	
Set up DPDK (after each reboot of the physical machine):

    $ sudo modprobe uio
    $ sudo insmod [dpdk-folder]/x86_64-ivshmem-linuxapp-gcc/kmod/igb_uio.ko
    ; Bind the physical network device to `igb_uio`. The following row
    ; shows how to bind eth1. Repeat the command for each network interface
    ; you want to bind.
    $ sudo [dpdk-folder]/tools/dpdk_nic_bind.py --bind=igb_uio eth1

Start `ovsdb-server`:

    $ sudo ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock \
        --remote=db:Open_vSwitch,Open_vSwitch,manager_options --remote=ptcp:6632  --pidfile --detach
	
The first time after the ovsdb database creation, initialize it:

    $ sudo ovs-vsctl --no-wait init

Start the switching daemon:	

    $ export DB_SOCK=/usr/local/var/run/openvswitch/db.sock
    $ sudo ovs-vswitchd --dpdk -c 0x1 -n 4 --socket-mem 1024,0 \
        -- unix:$DB_SOCK --pidfile --detach

### How to start xDPd with DPDK support to work with the un-orchestrator

Set up DPDK (after each reboot of the physical machine), in order to:

  * Build the environment x86_64-native-linuxapp-gcc
  * Insert IGB UIO module
  * Insert KNI module
  * Setup hugepage mappings for non-NUMA systems (1000 should be a reasonable
    number)
  * Bind Ethernet devices to IGB UIO module (bind all the ethernet interfaces
    that you want to use)
 
    $ cd [xdpd]/libs/dpdk/tools  
    $ sudo ./setup.sh  
    ; Follow the instructions provided in the script

Start xDPd:

	$ cd [xdpd]/build/src/xdpd
	$ sudo ./xdpd

xDPd comes with a command line tool called `xcli`, that can be used to check
the  flows installed in the LSIs, which are the LSIs deployed, see statistics
on flows matched, and so on. The `xcli` can be run by just typing:

    $ xcli


## How to start the proper virtual execution environment

Only Libvirt needs to be explicitly started. If you do not intend 
to use this execution environment, you can skip this section.

### How to configure and start libvirt

In case you are planning to use OvS with DPDK support as virtual switch, you have 
to edit the file `/usr/local/etc/libvirt/qemu.conf` by adding the following line:

        hugetlbfs_mount = "/dev/hugepages"

Regardless of the virtual switch used, you have now to stop any running `libvirt` 
instance and run the alternative version installed:

	$ sudo service libvirt-bin stop
	$ sudo /usr/local/sbin/libvirtd --daemon
	
Similarly, if you use `virsh`, you would have to use the version from `/usr/local/bin`.

