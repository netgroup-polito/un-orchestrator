#!/bin/bash

#Author: Gergely Pongracz
#Date: 2015-11-5
#Brief: Add a Virtual Link between two bridges (using patch ports)

#$1 Source LSI id
#$2 Destination LSI id
#$3 Source LSI port id
#$4 Destination LSI port id

LSI_A_NAME=LSI$1
LSI_B_NAME=LSI$2
PORT_A=$3
PORT_B=$4

ERFS_MAIN_PORT=16632
ERFS_HOST="localhost"

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 1
fi

echo "Check whether ERFS is running"
n=`pgrep dof | wc -l`
if [ $n -eq 0 ]
then
    echo "ERFS is not running"
    exit 2
fi

echo "[$0] Connecting $LSI_A_NAME:$PORT_A with $LSI_B_NAME:$PORT_B"
echo "connect XSWITCH:$1-$3 XSWITCH:$2-$4" | nc $ERFS_HOST $ERFS_MAIN_PORT

exit 0
