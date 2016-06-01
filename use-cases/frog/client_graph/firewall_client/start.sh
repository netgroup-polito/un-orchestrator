#! /bin/bash
sleep 5

brctl addbr br0
brctl addif br0 eth0
brctl addif br0 eth1
ifconfig br0 up

#To change the rules of the firewall, edit the next rows and/or add new rows
iptables -I FORWARD -m physdev --physdev-in eth0 --physdev-out eth1 -d 8.8.8.8 -j DROP

echo "Firewall started"

while true
do
	sleep 1
done

