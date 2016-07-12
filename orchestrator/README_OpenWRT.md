# Porting of the UN to the OpenWRT platform

This document contains the instructions required to compile the UN for the OpenWRT platform.

**Warning**: The current status of the porting is very preliminary; not all the components have been compiled so far, nor we are sure that the software behaves properly. Therefore this document should be intended as an initial proof-of-concept.

In this page there is the list of all devices that are supported by OpenWrt, with the reference to a device page.

	https://wiki.openwrt.org/toh/start

## How to cross-compile the un-orchestrator for ARM architecture

In order to cross compile the un-orchestrator, it need to have at least 50 MB of available storage space on the device and it need to follow the following steps.

## Set up a cross-compilation toolchain

At first, download the OpenWrt SDK Barries Breaker source code from:

	https://wiki.openwrt.org/doc/howto/obtain.firmware.sdk

Then execute the following commands:

	$ cd [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
	$ ./scripts/feeds update -a
	$ ./scripts/feeds install libmicrohttpd
	$ ./scripts/feeds install boost-system
	$ ./scripts/feeds install libxml2
	$ ./scripts/feeds install libpthread
		
	$ cp /usr/local/include/rofl [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
	$ cp /usr/local/include/execinfo.h [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
	$ cp /usr/local/include/iconv.h [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
	$ cp /usr/local/include/inttypes.h [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include

	$ cd [un-orchestrator]/contrib
	$ unzip OpenWrt.zip
	$ cd OpenWrt
	$ cp * ../../orchestrator

	$ cp -r [un-orchestrator]/ [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/package

	replace row 791 of [un-orchestrator]/orchestrator/node_resource_manager/rest_server/match_parser.cc with this "if((sscanf(value.getString().c_str(),"%"SCNd64,&ipv6FLabel) != 1) || (ipv6FLabel > 4294967295UL) )"
	
	comments row 152 of [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.\_eabi.Linux-x86_64]/include/package-ipkg.mk
		
	make V=99

## Cross-compilation of the json-spirit library

At first, set the following environment variables:

	$ export OPENWRT=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/
	$ export KERNEL=${OPENWRT}/build_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/linux-bcm53xx/linux-3.18.20
	$ export STAGING_DIR=${OPENWRT}/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2/bin
	$ export PATH=$PATH:${STAGING_DIR}

Then execute the following commands:

	$ cd [un-orchestrator]/contrib
	$ unzip OpenWrt.zip
	$ cd OpenWrt/json-spirit
	$ cp * [json-spirit]/build

	$ cd [json-spirit]/build
	# Run CMake and check output for errors.
	$ cmake . -DCMAKE_TOOLCHAIN_FILE=~/json-spirit/build/openwrt-toolchain.cmake
	$ make

	$ cp libjson_spirit.so [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2/lib

## Cross compilation of the rofl-common library

Have to use a rofl-common patched presents in [un-orchestrator]/contrib/OpenWrt folder.

At first, set the following environment variables:

	$ export OPENWRT=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]
	$ export KERNEL=${OPENWRT}/build_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/linux-bcm53xx/linux-3.18.20
	$ export TOOLCHAIN_DIR=${OPENWRT}/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2
	$ export STAGING_DIR=${OPENWRT}/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2/bin
	$ export INCLUDE_DIR=${OPENWRT}/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2/include

	$ export HOST=${OPENWRT}/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/host/bin

	$ export HOST1=${OPENWRT}/staging_dir/host/bin

	$ export PATH=${HOST}:${STAGING_DIR}:${STAGING_DIR}:${HOST1}:${STAGING_DIR}:${HOST1}:${HOST1}:${INCLUDE_DIR}:$PATH

	$ export CROSS=arm-openwrt-linux-

	$ export CFLAGS=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/include
	$ export LDFLAGS=[OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/toolchain-arm_cortex-a9_gcc-4.8-linaro_uClibc-0.9.33.2/lib/ld-uClibc.so.0

Then execute the following commands:

	$ cd [rofl-common]
	$ ./autogen.sh
	$ cd build  
	$ sudo ../configure --target=arm-openwrt-linux --host=arm-openwrt-linux --build=x86_64-linux-gnu --includedir=$INCLUDE_DIR STAGING_DIR=${STAGING_DIR} PATH=${PATH} CC=${CROSS}gcc AR=${CROSS}ar AS=${CROSS}as STRIP=${CROSS}strip LD=${CROSS}ld RANLIB=${CROSS}ranlib CPP=${CROSS}cpp NM_PATH=${CROSS}nm NM=${CROSS}nm --program-prefix= --program-suffix= --prefix=/usr --exec-prefix=/usr --bindir=/usr/bin --sbindir=/usr/sbin --with-gnu-ld --libexecdir=/usr/lib --sysconfdir=/etc --datadir=/usr/share --localstatedir=/var --mandir=/usr/man --infodir=/usr/info --enable-shared --enable-static 
	$ sudo make -j4 CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} STAGING_DIR=${STAGING_DIR} PATH=${PATH} CC=${CROSS}gcc AR=${CROSS}ar AS=${CROSS}as STRIP=${CROSS}strip LD=${CROSS}ld RANLIB=${CROSS}ranlib CPP=${CROSS}cpp NM_PATH=${CROSS}nm NM=${CROSS}nm

	$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib
	$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0 [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib
	$ cp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0.1.1 [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib

## Set up OpenWrt environment for Netgear R6300

At first, download the Firmware OpenWrt source code for Netgear R6300 from:

	https://downloads.openwrt.org/latest/bcm53xx/generic/openwrt-15.05-bcm53xx-netgear-r6300-v2-squashfs.chk

then install it on the Netgear R6300.

At second, login to OpenWrt:

	https://wiki.openwrt.org/doc/howto/firstlogin

then execute the following commands:

	$ scp [json-spirit]/build/libjson_spirit.so root@192.168.1.1:

	$ scp [rofl-common]/build/src/rofl/.libs/librofl_common.so root@192.168.1.1:
	$ scp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0 root@192.168.1.1:
	$ scp [rofl-common]/build/src/rofl/.libs/librofl_common.so.0.1.1 root@192.168.1.1:

	$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib/libjson_spirit.so root@192.168.1.1:/lib
	$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib/librofl_common.so root@192.168.1.1:/lib
	$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib/librofl_common.so.0 root@192.168.1.1:/lib
	$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/staging_dir/target-arm_cortex-a9_uClibc-0.9.33.2_eabi/lib/librofl_common.so.0.1.1 root@192.168.1.1:/lib

	$ scp [OpenWrt-SDK-15.05-bcm53xx_gcc-4.8-linaro_uClibc-0.9.33.2_eabi.Linux-x86_64]/bin/bcm53xx/packages/base/node-orchestrator_0.0.1-1_bcm53xx.ipk root@192.168.1.1:

	$ scp -r [un-orchestrator]/orchestrator/config root@192.168.1.1:

	$ ssh root@192.168.1.1

	$ opkg install node-orchestrator_0.0.1-1_bcm53xx.ipk

## Network configuration of the device

Due to hardware constraints, the OpenWRT platform cannot offer a 1:1 mapping of the physical interfaces with internal logical interfaces (e.g., eth0, etc.).
The basic schema of the mapping, which may slightly vary according to the device used, is shown in this page:

	https://wiki.openwrt.org/doc/uci/network/switch


