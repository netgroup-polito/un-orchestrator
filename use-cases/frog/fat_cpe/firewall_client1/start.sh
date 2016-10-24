#! /bin/bash

brctl addbr br0
brctl addif br0 eth0
brctl addif br0 eth1
ifconfig br0 up

#block packets to www.lastampa.it
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 107.154.127.10 -j DROP
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 107.154.113.10 -j DROP
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 107.154.115.10 -j DROP
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 107.154.118.10 -j DROP

echo "Firewall started"
