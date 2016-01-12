#!/bin/bash

#Author: Gergely Pongracz (Unify)
#Date: 2015-11-02
#Brief: Create a logical switch

#$1 LSI id
#$2 controller ip
#$3 controller port

NAME=`echo LSI$1`
CTRL_HOST=$2
CTRL_PORT=$3

LSI_CTRL_PORT=`echo "16633 + $1" | bc`
ERFS_MAIN_PORT=16632
ERFS_HOST="localhost"

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 1
fi

#echo "Check whether ERFS is running"
n=`pgrep dof | wc -l`
if [ $n -eq 0 ]
then
    echo "[$0] ERFS is not running"
    exit 2
fi

echo "[$0] Creating $NAME"
echo "add-switch dpid=$1" | nc $ERFS_HOST $ERFS_MAIN_PORT
nohup socat tcp:$ERFS_HOST:$LSI_CTRL_PORT tcp:$CTRL_HOST:$CTRL_PORT &

exit 0

