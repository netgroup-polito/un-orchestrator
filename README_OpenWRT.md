# Porting of the UN to the OpenWRT platform

This document contains the instructions required to compile the UN for the OpenWRT platform.

In this page there is the list of all devices that are supported by OpenWrt, with the reference to a device page.
https://wiki.openwrt.org/toh/start

# How to compile the un-orchestrator for ARM architecture

In order to cross compile the un-orchestrator, it need to have at least 50 MB of available storage space on the device and it need to follow the following steps.

## Preliminary operations

Ensure that the following libraries are installed on the PC:

```sh
; - build-essential: it includes GCC, basic libraries, etc
; - cmake: to create cross-platform makefiles
; - cmake-curses-gui: nice 'gui' to edit cmake files
; - libboost-all-dev: nice c++ library with tons of useful functions
; - libmicrohttpd-dev: embedded micro http server
; - libxml2-dev: nice library to parse and create xml
; - ethtool: utilities to set some parameters on the NICs (e.g., disable TCP offloading)
; - libncurses-dev
; - subversion
; - git, gawk, libssl-dev
$ sudo apt-get install build-essential cmake cmake-curses-gui libboost-all-dev libmicrohttpd-dev libxml2-dev ethtool libncurses-dev subversion git gawk libssl-dev
```

```sh
$ install ccache:
$ sudo apt-get install -y ccache && echo 'export PATH="/usr/lib/ccache:$PATH"' | tee -a ~/.bashrc && source ~/.bashrc && echo $PATH
```

## Set up a cross-compilation toolchain
#### Selection of the correct SDK
Although the process described in the following is generic (i.e., it is valid for each platform), we have first to download the SDK for a specific platform.

In particular, we used the SDK `OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64`, which is specific for the Netgear 6300v2. Download source code for OpenWrt e orchestrator and set the proper environment variable:
```sh
$ wget https://downloads.openwrt.org/chaos_calmer/15.05/bcm53xx/generic/OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64.tar.bz2
$ tar -jxvf OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64.tar.bz2
$ export OPENWRT=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
```

The procedure has also been tested, except for Double Decker, on the SDK `OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2`, which is specific for Tiesse Imola.

#### Toolchain setup
From here, the procedure is not related to a specific ARM platform.

Execute the following commands to compile the Openwrt Environment:
```sh
$ git clone https://github.com/netgroup-polito/un-orchestrator
$ export UN=[un-orchestrator]
$ export PATH=$PATH:${OPENWRT}/staging_dir/toolchain-*/bin
$ export STAGING_DIR=${OPENWRT}/staging_dir/toolchain-*
$ cd $OPENWRT
# The following line must be executed only in case of OpenWRT 15.05. It seems that this version is missing base feed source.
$ sed -i -e '1isrc-git base https://git.openwrt.org/15.05/openwrt.git\' feeds.conf.default
$ ./scripts/feeds update -a
$ ./scripts/feeds install libmicrohttpd
$ ./scripts/feeds install libxml2
$ ./scripts/feeds install boost-system
```

If the following warning occurs
```sh
WARNING: No feed for package 'expat' found, maybe it's already part of the standard packages?
```
add a new source to file ${OPENWRT}/feeds/packages.index

```sh
cat >> ${OPENWRT}/feeds/packages.index

Source-Makefile: feeds/packages/libs/expat/Makefile
Package: expat
Version: 2.1.0-3
Depends: +libc +SSP_SUPPORT:libssp +USE_GLIBC:librt +USE_GLIBC:libpthread 
Conflicts: 
Menu-Depends: 
Provides: 
Build-Types: host
Section: libs
Category: Libraries
Title: An XML parsing library
Maintainer: Steven Barth <cyrus@openwrt.org>
Source: expat-2.1.0.tar.gz
License: MIT
LicenseFiles: COPYING
Type: ipkg
Description:  A fast, non-validating, stream-oriented XML parsing library.
http://expat.sourceforge.net/
Steven Barth <cyrus@openwrt.org>
@@

; press Ctrl+D to end cat input
```

then execute

```sh
$ ./scripts/feeds uninstall -a
$ ./scripts/feeds install libmicrohttpd
$ ./scripts/feeds install boost-system
$ ./scripts/feeds install libxml2
```

Compile the Openwrt Environment
```sh
$ make -j4 V=99
```

---

---
The following guide details how to compile both the universal-node and the required libraries.
For each package you can find a Makefile and patches that prevent some compilation error, moreover (for future support) the documentation explains how patches have been created.


## Compilation of the json-spirit library
Execute the following commands:
```sh
$ cd ${OPENWRT}
$ cp -r ${UN}/contrib/OpenWrt/package/libjson-spirit ${OPENWRT}/package
$ make package/libjson-spirit/compile V=99
```

