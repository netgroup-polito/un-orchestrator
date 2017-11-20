#!/bin/bash

#Author: Federica Li Muti
#Date: 2017-08-18
#Brief: Create a namespace using "ip netns"

#$1 name of the namespace
#$2 name of the ppp interface
#$3 name of the interface3

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

echo [`date +%H:%M:%S:%N`]"[$0] Creating namespace '$1' and interfaces"

#The following instruction ensures that the namespace we are going to create does not exist yet.
ip netns del $1 &> /dev/null

ip netns add $1

t1=$(ifconfig | grep -o $2)
t2=$2

if [ "$t1" = "$t2" ]; then
  #echo "$2 up"
  inet=$(/sbin/ip -o -4 addr list $2 | awk '{print $4}' | cut -d/ -f1)
  peer=$(/sbin/ip -o -4 addr list $2 | awk '{print $6}' | cut -d/ -f1)

  ip link set $2 netns $1
  ip netns exec $1 ifconfig $2 up
  ip netns exec $1 ifconfig $2 $inet

  ip netns exec $1 ip route add default dev $2 
  ip netns exec $1 ip route add $peer dev $2

  echo [`date +%H:%M:%S:%N`]"[$0] namespace '$1' created, interface $2 and internal routing table configured"

  exit 1
else
  echo "[$0] $2 down"
  exit 0
fi
