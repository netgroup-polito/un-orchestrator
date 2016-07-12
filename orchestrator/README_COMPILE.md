# How to compile the un-orchestrator

In order to execute the un-orchestrator, we need to setup different components, namely:

  * a set of libraries and tools needed to compile and execute the un-orchestrator code
  * a virtual switch (either Open vSwitch, ERFS or xDPd) as a base switch for
    the platform
  * one or more execution environments for Virtual Network Functions, e.g., KVM for
    executing VM, Docker, or other.

## Required libraries

In addition to the libraries already listed in the main [../README_COMPILE.md](../README_COMPILE.md),
some more components are required to compile the un-orchestrator.
In the following we list the steps required on an **Ubuntu 14.04**.

	; - sqlite3: command line interface for SQLite 3
	; - libsqlite3-dev: SQLite 3 development files
	; - libssl-dev: SSL development libraries, header files and documentation
	$ sudo apt-get install sqlite3 libsqlite3-dev libssl-dev

	; Install ROFL-common (library to parse OpenFlow messages)
	; Alternatively, a copy of ROFL-common is provided in `[un-orchestrator]/contrib/rofl-common.zip`
	; Please note that you have to use version 0.6; newer versions have a different API that
	; is not compatible with our code.
	
	$ git clone https://github.com/bisdn/rofl-common
	$ cd rofl-common/
	$ git checkout stable-0.6

	; Now install the above library according to the description provided
	; in the cloned folder
	
	; Install inih (a nice library used to read the configuration file)
	$ cd [un-orchestrator]/contrib
	$ unzip inih.zip
	$ cd inih
	$ cp * ../../orchestrator/node_resource_manager/database_manager/SQLite

The following libraries are required if you plan to enable the publisher/subscriber 
mechanism, which is used by the un-orchestrator, for instance, to export the configuration
of the universal node.

	; Install Double Decker (hierarchical messaging system)
	$ git clone https://github.com/Acreo/DoubleDecker
	$ cd DobuleDecker/c/
	
	; Now install the above library according to the description provided
	; in the cloned folder

## Install the proper virtual switch

The current un-orchestrator supports different types of virtual switches.
You have to install the one that you want to use, choosing from the
possibilities listed in this section.


### Open vSwitch (OVSDB)

At first, download the Open vSwitch source code from:

    http://openvswitch.org/releases/openvswitch-2.4.0.tar.gz

Then execute the following commands:

    $ tar -xf openvswitch-2.4.0.tar.gz
    $ cd openvswitch-2.4.0
    $ ./configure --prefix=/ --datarootdir=/usr/share --with-linux=/lib/modules/$(uname -r)/build
    $ make
    $ sudo make install


### Open vSwitch (OVSDB) with DPDK support

Before installing OvS with DPDK, you must download and compile the DPDK library. At first, download
the source code from:

	http://dpdk.org/browse/dpdk/snapshot/dpdk-2.0.0.tar.gz

Then execute the following commands:

    $ tar -xf dpdk-2.0.0.tar.gz
    $ cd dpdk-2.0.0
    $ export DPDK_DIR=`pwd`
    ; modify the file `$DPDK_DIR/config/common_linuxapp` so that
    ; `CONFIG_RTE_BUILD_COMBINE_LIBS=y`
    ; `CONFIG_RTE_LIBRTE_VHOST=y`

To compile OvS with the DPDK support, execute:

	$ make install T=x86_64-ivshmem-linuxapp-gcc
	$ export DPDK_BUILD=$DPDK_DIR/x86_64-ivshmem-linuxapp-gcc/

