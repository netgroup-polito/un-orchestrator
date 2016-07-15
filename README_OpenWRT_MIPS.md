# Porting of the UN to the OpenWRT platform

This document contains the instructions required to compile the UN for the OpenWRT platform.

Warning: The current status of the porting is very preliminary; not all the components have been compiled so far, nor we are sure that the software behaves properly. Therefore this document should be intended as an initial proof-of-concept.

In this page there is the list of all devices that are supported by OpenWrt, with the reference to a device page.
https://wiki.openwrt.org/toh/start

## How to cross-compile the un-orchestrator for MIPS architecture

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

The version of the SDK used for our tests is: 
OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2

Then execute the following commands:
```sh
$ cd [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]
$ ./scripts/feeds update -a
$ ./scripts/feeds install libmicrohttpd
$ ./scripts/feeds install boost-system
$ ./scripts/feeds install libxml2
$ ./scripts/feeds install libpthread
```

It may happen that the files to be copied (execinfo.h, iconv.h, inttypes.h) are in /usr/include/.
In that case first copy them in usr/local/include/ and then move forward.

```sh
$ cp -r /usr/local/include/rofl [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/target-mips_mips32_uClibc-0.9.33.2/include
$ cp /usr/local/include/execinfo.h [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/target-mips_mips32_uClibc-0.9.33.2/include
$ cp /usr/local/include/iconv.h [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/target-mips_mips32_uClibc-0.9.33.2/include
$ cp /usr/local/include/inttypes.h [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/target-mips_mips32_uClibc-0.9.33.2/include
```

It may happen that the source files in the OpenWRT folder are not updated with the latest changes on UN. So you have to check it.
```sh
$ cd [un-orchestrator]/orchestrator
; copy the paths present in the SOURCES1 variable of the CMakeLists.txt and past it in the SOURCES variable of
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


$ cp -r [un-orchestrator]/name-resolver [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/package
$ cp -r [un-orchestrator]/orchestrator [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/package

; comments row 152 ("$(CheckDependencies)") in
[OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/include/package-ipkg.mk


cd [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]
$ make V=99 
; the compilation should be successful until the linking process with the json-spirit and rofl libraries. Then continue with the cross-compilation of the libraries.
```

### Cross-compilation of the json-spirit library
At first, set the following environment variables:
```sh
$ export STAGING_DIR=[OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/bin
$ export PATH=$PATH:${STAGING_DIR}
```
Then compile json-spirit 
```sh
$ cd [un-orchestrator]/contrib
$ unzip json-spirit

$ cd OpenWrt/json-spirit-mips
; in "CMakeLists.txt" file in the variable "Boost_ROOT" (line 60) substitute [OPENWRT] with the full directory path of
[OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]
; check whether the version of the boost-library is the same installed by you. If not, change the path in accordance with the right version
;
; in "openwrt-toolchain.cmake" file replace [KERNEL]
; with the full directory path of
; [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/build_dir/target-mips_mips32_uClibc-0.9.33.2/linux-imola5_vx185/linux-3.10.49

; [json-spirit] refers to the folder unzipped with the command $ unzip json-spirit
$ cp * [json-spirit]/build

; copy [json-spirit] in the home directory
$ cp [json-spirit] ~/
$ cd [json-spirit]/build
; run CMake and check output for errors.
$ cmake . -DCMAKE_TOOLCHAIN_FILE=~/json-spirit/build/openwrt-toolchain.cmake
$ make

$ cp libjson_spirit.so [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/lib
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
$ export OPENWRT=[OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]
$ export KERNEL=${OPENWRT}/build_dir/target-mips_mips32_uClibc-0.9.33.2/linux-imola5_vx185/linux-3.10.49
$ export TOOLCHAIN_DIR=${OPENWRT}/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2
$ export STAGING_DIR=${OPENWRT}/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/bin
$ export INCLUDE_DIR=${OPENWRT}/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/include

$ export HOST=${OPENWRT}/staging_dir/target-mips_mips32_uClibc-0.9.33.2/host/bin

$ export HOST1=${OPENWRT}/staging_dir/host/bin

$ export PATH=${HOST}:${STAGING_DIR}:${STAGING_DIR}:${HOST1}:${STAGING_DIR}:${HOST1}:${HOST1}:${INCLUDE_DIR}:$PATH

$ export CROSS=mips-openwrt-linux-

$ export CFLAGS=[OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/target-mips_mips32_uClibc-0.9.33.2/include
$ export LDFLAGS=[[OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/lib/ld-uClibc.so.0
```
Then compile rofl-common
```sh
$ cd ~/rofl-common
$ sudo ./autogen.sh
$ cd build  

$ sudo ../configure --target=mips-openwrt-linux --host=mips-openwrt-linux --build=x86_64-linux-gnu --includedir=$INCLUDE_DIR STAGING_DIR=${STAGING_DIR} PATH=${PATH} CC=${CROSS}gcc AR=${CROSS}ar AS=${CROSS}as STRIP=${CROSS}strip LD=${CROSS}ld RANLIB=${CROSS}ranlib CPP=${CROSS}cpp NM_PATH=${CROSS}nm NM=${CROSS}nm --program-prefix= --program-suffix= --prefix=/usr --exec-prefix=/usr --bindir=/usr/bin --sbindir=/usr/sbin --with-gnu-ld --libexecdir=/usr/lib --sysconfdir=/etc --datadir=/usr/share --localstatedir=/var --mandir=/usr/man --infodir=/usr/info --enable-shared --enable-static 

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
[rofl-common]/src/rofl/platform/unix/cunixenv.h:
#include <execinfo.h>
```
as well:
```sh
#include </usr/local/include/execinfo.h>
```
Then copy rofl-common in the OpenWRT folder
```sh
$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/lib
$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0 [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/lib
$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0.1.1 [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/toolchain-mips_mips32_gcc-4.8.3_uClibc-0.9.33.2/lib
```

