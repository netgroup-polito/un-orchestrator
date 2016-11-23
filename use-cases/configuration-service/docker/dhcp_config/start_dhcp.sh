#! /bin/bash
#useful link:
#	http://www.cyberciti.biz/faq/howto-debian-ubutnu-set-default-gateway-ipaddress/

#start the DHCP server

ifconfig
cp /sbin/dhclient /usr/sbin/dhclient && /usr/sbin/dhclient eth0 -v
ifconfig

service dnsmasq start
echo "DHCP service started"

ifconfig

#start the SSH server
#service ssh start
#echo "ssh service started"

#Keep the container alive
#while true
#do
#	sleep 100
#done