## Compilation of the rofl library

### Compile glog
Execute the following commands:
```sh
$ cd ${OPENWRT}
$ cp -r ${UN}/contrib/OpenWrt/package/glog ${OPENWRT}/package
$ make package/glog/compile V=99
```

Cross compiling glog we experienced an error releted to a wrong path. In case you experience the same error, you'll need to edit the file `$OPENWRT/staging_dir/target-*/usr/lib/libglog.la` and set the proper path.

*In our specific case* (Netgear 6300v2), we solved the problem executing the following line
```sh
$ sed -i "s|/home/buildbot/slave-local/bcm53xx_generic/build/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/arm-openwrt-linux-uclibcgnueabi|$OPENWRT/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi|g" $OPENWRT/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/usr/lib/libglog.la
```

### Compile librofl

```sh
$ cp -r ${UN}/contrib/OpenWrt/package/librofl ${OPENWRT}/package
$ cp -f ${OPENWRT}/staging_dir/host/bin/libtoolize ${OPENWRT}/staging_dir/target-*/host/bin/libtoolize
$ make package/librofl/compile V=99
```

#### How to patch librofl
Performe the preliminary commands:
```sh
$ make package/librofl/{clean,prepare} V=99 QUILT=1
```
---
To fix error due to missing include create a new patch:

```sh
$ cd build_dir/target-*/librofl-*
$ quilt new 001-missing_cstdio_include.patch
```
edit the file using the quilt command (to map editing to a patch):
```sh
$ quilt edit src/rofl/common/caddress.cc
```
and add the following include:
```sh
#include <cstdio>
```
Save the changes and repeat the same operation to files:
```sh
src/rofl/common/openflow/cofdescstats.cc
src/rofl/common/openflow/coftablestats.cc
```
the `quilt diff` command show the changes performed, if it looks ok save the patch:
```sh
$ quilt refresh
```
---
To fix error due to less recent kernel version create a new patch:

```sh
$ quilt new 002-eventfd_function.patch
$ quilt edit src/rofl/common/cthread.cpp
```
substitute the following line to:
```sh
-	event_fd = eventfd(0, EFD_NONBLOCK);
+	event_fd = eventfd(0, 0);
```
save the patch:
```sh
$ quilt refresh
```
---
To fix error due to useless include create a new patch:

```sh
$ quilt new 003-execinfo.patch
$ quilt edit examples/ethswctld/cunixenv.h
```
remove the include:
```sh
#include <execinfo.h>
```
save the patch:
```sh
$ quilt refresh
```
---
To fix error due to unknown function pthread_setname_np create a new patch:

```sh
$ quilt new 004-thread_name.patch
$ quilt edit src/rofl/common/cthread.cpp
```
remove these lines:
```sh
if (thread_name.length() && thread_name.length() < 16)
	pthread_setname_np(tid, thread_name.c_str());
```

save the patch:
```sh
$ quilt refresh
```
---

apply the patches and rebuild the package:
```sh
$ cd ${OPENWRT}
$ make package/librofl/update V=99
$ make package/librofl/{clean,compile} package/index V=99
```

## Compilation of the universal-node
Execute the following commands:
```sh
$ cd ${OPENWRT}
$ cp -r ${UN}/contrib/OpenWrt/package/un-orchestrator ${OPENWRT}/package
$ make package/un-orchestrator/compile V=99
```

The compilation may abort due to incorrect configuration options; if it happens you have to select the desired configuration (remember that only native functions can run on Openwrt):
```sh
$ cd ${OPENWRT}/build_dir/target-*/un-orchestrator-*
$ ccmake .
```
compile the UN:
```sh
$ cd ${OPENWRT}
$ make package/un-orchestrator/compile V=99
```


#### How to patch the un-orchestrator

To fix error due to useless include:

```sh
$ make package/un-orchestrator/{clean,prepare} V=99 QUILT=1

$ cd build_dir/target-*/un-orchestrator-*
$ quilt new 001-execinfo.patch

$ quilt edit orchestrator/node_orchestrator.cc
```
remove the following include:
```sh
#include <execinfo.h>
```
Save the changes and apply the patch
```sh
$ quilt refresh
$ cd ${OPENWRT}
$ make package/un-orchestrator/update V=99
```

## Compilation of double decker

** *WARNING:* ** *Double decker compiles correctly, but interaction with the orchestrator did not work during out tests on Netgear 6300v2.*

Double Decker is an extra module that is not necessary to compile the un-orchestrator. Procede if you intend to add double decker support, otherwise jump to the set up of OpenWrt environment

Execute the following commands:
```sh
$ cd ${OPENWRT}
$ cp -rf ${UN}/contrib/OpenWrt/package/libsodium/Makefile ${OPENWRT}/feeds/packages/libs/libsodium
$ ./scripts/feeds install libzmq
```

