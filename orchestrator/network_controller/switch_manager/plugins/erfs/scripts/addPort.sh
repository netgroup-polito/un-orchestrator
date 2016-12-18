#!/bin/bash

#Author: Gergely Pongracz
#Date: 2015-11-04
#Brief: Add a port to a bridge

#$1 LSI id
#$2 port name or numa node (in case of ivshmem)
#$3 type
### physical: dpdk or host
### VNF: ivshmem (others are not yet supported)
### XSWITCH: xswitch
#$4 port id to assign
#$5 core ID to handle port (FIXME: add to C++ code)

# TODO: multiple rx-queues per port to be added

NAME=`echo LSI$1`
PORT_NAME=$2
PORT_TYPE=$3
PORT_ID=$4
LCORE=$5

ERFS_MAIN_PORT=16632
ERFS_HOST="localhost"

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 1
fi

#echo "[$0] Check whether ERFS is running"
n=`pgrep dof | wc -l`
if [ $n -eq 0 ]
then
    echo "[$0] ERROR: ERFS is not running"
    exit 2
fi

if [ "$LCORE" = "auto" ]
then
    MASK=`ps ax | grep dof | awk '{for (i=1; i<10; i++) if ($i=="-c") print $(i+1)}' | awk -F x '{print $2}'`
    if [ "$MASK" = "" ]
    then
        CORES=`ps ax | grep dof | awk '{for (i=1; i<10; i++) if ($i=="-c") print $(i+1)}'`
        echo "CORES = $CORES"
    else
        BINMASK=`echo "ibase=16; obase=2; ${MASK^^}" | bc`
        echo "MASK = $MASK, BINMASK = $BINMASK"
    fi
    LCORE=`expr $PORT_ID + 1` # FIXME
    echo "[$0] Selected core $LCORE automatically"
fi

echo "[$0] Adding port to $NAME (name=$PORT_NAME, type=$PORT_TYPE, id=$PORT_ID, core_id=$LCORE)"

type_cmd="type=$port_type"

if [ "$PORT_TYPE" = "dpdk" ]
then
    echo "Configuring physical port"
    echo "add-port dpid=$1 port-num=$PORT_ID $PORT_NAME" | nc $ERFS_HOST $ERFS_MAIN_PORT
    echo "lcore $LCORE $PORT_NAME" | nc $ERFS_HOST $ERFS_MAIN_PORT
else
    if [ "$PORT_TYPE" = "ivshmem" ]
    then
        echo "Configuring IVSHMEM port on socket $2"
        echo "add-port dpid=$1 port-num=$PORT_ID IVSHMEM socket=$2" | nc $ERFS_HOST $ERFS_MAIN_PORT
        echo "lcore $LCORE IVSHMEM:$1-$PORT_ID" | nc $ERFS_HOST $ERFS_MAIN_PORT
    else
        if [ "$PORT_TYPE" = "xswitch" ]
        then
            echo "Configuring XSWITCH port"
            echo "add-port dpid=$1 port-num=$PORT_ID XSWITCH" | nc $ERFS_HOST $ERFS_MAIN_PORT
        else
            echo "[$0] ERROR: Unknown port type $PORT_TYPE"
            exit 3
        fi
    fi
fi


exit 0
