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

exit 0