```sh
$ cd ${OPENWRT}
$ cp -r ${UN}/contrib/OpenWrt/package/czmq ${OPENWRT}/package
$ make package/czmq/compile V=99
```
---
```sh
$ cd ${OPENWRT}
$ cp -r ${UN}/contrib/OpenWrt/package/userspace-rcu ${OPENWRT}/package
$ make package/userspace-rcu/compile V=99
```
---
```sh
$ cd ${OPENWRT}
$ cp -r ${UN}/contrib/OpenWrt/package/doubledecker ${OPENWRT}/package
$ make package/doubledecker/compile V=99
```

#### Enable Double Decker connection on the orchestrator
You will need to recompile the orchestrator. Execute the following commands
```sh
$ cp -f ${UN}/contrib/OpenWrt/package/un-orchestrator-dd/Makefile ${OPENWRT}/package/un-orchestrator
$ cd ${OPENWRT}
$ make package/un-orchestrator/compile V=99
```

Compilation will probably stop due to an error. You need to change the configuration of the UN to use native implementation of NFs.  To enable Double Decker, turn on  Double Decker Connection and Resource Manager too.
```sh
$ cd ${OPENWRT}/build_dir/target-*/un-orchestrator-1.0.0 
$ ccmake .
```
Before compiling the orchestrator, edit the default configuration file and uncomment lines relative to resource manager and double decker. The DD keys json files are stored in /cfg/dd/keys/.


Finally execute
```sh
$ cd ${OPENWRT}
$ make package/un-orchestrator/compile V=99
```

# Netgear R6300v2
### Set up OpenWrt environment
You can get the Firmware OpenWrt source code for Netgear R6300 from https://downloads.openwrt.org/chaos_calmer/15.05/bcm53xx/generic/openwrt-15.05-bcm53xx-netgear-r6300-v2-squashfs.chk


Execute the following commands

```sh
$ export OPENWRT=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
$ export R_IP=[IP_of_your_router]
```

```sh
$ ssh-keygen -f "/home/$(whoami)/.ssh/known_hosts" -R $R_IP
$ ssh root@$R_IP
```

```sh
$ mkdir -p /pkg
$ exit
```

Copy fundamental packages
```sh
$ scp ${OPENWRT}/bin/bcm53xx/packages/base/libjson-spirit_1.0.0-1_bcm53xx.ipk root@$R_IP:/pkg
$ scp ${OPENWRT}/bin/bcm53xx/packages/base/glog_v0.3.4-1_bcm53xx.ipk root@R_IP:/pkg
$ scp ${OPENWRT}/bin/bcm53xx/packages/base/librofl_0.11.1-1_bcm53xx.ipk root@$R_IP:/pkg
$ scp ${OPENWRT}/bin/bcm53xx/packages/base/un-orchestrator_1.0.0-1_bcm53xx.ipk root@$R_IP:/pkg
```
If you compiled DoubleDecker, then copy also these packages
```sh
$ scp ${OPENWRT}/bin/bcm53xx/packages/packages/libsodium_1.0.11-2_bcm53xx.ipk root@$R_IP:/pkg
$ scp ${OPENWRT}/bin/bcm53xx/packages/packages/libzmq-nc_4.1.1-1_bcm53xx.ipk root@$R_IP:/pkg
$ scp ${OPENWRT}/bin/bcm53xx/packages/base/czmq_3.0.2-1_bcm53xx.ipk root@$R_IP:/pkg
$ scp ${OPENWRT}/bin/bcm53xx/packages/base/liburcu_0.9.2-1_bcm53xx.ipk root@$R_IP:/pkg
$ scp ${OPENWRT}/bin/bcm53xx/packages/base/DoubleDecker_0.4-1_bcm53xx.ipk root@$R_IP:/pkg
```
---
Execute the following commands
```sh
$ ssh root@$R_IP
$ opkg update
$ cd /pkg
```

Install UN dependencies
```sh
$ opkg install libjson-spirit_1.0.0-1_bcm53xx.ipk
$ opkg install glog_v0.3.4-1_bcm53xx.ipk
$ opkg install librofl_0.11.1-1_bcm53xx.ipk
$ opkg install openvswitch
```

If you compiled DoubleDecker, install also these packages
```sh
$ opkg install libsodium_1.0.11-2_bcm53xx.ipk
$ opkg install libzmq-nc_4.1.1-1_bcm53xx.ipk
$ opkg install czmq_3.0.2-1_bcm53xx.ipk
$ opkg install liburcu_0.9.2-1_bcm53xx.ipk
$ opkg install DoubleDecker_0.4-1_bcm53xx.ipk
```

