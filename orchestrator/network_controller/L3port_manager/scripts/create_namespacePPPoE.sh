#!/bin/bash

#Author: Federica Li Muti
#Date: 2017-08-18
#Brief: Create a namespace using "ip netns"

#$1 name of the namespace
#$2 name of interface2 into the namespace (to ppp)
#$3 name of the interface3

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

echo "[$0] Creating namespace '$1' and interfaces"

#The following instruction ensures that the namespace we are going to create does not exist yet.
ip netns del $1 &> /dev/null

ip netns add $1


ip link add $2 type veth peer name $3

ifconfig $3 up

ip link set $2 netns $1
ip netns exec $1 ifconfig $2 up

#aggiungere alra interfaccia nel namespace quando si crea il grafo con collegamento vero ppp0

echo "[$0] namespace '$1' and interfaces created"

exit 1