Details on the DPDK ports, namely `user space vhost` and `ivshmem`, are available
on the [DPDK website](http://dpdk.org/)

Now, download and install Open vSwitch:

	$ wget http://openvswitch.org/releases/openvswitch-2.4.0.tar.gz
	$ tar -xf openvswitch-2.4.0.tar.gz
	$ cd openvswitch-2.4.0
	$ ./configure --with-dpdk=$DPDK_BUILD
	$ make
	$ sudo make install

Now create the ovsdb database:

	$ mkdir -p /usr/local/etc/openvswitch
	$ mkdir -p /usr/local/var/run/openvswitch
	$ sudo rm /usr/local/etc/openvswitch/conf.db
	$ sudo ovsdb-tool create /usr/local/etc/openvswitch/conf.db  \
		/usr/local/share/openvswitch/vswitch.ovsschema


### ERFS with DPDK support

The Ericsson Research Flow Switch has not been released yet in the open source domain, hence its usage is restricted to people that have access to the source code (e.g., members of the [FP7 UNIFY](http://www.fp7-unify.eu) project).
 
In order to install ERFS with DPDK support, you have to follow the steps below.

    $ svn checkout <Unify SVN repo>  
    $ cd <WP5 / Task 5.2 / code / erfs>

    ; Make sure you have DPDK with ivshmem target installed
    ; Also make sure you have a proper C++ build environment with boost
    $ ./makeconf
    $ ./configure --enable-debug=[yes/no]
    $ make

Please note that performance-wise there is some drop on Xeon between DPDK 1.7 and later versions due to the change in the `mbuf` structure.


### xDPd with DPDK support

In order to install xDPd with DPDK support, you have to follow the steps below.

	$ git clone https://github.com/bisdn/xdpd
	$ cd xdpd/
	$ git checkout stable

	;Install all the libraries required by the README provided in this folder
	;Edit config/dpdk.m4 and
	; change DPDK_TARGET="x86_64-native-${OS}app-${TARGET_CC}"
	;     to DPDK_TARGET="x86_64-ivshmem-${OS}app-${TARGET_CC}"
	$ bash autogen.sh
	$ cd build
	$ ../configure --with-hw-support=gnu-linux-dpdk --with-plugins="node_orchestrator rest"
	$ make
	$ sudo make install

**WARNING: Currently, xDPd does not compile on Linux kernels newer than 3.16.0-30.**


## Virtual Execution Environment for network functions

The current un-orchestrator supports different types of execution environments.
You have to install the ones that you want to use, selecting one or more
possibilities from the ones listed in this section.

### Docker

In order to support the Docker execution environment, follow the instructions
provided here:

	http://docs.docker.com/installation/

### QEMU/KVM/Libvirt

This is needed in order to run VNFs in KVM-based virtual machines.
Two flavors of virtual machines are supported:

  * virtual machines that exchange packets with the vSwitch through the `virtio` driver. This configuration allows you to run both traditional processes and DPDK-based processes within the virtual machines. In this case, the host backend for the virtual NICs is implemented through `vhost` in case OvS and xDPd as vSwitches, and through `vhost-user` when OvS-DPDK is used as vSwitch.
  * virtual machines that exchange packets with the vSwitch through shared memory (`ivshmem`). This configuration is oriented to performance, and only supports DPDK-based processes within the virtual machine.

#### Libvirt

In order to start/stop virtual machines, a recent version of Libvirt must be used. 
You can build it from sources using the following commands:

	$ sudo apt-get install libxml-xpath-perl libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev python-dev xsltproc autopoint uuid-dev libxml2-utils
	$ git clone git://libvirt.org/libvirt.git
	; select the commit that is known to work and have the necessary support
	$ cd libvirt
	$ git checkout f57842ecfda1ece8c59718e62464e17f75a27062
	$ ./autogen.sh
	$ make
	$ sudo make install

#### QEMU/KVM

To compile and install the QEMU/KVM execution environment, you need a recent QEMU 
version.

Additionally, for `ivshmem` support, a patch ([`[un-orchestrator]/orchestrator/compute_controller/plugins/kvm-libvirt/patches/ivshmem-qemu-2.2.1.patch`](./compute_controller/plugins/kvm-libvirt/patches/ivshmem-qemu-2.2.1.patch)) 
is needed to introduce the same changes that were present in the old `qemu-1.6-based` 
version included in OVDK (Intel DPDK vSwitch).

Here there are the required steps:

	$ sudo apt-get install libperl-dev libgtk2.0-dev bridge-utils
	$ git clone https://github.com/qemu/qemu.git
	$ cd qemu
	$ git checkout v2.2.1
	; The next step is only required to support `ivshmem`
	$ git apply [un-orchestrator]/orchestrator/compute_controller/plugins/kvm-libvirt/patches/ivshmem-qemu-2.2.1.patch
	$ ./configure --target-list=x86_64-softmmu
	$ make
	$ sudo make install

### DPDK processes

In order to run VNFs implemented as DPDK processes, no further operations are required,
since the DPDK library has already been installed together with the vSwitch.

## Compile the un-orchestrator

We are now ready to compile the un-orchestrator. If you intend to enable support for DPDK IVSHMEM-based ports, you'll need to define environment variables pointing to your build of DPDK.
If you are using xDPd (which includes its own DPDK tree and builds it), this would be:

	$ export RTE_SDK=$XDPD_DIR/build/libs/dpdk
	$ export RTE_TARGET=build

Otherwise use:

	$ export RTE_SDK=$DPDK_DIR
	$ export RTE_TARGET=x86_64-ivshmem-linuxapp-gcc

You can then build the un-orchestrator:

	$ cd orchestrator

	; Choose among possible compilation options
	$ ccmake .

The previous command allows you to select some configuration parameters for the
un-orchestrator, such as the virtual switch used, which kind of execution environment(s)
you want to enable, and more. When you're finished, exit from the `ccmake` interface by 
*generating the configuration files* (press 'c' and 'g') and type the following commands:

	; Create makefile scripts based on the previously selected options
	$ cmake .

	; Compile and create the executable
	$ make
