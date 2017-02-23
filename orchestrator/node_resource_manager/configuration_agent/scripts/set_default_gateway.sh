#!/bin/bash

#$1 ip address for the default gateway

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

#check if a default gw already exists
route -n | grep UG
if [ $?==0 ]
then
    echo "route del default"
    route del default
fi

echo "route add default gw $1"
route add default gw $1

exit $?
