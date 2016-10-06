#!/bin/bash

#Author: Francesco Benforte
#Date: 2016-10-05
#Brief: Create tunnel interface using the command "ip link"

#$1 name of the port
#$2 local ip
#$3 remote ip
#$4 gre key

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

#The following instruction ensures that the interface we are going to create does not exist yet.
ip link delete $1 &> /dev/null

echo "[$0] Creating tunnel interface $1"

ip link add $1 type gretap local $2 remote $3 key $4

ip link set dev $1 up

echo "[$0] tunnel interface $1 created"

exit 1