Finally install the orchestrator
```sh
$ opkg install un-orchestrator_1.0.0-1_bcm53xx.ipk
```
### Port configuration on Netgear R6300
 
 To use UN on the router, you have to modify port configuration.
 
 Access the web interface of router typing its address in the browser bar. First, go on Network -> Switch. Here it will be the configuration of VLAN 1 and 2; you need to create three other VLAN typing the add button below, so that every physical port of the router refers to a different VLAN. For what concern the VLAN 1 that already existed, leave unchanged the field related to "port1" and "CPU", while for the other fields select "off": it means that VLAN 1 will be connected to port 1. Make the same procedure for all other VLAN connecting them to the corresponding port; don't forget to set the CPU field "tagged".
 
 This is an example of how the final configuration should be:
 
 ![vlan-configuration](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/vlan_configuration_OpenWRT_NetGear.png)
 
 
 After doing this, go on Network -> Interfaces. Here you have to create a new interfaces for each new VLAN created in the previous step (so in this particular case you have to insert 3 interfaces) clicking on “add new interface” button. Select "unmanaged" under protocol.
 
 This is an example of the final result:
 
 ![interfaces-configuration](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/interface_configuration_OpenWRT_NetGear.png)
 

### Start node-orchestrator
To execute the orchestrator you will need to start the ovs server first.
```sh
$ ovs-appctl -t ovsdb-server ovsdb-server/add-remote ptcp:6632
```
Then run the orchestrator
```sh
$ cd /cfg/orchestrator
$ node-orchestrator
```
Now orchestrator is running.

#Tiesse Imola

### Set up OpenWrt environment for Tiesse Imola

The firmware OpenWRT should be already installed on the router.

Access the router via ssh connecting the Ethernet cable to the eth0 port. The default IP address is 192.168.1.97, the default password should be empty. 
```sh
$ ssh root@192.168.1.97:
$root@imolaSDN: password: 
```

Create the orchestrator folder inside /cfg because the root partition does not have enough memory to install the orchestrator and its libraries
```sh
$root@imolaSDN: mkdir /cfg/orchestrator
$root@imolaSDN: exit
```

Copy the system libraries compiled for the router (found offline) needed to install the orchestrator
```sh
$ scp libstdcpp_4.8.3-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator
$ scp boost-system_1_51_0-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator
$ scp boost-chrono_1_51_0-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator
$ scp boost-thread_1_51_0-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator
$ scp libxml2_2.9.2-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator
$ scp libmicrohttpd_0.9.19-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator
$ scp libsqlite3_3070701-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator

$ scp ${OPENWRT}/bin/imola5/packages/base/libjson-spirit_1.0.0-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator

$ scp ${OPENWRT}/bin/imola5/packages/base/librofl_0.10.9-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator

$ scp ${OPENWRT}/bin/imola5/packages/base/un-orchestrator_1.0.0-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator
```

Now enter the router and install the libraries
```sh
$ ssh root@192.168.1.97

$root@imolaSDN cd /cfg/orchestrator
$root@imolaSDN opkg install libstdcpp_4.8.3-1_imola5.ipk
$root@imolaSDN opkg install boost-system_1_51_0-1_imola5.ipk
$root@imolaSDN opkg install boost-chrono_1_51_0-1_imola5.ipk
$root@imolaSDN opkg install boost-thread_1_51_0-1_imola5.ipk
$root@imolaSDN opkg install libxml2_2.9.2-1_imola5.ipk
$root@imolaSDN opkg install libmicrohttpd_0.9.19-1_imola5.ipk
$root@imolaSDN opkg install libsqlite3_3070701-1_imola5.ipk

$root@imolaSDN opkg install libjson-spirit_1.0.0-1_imola5.ipk
$root@imolaSDN opkg install librofl_0.10.9-1_imola5.ipk
$root@imolaSDN opkg install node-orchestrator_0.0.1-1_bcm53xx.ipk
```

### Start node-orchestrator

Disable openvswitch on the router
```sh
$root@imolaSDN openvswitch off
```

There should be no need to map the switch ports with VLAN as you normally do in openwrt. The imola driver already supports the mapping between the operating system side of the network interfaces and the physical switch ports. Typing:
```sh
$root@imolaSDN ifconfig -a 
```
you should see at least four interfaces (port1, port2, port3, Port4).

Start ovsdb-server:
```sh
$root@imolaSDN ovs-appctl -t ovsdb-server ovsdb-server/add-remote ptcp:6632
```

Now you can run the orchestrator.


** *WARNING:* ** *Tiesse Imola lose all installations on shutdown. In /cfg/orchestrator/ you find a script that clean up previous installation, re-install packages and execute the preliminary commandss to startup of the orchestrator.*

