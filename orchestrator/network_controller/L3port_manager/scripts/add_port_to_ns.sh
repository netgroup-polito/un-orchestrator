#!/bin/bash

#Author: Federica Li Muti
#Date: 2017-09-21
#Brief: Add an interface to the namespace using "ip link"

#$1 name of the namespace
#$2 name of interface that have to be attached into the namespace

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

echo "[$0] Attaching interface '$2' to the '$1' namespace"

ip link set $2 netns $1
ip netns exec $1 ifconfig $2 up

echo "[$0] interface '$2' attached to namespace '$1'"

exit 1