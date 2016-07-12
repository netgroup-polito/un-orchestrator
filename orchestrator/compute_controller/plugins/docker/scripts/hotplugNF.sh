#!/bin/bash

#Author: Roberto Bonafiglia
#Date: April 28th 2016
#Brief: update the ports for a Docker.

#command line: sudo ./pullAndRunNF.sh $1 $2 $3 $4 [$5 ...]

#$1 LSI ID                              (e.g., 2)
#$2 NF name                             (e.g., firewall)
#$3 last inserted interface		(for counting which eth will be)
#$4 number_of_ports             (e.g., 2)
#The next $5 *3 parameters are:
#       * the port name to be provided to the container (e.g., vEth0)
#       * the MAC address to be assigned to the port (e.g., aa:bb:cc:dd:ee:ff)
#       * the IP addres/netmask to be assigned to the port (e.g., 10.0.0.1/24)
#In case the MAC address and/or IP address must not be assigned to the port,
#their value is 0.

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

num_posrts=$4
PID=`sudo docker inspect --format='{{.State.Pid}}' $1_$2`
sudo mkdir -p /var/run/netns
sudo ln -s /proc/$PID/ns/net /var/run/netns/$PID

current=5
current_mac=`expr $current + 1`
current_ip=`expr $current + 2`
firstnicname=$3
lastnicname=`expr $firstnicname + $num_posrts`

for (( c=$firstnicname; c<$lastnicname; c++ ))
do
        ip link set ${!current} netns $PID

        echo [`date`]"[$0] Inserting port ${!current} inside a container. It will have name eth$c"

        ip netns exec $PID ip link set dev ${!current} name eth$c
        ip netns exec $PID ip link set eth$c up

        if [ ${!current_mac} != 0 ]
        then
                echo [`date`]"[$0] Assigning MAC address '${!current_mac}'"
                ip netns exec $PID ifconfig eth$c hw ether ${!current_mac}
        fi

        if [ ${!current_ip} != 0 ]
        then
                echo [`date`]"[$0] Assigning IP configuration '${!current_ip}'"
                ip netns exec $PID ifconfig eth$c ${!current_ip}
        fi

        ip netns exec $PID ifconfig eth$c

        current=`expr $current + 3`
        current_mac=`expr $current_mac + 3`
        current_ip=`expr $current_ip + 3`
done

exit 1
