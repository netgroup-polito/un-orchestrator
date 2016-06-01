#!/bin/bash

start(){
	ovs-vsctl add-br br-auth
#	ifconfig br0 up

	ovs-vsctl set bridge br-auth datapath_type=netdev
	ovs-vsctl set Bridge br-auth protocol=OpenFlow10
	ovs-vsctl set-controller br-auth tcp:192.168.4.4:6633
	for i in `ls /sys/class/net`
	do
		if [ $i != lo -a $i != 'ovs-system' ]; then
		    ovs-vsctl add-port br-auth $i
		fi
	done

	ovs-vsctl show

	ifconfig br-auth 192.168.4.5/24
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

