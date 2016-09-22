# Porting of the UN to the OpenWRT platform

This document contains the instructions required to compile the UN for the OpenWRT platform.

In this page there is the list of all devices that are supported by OpenWrt, with the reference to a device page.
https://wiki.openwrt.org/toh/start

# How to compile the un-orchestrator for MIPS architecture

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
$ sudo apt-get install build-essential cmake cmake-curses-gui libboost-all-dev libmicrohttpd-dev libxml2-dev ethtool libncurses-dev subversion
```

```sh
$ install ccache:
$ sudo apt-get install -y ccache &&\
$ echo 'export PATH="/usr/lib/ccache:$PATH"' | tee -a ~/.bashrc &&\
$ source ~/.bashrc && echo $PATH
```

## Set up a cross-compilation toolchain

The version of the SDK used for our tests is: 
OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2.

Execute the following commands to compile the Openwrt Environment:
```sh
$ export OPENWRT=[OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]
$ export PATH=$PATH:${OPENWRT}/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/bin
$ export STAGING_DIR=${OPENWRT}/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2
$ cd $OPENWRT
$ ./scripts/feeds update -a
$ ./scripts/feeds install libmicrohttpd
$ ./scripts/feeds install boost-system
$ ./scripts/feeds install libxml2

$ make -j4 V=99
```

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
Execute the following commands:
```sh
$ cd ${OPENWRT}
$ cp -r ${UN}/contrib/OpenWrt/package/librofl ${OPENWRT}/package
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
$ quit edit src/rofl/common/caddress.cc
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
$ cd build_dir/target-*/un-orchestrator-*:
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

### Start node-orchestrator on Tiesse Imola

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




