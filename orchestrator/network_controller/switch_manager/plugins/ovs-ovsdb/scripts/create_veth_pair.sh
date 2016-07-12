#!/bin/bash

#Author: UNIFY Consortium
#Date: 2016-01-18
#Brief: Create a veth pair

#$1 name of the first end
#$2 name of the second end

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

#The following instruction ensures that the interface we are going to create does not exist yet.
#This in fact may happen when the VNF is removed and then immediately created again. This is probably
#a bug in Docker (?), as illustrated here https://github.com/docker/docker/issues/6576 .
ip link delete $2 &> /dev/null

echo "[$0] Creating the veth pair $1 - $2"

ip link add $1 type veth peer name $2

echo "[$0] Veth pair $1 - $2 created"

ip link set $1 up
ip link set $2 up

exit 1
