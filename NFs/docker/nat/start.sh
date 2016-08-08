#! /bin/bash

#This gives time to the UN to configure the network namespace of the container
sleep 3

#Assign the ip addresses dinamically
cp /sbin/dhclient /usr/sbin/dhclient && /usr/sbin/dhclient eth0 -v && /usr/sbin/dhclient eth1 -v

ifconfig

iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE
