# How to compile the un-orchestrator

In order to execute the un-orchestrator, we need to setup different components, namely:

  * a set of libraries needed to compile the un-orchestrator code
  * a virtual switch (either xDPd, ERFS or OpenvSwitch) as a base switch for
    our platform
  * one or more execution environments for virtual network functions, e.g., KVM for
    executing VM, Docker, or other.

## Required libraries

Several libraries are required to compile the un-orchestrator.
In the following we list the steps required on an **Ubuntu 14.04**.

	; Install required libraries
	; - build-essential: it includes GCC, basic libraries, etc
	; - cmake: to create cross-platform makefiles
	; - cmake-curses-gui: nice 'gui' to edit cmake files
	; - libboost-all-dev: nice c++ library with tons of useful functions
	; - libmicrohttpd-dev: embedded micro http server
	; - libxml2-dev: nice library to parse and create xml
	$ sudo apt-get install build-essential cmake cmake-curses-gui libboost-all-dev libmicrohttpd-dev libxml2-dev

	; Install JSON Spirit (nice library to parse JSON files)
	$ git clone https://github.com/sirikata/json-spirit
	; alternatively, a copy of JSON Spirit is provided in `[un-orchestrator]/contrib/json-spirit.zip`
	$ cd json-spirit/

	; Now install the above library according to the description provided
	; in the cloned folder

	; Install ROFL-common  (library to parse OpenFlow messages)
	; alternatively, a copy of ROFL-common is provided in `[un-orchestrator]/contrib/rofl-common.zip`
	; Please note that you have to use version 0.6; newer versions have a different API that
	; is not compatible with our code.
	;
	$ git clone https://github.com/bisdn/rofl-common
	$ cd rofl-common/
	$ git checkout stable-0.6

	; Now install the above library according to the description provided
	; in the cloned folder

## Install the proper virtual switch

The current un-orchestrator supports different types of virtual switches.
You have to install the one that you want to use, choosing from the
possibilities listed in this section.


### xDPd with DPDK support

In order to install xDPd with DPDK support, you have to follow the steps below.

	$ git clone https://github.com/bisdn/xdpd
	$ cd xdpd/

	;Install all the libraries required by the README provided in this folder
	$ bash autogen
	$ cd build
	$ ../configure --with-hw-support=gnu-linux-dpdk --with-plugins="node_orchestrator rest"
	$ make
	$ sudo make install

**WARNING: Currently, xDPd is not compiling on Linux kernels newer than 3.16.0-30.**

### OpenvSwitch (of-config) [DEPRECATED]

OpenvSwitch can be installed with either the OVSDB or OF-CONFIG plugin.
Although both protocols allow to control the switch (e.g., create/delete
new bridging instances, create/delete ports, etc), we found out
that OF-CONFIG is rather limited in terms of capabilities. For instance,
it cannot set the type of port configured on the switch (e.g., virtio
or IVSHMEM), requiring the orchestrator to rely on a combination of
OF-CONFIG commands and bash scripts to perform its job.

For this reason we suggest to install OpenvSwitch with its native OSVDB
support (next section); although OVSDB is not standard, it seems that it
does its job better than OF-CONFIG.

In any case, the compilation instruction for setting up OpenvSwitch with
OF-CONFIG are the following (not guaranteed that those are 100% accurate,
as the OF-CONFIG support in OpenvSwitch is rather primitive).

OvS with the OFCONFIG support can be installed as follows:

	$ sudo apt-get install autoconf automake gcc libtool libxml2 libxml2-dev m4 make openssl dbus

	; Download LIBSSH from
	;       https://red.libssh.org/projects/libssh/files
	; Now install the above library following the INSTALL file provided in the root directory

	; Clone the libnetconf repository
	$ git clone https://github.com/cesnet/libnetconf
    $ cd libnetconf/
    $ git checkout -b 0.9.x origin/0.9.x

	; Install the libnetconf library by following the instructions in the
    ; INSTALL file contained in the root folder of this library.

    ; Download OpenvSwitch from
    ;      from http://openvswitch.org/releases/openvswitch-2.4.0.tar.gz
    $ tar -xf openvswitch-2.4.0.tar.gz
    $ cd openvswitch-2.4.0
    $ ./configure --prefix=/ --datarootdir=/usr/share --with-linux=/lib/modules/$(uname -r)/build
    $ make
    $ sudo make install

	; Clone the of-config repository
	$ git clone https://github.com/openvswitch/of-config

	; Follow the instructions as described in the file INSTALL.md provided in the root folder of that repository.

### OpenvSwitch (OVSDB)

