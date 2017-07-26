#! /bin/bash

# $port1 $port2 $namespace $path



SCRIPTPATH=$4

mkdir -p /etc/netns/$3/ipsec.d/run
mkdir -p /etc/netns/$3/strongswan
mkdir -p /etc/netns/$3/iproute2

cp -r $SCRIPTPATH/rt_tables /etc/netns/$3/iproute2

ip netns exec $3 ifconfig $1 192.168.1.1 netmask 255.255.255.0
#ip netns exec $3 route add default gw 130.192.225.254
ip netns exec $3 ifconfig $2 1.1.1.1 netmask 255.255.255.0
#ip netns exec $3 ip route add 10.2.0.0/16 dev $2 scope link table tapIPSEC
#ip netns exec $3 ip route add default via 10.2.2.252 dev $2 table tapIPSEC
#ip netns exec $3 ip rule add from 10.2.1.0/24 table tapIPSEC pref 2
#ip netns exec $3 ip rule add to 10.2.1.0/24 table tapIPSEC pref 2

cp -r /etc/strongswan* /etc/netns/$3/
cp -r $SCRIPTPATH/strongswan/ipsec.* /etc/netns/$3/
