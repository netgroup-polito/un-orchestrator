# Porting of the UN to the OpenWRT platform

This document contains the instructions required to compile the UN for the OpenWrt platform.

Warning: The current status of the porting is very preliminary; not all the components have been compiled so far, nor we are sure that the software behaves properly. Therefore this document should be intended as an initial proof-of-concept.

This [page](https://wiki.openwrt.org/toh/start) contains a list of devices supported by OpenWRT. In particular, we tested the above instructions on a [Netgear R6300v2](https://wiki.openwrt.org/toh/netgear/netgear_r6300_v2 "Netgear R6300v2").


## How to cross-compile the un-orchestrator for ARM architecture

In order to cross compile the un-orchestrator, it need to have at least 50 MB of available storage space on the device and it need to follow the following steps.

### Preliminary operations

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
; - sqlite3: command line interface for SQLite 3
; - libsqlite3-dev: SQLite 3 development files
; - libssl-dev: SSL development libraries, header files and documentation
$ sudo apt-get install sqlite3 libsqlite3-dev libssl-dev
```

```sh
; Install ROFL-common (library to parse OpenFlow messages)
; Alternatively, a copy of ROFL-common is provided in `[un-orchestrator]/contrib/rofl-common.zip`
; Please note that you have to use version 0.6; newer versions have a different API that
; is not compatible with our code.

$ git clone https://github.com/bisdn/rofl-common
$ cd rofl-common/
$ git checkout stable-0.6

; Now install the above library according to the description provided
; in the cloned folder
```
```sh
$ install ccache:
$ sudo apt-get install -y ccache &&\
$ echo 'export PATH="/usr/lib/ccache:$PATH"' | tee -a ~/.bashrc &&\
$ source ~/.bashrc && echo $PATH
```

```sh
; Install inih (a nice library used to read the configuration file)
$ cd [un-orchestrator]/contrib
$ unzip inih.zip
$ cd inih
$ cp * ../../orchestrator/node_resource_manager/database_manager/SQLite
```
### Set up a cross-compilation toolchain

The version of the SDK used for our tests, for the Netgear R6300v23 box, is the following: 
OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64.

At first, download the OpenWrt SDK Barries Breaker source code from:
https://downloads.openwrt.org/chaos_calmer/15.05/bcm53xx/generic/OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64.tar.bz2

Then execute the following commands:
```sh
$ cd [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
$ ./scripts/feeds update -a
$ ./scripts/feeds install libmicrohttpd
$ ./scripts/feeds install boost-system
$ ./scripts/feeds install libxml2
$ ./scripts/feeds install libpthread
```

It may happen that the files to be copied (execinfo.h, iconv.h, inttypes.h) are in /usr/include/.
In that case first copy them in usr/local/include/ and then move forward.

```sh
$ cp -r /usr/local/include/rofl [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
$ cp /usr/local/include/execinfo.h [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
$ cp /usr/local/include/iconv.h [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
$ cp /usr/local/include/inttypes.h [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
```

It may happen that the source files in the OpenWRT folder are not updated with the latest changes on UN. So you have to check it.
```sh
$ cd [un-orchestrator]/orchestrator
; copy the paths present in the SOURCES1 variable of the CMakeLists.txt file and past it in the SOURCES variable of
[un-orchestrator]/contrib/OpenWrt/orchestrator/CMakeLists.txt

$ cd [un-orchestrator]/name-resolver
; copy the paths present in the SOURCES variable of the CMakeLists.txt and past it in the SOURCES variable of
[un-orchestrator]/contrib/OpenWrt/name-resolver/CMakeLists.txt
```
Now you can move forward:
```sh
$ cd [un-orchestrator]/contrib/OpenWrt/orchestrator
$ cp * ../../../orchestrator

$ cd [un-orchestrator]/contrib/OpenWrt/name-resolver
$ cp * ../../../name-resolver

$ cp -r [un-orchestrator]/name-resolver [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/package
$ cp -r [un-orchestrator]/orchestrator [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/package

; comments row 152 ("$(CheckDependencies)") in
[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/include/package-ipkg.mk


$ cd [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
$ make V=99
; the compilation should be successful until the linking process with the json-spirit and rofl libraries. Then continue with the cross-compilation of the libraries.
```

If such an error appears during compilation:
```sh
$ ERROR: gettext infrastructure mismatch: using a Makefile.in.in from gettext version 0.18 but the autoconf macros are from getteext version 0.19
```
change row 12 of
```sh
[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/build_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/gdbm-1.11/po/Makefile.in.in
```
as well:
```sh
GETTEXT_MACRO_VERSION = 0.19 
```

### Cross-compilation of the json-spirit library
At first, set the following environment variables:
```sh
$ export STAGING_DIR=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/bin
$ export PATH=$PATH:${STAGING_DIR}
```
Then compile json-spirit 
```sh
$ cd [un-orchestrator]/contrib
$ unzip json-spirit

$ cd OpenWrt/json-spirit-arm
; in "CMakeLists.txt" file in the variable "Boost_ROOT" (line 60) substitute [OPENWRT] with the full directory path of
[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
; check whether the version of the boost-library is the same installed by you. If not, change the path in accordance with the right version
;
; in "openwrt-toolchain.cmake" file replace [KERNEL]
; with the full directory path of
; [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/build_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/linux-bcm53xx/linux-3.18.20

; [json-spirit] refers to the folder unzipped with the command $ unzip json-spirit
$ cp * [json-spirit]/build

; copy [json-spirit] in the home directory
$ cp [json-spirit] ~/
$ cd [json-spirit]/build
; Run CMake and check output for errors.
$ cmake . -DCMAKE_TOOLCHAIN_FILE=~/json-spirit/build/openwrt-toolchain.cmake
$ make

$ cp libjson_spirit.so [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/lib
```

### Cross-compilation of the rofl-common library

Have to use a rofl-common patched presents in [un-orchestrator]/contrib/OpenWrt folder.
```sh
$ cd [un-orchestrator]/contrib
$ unzip rofl-common

; copy [rofl-common] in the home directory
cp -r [rofl-common] ~/
```
Set the following environment variables:
```sh
$ export OPENWRT=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
$ export KERNEL=${OPENWRT}/build_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/linux-bcm53xx/linux-3.18.20
$ export TOOLCHAIN_DIR=${OPENWRT}/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi
$ export STAGING_DIR=${OPENWRT}/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/bin
$ export INCLUDE_DIR=${OPENWRT}/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/include

$ export HOST=${OPENWRT}/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/host/bin

$ export HOST1=${OPENWRT}/staging_dir/host/bin

$ export PATH=${HOST}:${STAGING_DIR}:${STAGING_DIR}:${HOST1}:${STAGING_DIR}:${HOST1}:${HOST1}:${INCLUDE_DIR}:$PATH

$ export CROSS=arm-openwrt-linux-

$ export CFLAGS=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
$ export LDFLAGS=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/lib/ld-uClibc.so.0
```
Then compile rofl-common
```sh
$ cd [un-orchestrator]/contrib
$ unzip rofl-common

; copy [rofl-common] in the home directory
cp -r rofl-common ~/

$ cd ~/rofl-common
$ sudo ./autogen.sh
$ cd build  

$ sudo ../configure --target=arm-openwrt-linux --host=arm-openwrt-linux --build=x86_64-linux-gnu --includedir=$INCLUDE_DIR STAGING_DIR=${STAGING_DIR} PATH=${PATH} CC=${CROSS}gcc AR=${CROSS}ar AS=${CROSS}as STRIP=${CROSS}strip LD=${CROSS}ld RANLIB=${CROSS}ranlib CPP=${CROSS}cpp NM_PATH=${CROSS}nm NM=${CROSS}nm --program-prefix= --program-suffix= --prefix=/usr --exec-prefix=/usr --bindir=/usr/bin --sbindir=/usr/sbin --with-gnu-ld --libexecdir=/usr/lib --sysconfdir=/etc --datadir=/usr/share --localstatedir=/var --mandir=/usr/man --infodir=/usr/info --enable-shared --enable-static 

$ sudo make -j4 CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} STAGING_DIR=${STAGING_DIR} PATH=${PATH} CC=${CROSS}gcc AR=${CROSS}ar AS=${CROSS}as STRIP=${CROSS}strip LD=${CROSS}ld RANLIB=${CROSS}ranlib CPP=${CROSS}cpp NM_PATH=${CROSS}nm NM=${CROSS}nm
```
If such an error appears during compilation:
```sh
$ rofl-common/src/rofl/platform/unix/cunixenv.h
In file included from ../../../../../src/rofl/platform/unix/cunixenv.cc:5:0:
../../../../../src/rofl/platform/unix/cunixenv.h:14:22: fatal error: execinfo.h: No such file or directory
 #include <execinfo.h>
                      ^
compilation terminated
```
change row 14 of 
```sh
[rofl-common]/src/rofl/platform/unix/cunixenv.h
```
as well:
```sh
#include </usr/local/include/execinfo.h>
```
In different linux installation that file can be in /usr/include/

Then copy rofl-common in the OpenWRT folder
```sh
$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/lib
$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0 [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/lib
$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0.1.1 [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/lib
```

### Complete the compilation of the UN
Now you can complete the compilation
```sh
$ cd [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
$ make V=99
```

In order to clean and/or compile only a package you can run:
```sh
; package-name: name of your specific package
$ make package/[package-name]/clean V=99
$ make package/[package-name]/compile V=99 (if you want to compile in debug mode, add CONFIG_DEBUG=y)
```

### Set up OpenWrt environment for Netgear R6300

At first, download the Firmware OpenWrt source code for Netgear R6300 from:
https://downloads.openwrt.org/chaos_calmer/15.05/bcm53xx/generic/openwrt-15.05-bcm53xx-netgear-r6300-v2-squashfs.chk
then install it on the Netgear R6300.
Link for an image with the UN already compiled and ready for using the native functions:
https://owncloud.ipv6.polito.it:9090/index.php/s/VhQ5gvAlupeU0Hd

At second, login to OpenWrt: https://wiki.openwrt.org/doc/howto/firstlogin

then execute the following commands:
```sh
$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib/libjson_spirit.so root@192.168.1.1:/lib
$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib/librofl_common.so.0.1.1 root@192.168.1.1:/lib
$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/usr/lib/libsqlite3.so.0.8.6 root@192.168.1.1:/lib

$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/bin/bcm53xx/packages/base/node-orchestrator_0.0.1-1_bcm53xx.ipk root@192.168.1.1:

$ scp -r [un-orchestrator]/orchestrator/config root@192.168.1.1:

$ ssh root@192.168.1.1

$ opkg update
$ opkg install libmicrohttpd
$ opkg install boost-system
$ opkg install libxml2
$ opkg install libpthread

$ cd /lib
$ ln -s librofl_common.so.0.1.1 librofl_common.so.0
$ ln -s librofl_common.so.0.1.1 librofl_common.so
$ ln -s libsqlite3.so.0.8.6 libsqlite3.so.0
$ ln -s libsqlite3.so.0.8.6 libsqlite3.so

$ cd /root
$ opkg install node-orchestrator_0.0.1-1_bcm53xx.ipk
```

For native functions support it is needed to install sudo and bash
```
$ opkg install sudo bash
```

### Port configuration on Netgear R6300

To use UN on the router, you have to modify port configuration.

Access the web interface of router typing its address in the browser bar. First, go on Network -> Switch. Here it will be the configuration of VLAN 1 and 2; you need to create three other VLAN typing the add button below, so that every physical port of the router refers to a different VLAN. For what concern the VLAN 1 that already existed, leave unchanged the field related to "port1" and "CPU", while for the other fields select "off": it means that VLAN 1 will be connected to port 1. Make the same procedure for all other VLAN connecting them to the corresponding port; don't forget to set the CPU field "tagged".

This is an example of how the final configuration should be:

![vlan-configuration](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/vlan_configuration_OpenWRT_NetGear.png)


After doing this, go on Network -> Interfaces. Here you have to create a new interfaces for each new VLAN created in the previous step (so in this particular case you have to insert 3 interfacecs) clicking on “add new interface” button.

This is an example of the final result:

![interfaces-configuration](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/interface_configuration_OpenWRT_NetGear.png)