At first, download the OpenvSwitch source code from:

    http://openvswitch.org/releases/openvswitch-2.4.0.tar.gz

Then execute the following commands:

    $ tar -xf openvswitch-2.4.0.tar.gz
    $ cd openvswitch-2.4.0
    $ ./configure --prefix=/ --datarootdir=/usr/share --with-linux=/lib/modules/$(uname -r)/build
    $ make
    $ sudo make install

### OpenvSwitch (OVSDB) with DPDK support

Before installing OvS with DPDK, you must download and compile the DPDK library. At first, download
the source code from:

	http://dpdk.org/browse/dpdk/snapshot/dpdk-2.1.0.tar.gz

Then execute the following commands:

    $ tar -xf dpdk-2.1.0.tar.gz
    $ cd dpdk-2.1.0
    $ export DPDK_DIR=`pwd`
    ; modify the file `$DPDK_DIR/config/common_linuxapp` so that
    ; `CONFIG_RTE_BUILD_COMBINE_LIBS=y`
    ; `CONFIG_RTE_LIBRTE_VHOST=y`

To compile OvS with the DPDK support, execute:

	$ make install T=x86_64-ivshmem-linuxapp-gcc
	$ export DPDK_BUILD=$DPDK_DIR/x86_64-ivshmem-linuxapp-gcc/

Details on the DPDK ports, namely `user space vhost` and `ivshmem`, are available
on the [DPDK website](http://dpdk.org/)

Now, download the OpenvSwitch source code:

    $ git clone https://github.com/openvswitch/ovs

Then execute the following commands:

    $ cd ovs
	$ ./boot.sh
	$ ./configure --with-dpdk=$DPDK_BUILD
	$ make
	$ sudo make install

Now create the ovsdb database:

	$ mkdir -p /usr/local/etc/openvswitch
	$ mkdir -p /usr/local/var/run/openvswitch
	$ sudo rm /usr/local/etc/openvswitch/conf.db
	$ sudo ovsdb-tool create /usr/local/etc/openvswitch/conf.db  \
		/usr/local/share/openvswitch/vswitch.ovsschema

## Virtual Execution Environment for network functions

The current un-orchestrator supports different types of execution environments.
You have to install the ones that you want to use, selecting one or more
possibilities from the ones listed in this section.

### Docker

In order to support the Docker execution environment, follow the instructions
provided here:

	http://docs.docker.com/installation/

### QEMU/KVM/Libvirt

This is needed in order to run network functions in KVM-based virtual machines.
Two flavors of virtual machines are supported:

  * virtual machines that exchange packets with the vSwitch through the `virtio` driver. This configuration allows you to run both traditional processes and DPDK-based processes within the virtual machines. In this case, the host backend for the virtual NICs is implemented through `vhost` in case OvS and xDPd as vSwitches, and through `vhost-user` when OvS-DPDK is used as vSwitch;
  * virtual machines that exchange packets with the vSwitch through shared memory (`ivshmem`). This configuration is oriented to performance, and only supports DPDK-based processes within the virtual machine.

##### Libvirt

In order to start/stop virtual machines, a recent version of Libvirt must be used. 
You can build it from sources using the following commands:

	$ sudo apt-get install libxml-xpath-perl libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev python-dev xsltproc autopoint
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

Additionally, for `ivshmem` support, a patch (`[un-orchestrator]/orchestrator/compute_controller/plugins/kvm-libvirt/patches/ivshmem-qemu-2.2.1.patch`) 
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

In order to run VNFs implemented as DPDK processes, no further operation is required,
since the DPDK library has already been installed together with the vSwitch.

## NF-FG library

These steps are mandatory only if you plan to use the Network Functions -
Forwarding Graph (NF-FG) defined in WP3, which is based on the concept of *virtualizer*.

	; Retrieve the NF-FG library.
	$ cd [un-orchestrator]
	$ git submodule update --init --recursive

Finally, remember to select the proper `cmake` option when compiling the `un-orchestrator`.


## Compile the un-orchestrator

We are now ready to compile the un-orchestrator.

	$ cd orchestrator

	; Choose among possible compilation options
	$ ccmake .

The previous command allows you to select some configuration parameters for the
un-orchestrator, such as the virtual switch used, which kind of execution environment(s)
you want to enable, the NF-FG format to use (the default WP5 one or the one defined
in WP3), etc. When you're finished, exit from
the `ccmake` interface by *generating the configuration files* (press 'c' and 'g')
and type the following commands:

	; Create makefile scripts based on the previously selected options
	$ cmake .

	; Compile and create the executable
	$ make
