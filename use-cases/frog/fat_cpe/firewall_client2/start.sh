#! /bin/bash

brctl addbr br0
brctl addif br0 eth0
brctl addif br0 eth1
ifconfig br0 up

#block packets to www.repubblica.it
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 213.92.16.171 -j DROP
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 213.92.16.191 -j DROP
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 213.92.16.101 -j DROP

echo "Firewall started"
