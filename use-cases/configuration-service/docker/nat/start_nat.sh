#! /bin/bash
#Assign the ip addresses dinamically
ifconfig
cp /sbin/dhclient /usr/sbin/dhclient && /usr/sbin/dhclient eth0 -v
ifconfig

iptables -t nat -A POSTROUTING -o eth2 -j MASQUERADE

#while true
#do
#	sleep 1
#done