### Complete the compilation of the UN
Now you can complete the compilation
```sh
$ cd [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]
$ make V=99
```

In order to clean and/or compile only a package you can run:
```sh
; package-name: name of your specific package
$ make package/[package-name]/clean V=99
$ make package/[package-name]/compile V=99 (if you want to compile in debug mode, add CONFIG_DEBUG=y)
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

$ scp [json-spirit]/build/libjson_spirit.so root@192.168.1.97:/cfg/orchestrator

$ scp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0.1.1 root@192.168.1.97:/cfg/orchestrator

$ scp [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/staging_dir/target-mips_mips32_uClibc-0.9.33.2/usr/lib/libsqlite3.so.0.8.6
root@192.168.1.97:/cfg/orchestrator

$ scp [OpenWrt-SDK-imola5-for-linux-x86_64-gcc-4.8.3_uClibc-0.9.33.2]/bin/imola5/packages/base/node-orchestrator_0.0.1-1_imola5.ipk root@192.168.1.97:/cfg/orchestrator

$ scp -r [un-orchestrator]/orchestrator/config root@192.168.1.97:/cfg/orchestrator
```

Now enter the router and install the libraries
```sh
$root@imolaSDN ssh root@192.168.1.97

$root@imolaSDN cd /cfg/orchestrator
$root@imolaSDN opkg install libstdcpp_4.8.3-1_imola5.ipk
$root@imolaSDN opkg install boost-system_1_51_0-1_imola5.ipk
$root@imolaSDN opkg install boost-chrono_1_51_0-1_imola5.ipk
$root@imolaSDN opkg install boost-thread_1_51_0-1_imola5.ipk
$root@imolaSDN opkg install libxml2_2.9.2-1_imola5.ipk
$root@imolaSDN opkg install libmicrohttpd_0.9.19-1_imola5.ipk

$root@imolaSDN cd /lib/
$root@imolaSDN ln -s /cfg/orchestrator/libjson_spirit.so

$root@imolaSDN ln -s /cfg/orchestrator/librofl_common.so.0.1.1
$root@imolaSDN ln -s /cfg/orchestrator/librofl_common.so.0.1.1 librofl_common.so
$root@imolaSDN ln -s /cfg/orchestrator/librofl_common.so.0.1.1 librofl_common.so.0

$root@imolaSDN ln -s /cfg/orchestrator/libsqlite3.so.0.8.6
$root@imolaSDN ln -s /cfg/orchestrator/libsqlite3.so.0.8.6 libsqlite3.so
$root@imolaSDN ln -s /cfg/orchestrator/libsqlite3.so.0.8.6 libsqlite3.so.0

$root@imolaSDN cd /cfg/orchestrator
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



