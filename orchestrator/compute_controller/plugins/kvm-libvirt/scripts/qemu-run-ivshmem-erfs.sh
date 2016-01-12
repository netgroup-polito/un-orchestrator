#!/bin/bash

# Author: Gergely Pongracz
# Date: Nov 24th 2015

# $1 LSI ID
# $2 network function name
# $3 TCP port to be used for the monitor
# $4 disk path
# $5 port list (column separated)
# $6 ncores
# $7 coremask

LSI_ID=$1
NAME=$2
MONITOR=$3
VM_IMG=$4
PORTS=$5
NCORES=2
COREMASK=""

ERFS_MAIN_PORT=16632
ERFS_HOST="localhost"

QEMU_PATH="/home/sdn/VEE/dpdk-ovs/qemu/x86_64-softmmu/"
QEMU_BIN=$QEMU_PATH/qemu-system-x86_64

echo $0 $@

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 1
fi

if [ "$1" = "" ]; then echo "Parameter 1 is mandatory"; exit 2; fi

#
# Generating command line for Qemu
#
echo "Check whether ERFS is running"
n=`pgrep dof | wc -l`
if [ $n -eq 0 ]
then
    echo "ERFS is not running"
    exit 3
fi

COMMAND="group-ivshmems LSI$LSI_ID"
for i in `echo $PORTS | awk -F "," '{print $1,$2,$3,$4,$5,$6,$7}'`
do
    echo "NF port = $i"
    COMMAND="$COMMAND IVSHMEM:$LSI_ID-$i"
done
echo "COMMAND=$COMMAND"
echo $COMMAND | nc $ERFS_HOST $ERFS_MAIN_PORT

#
# Setting core mask (if given)
#
TASKSET=""
if [ "$COREMASK" != "" ]
then
    TASKSET="taskset -c $6 "
fi

#
# Starting Qemu
#
IVSHMEM_CMDLINE=`cat /tmp/ivshmem_qemu_cmdline_LSI$LSI_ID`

$TASKSET $QEMU_BIN -enable-kvm -name $NAME \
-machine pc-i440fx-1.6,accel=kvm,usb=off \
-cpu host \
-vnc :1$LSI_ID \
-monitor tcp:127.0.0.1:$MONITOR,server,nowait \
-smp cores=$NCORES,threads=1 -m 4096  \
-drive media=disk,format=raw,file=$VM_IMG \
-daemonize \
-netdev type=tap,id=net$LSI_ID,script=no,downscript=no,ifname=vnet$LSI_ID -device virtio-net-pci,netdev=net$LSI_ID,mac=fe:11:00:00:00:0$LSI_ID,csum=off,gso=off,guest_tso4=off,guest_tso6=off,guest_ecn=off \
$IVSHMEM_CMDLINE



#-object memory-backend-file,id=mem,size=4096M,mem-path=/mnt/huge,share=on -numa node,memdev=mem -mem-prealloc \
























