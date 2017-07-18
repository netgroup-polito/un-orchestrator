#!/bin/bash

#Author: Ivano Cerrato
#Date: Oct 28th 2015
#Brief: activate a network interface and configure it.
#		If the interface is still not there, it loops 
#		untill the interface appears/

#command line: sudo ./activate_interface.sh $1

# $1: interace name

while :
do
	sudo ifconfig $1 up &> /dev/null
	ret=`echo $?`
	
	if [ $ret == 0 ]
	then
		break
	fi

	sleep 0.5
done

echo "[$0] Activatig interface $1"

sudo ethtool --offload $1 rx off tx off &> /dev/null
sudo ethtool -K $1 gso off &> /dev/null

# Remove the ip address from the interface(rofl
sudo ifconfig $1 0

# Disable the network manager on the interface
# The network manager must be disabled on all interfaces under the control of the UN

address=`ifconfig $1 | grep HWaddr  | awk {'print $5'}`

if [ `cat /etc/NetworkManager/NetworkManager.conf  | grep unmanaged-devices | wc -l` -eq 0 ]
then
	echo "[keyfile]" >> /etc/NetworkManager/NetworkManager.conf
	echo "unmanaged-devices=mac:$address" >> /etc/NetworkManager/NetworkManager.conf
else
	string=`cat /etc/NetworkManager/NetworkManager.conf | grep unmanaged-devices`
	new=`echo $string $address`
	ok=`echo ${new// /;}`
	sed "s/$string/$ok/g" /etc/NetworkManager/NetworkManager.conf > /etc/NetworkManager/un-tmp.conf
	mv /etc/NetworkManager/un-tmp.conf /etc/NetworkManager/NetworkManager.conf
fi

exit 0
