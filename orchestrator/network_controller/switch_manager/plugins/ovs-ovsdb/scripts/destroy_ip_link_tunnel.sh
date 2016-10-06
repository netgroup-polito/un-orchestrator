#!/bin/bash

#Author: Francesco Benforte
#Date: 2016-10-05
#Brief: Destroy tunnel interface using the command "ip link"

#$1 name of the port

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

echo "[$0] Destroying tunnel interface $1"

ip link delete $1 &> /dev/null

exit 1
