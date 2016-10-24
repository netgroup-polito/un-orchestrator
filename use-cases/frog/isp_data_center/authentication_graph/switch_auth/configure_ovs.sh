#!/bin/bash

start(){
	echo "Executing command 'ovs-vsctl add-br br-auth -- set bridge br-auth datapath_type=netdev'"
	ovs-vsctl add-br br-auth -- set bridge br-auth datapath_type=netdev

	echo "Executing 'ovs-vsctl set-controller br-auth tcp:192.168.4.4:6633'"
	ovs-vsctl set-controller br-auth tcp:192.168.4.4:6633

	for i in `ls /sys/class/net`
	do
		if [ $i != lo -a $i != 'ovs-system' -a $i != 'br-auth' -a $i != 'ovs-netdev' ]; then
			echo "Executing 'ovs-vsctl add-port br-auth $i'"
		    ovs-vsctl add-port br-auth $i
		fi
	done
	echo "Executing 'ovs-vsctl show'"
	ovs-vsctl show

	echo "Executing command 'ifconfig br-auth 192.168.4.5/24'"
	ifconfig br-auth 192.168.4.5/24
	echo "Executing comand 'ifconfig br-auth up'"
 	ifconfig br-auth up
}

case $1 in
    start)
        start
    ;;
    stop)
    ;;
    *)
        echo "Usage: $0 {start}"
        exit 2
    ;;
esac

