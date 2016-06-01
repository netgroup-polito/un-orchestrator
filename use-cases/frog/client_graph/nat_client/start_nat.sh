#! /bin/bash

#useful link:
#	http://www.cyberciti.biz/faq/howto-debian-ubutnu-set-default-gateway-ipaddress/

sleep 5

#Assign the ip address to wan port using the dhcp server
cp /sbin/dhclient /usr/sbin/dhclient && /usr/sbin/dhclient eth0 -v 
#Assign the ip address to user port 
ifconfig eth1 192.168.1.1/24

#start the SSH server
#service ssh start
#echo "ssh service started"

iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

ifconfig

#Keep the container alive
while true
do
	sleep 100
done

