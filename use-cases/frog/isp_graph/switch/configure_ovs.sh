#!/bin/bash

start(){
	echo "ovs-vsctl show"
	ovs-vsctl show
	ovs-vsctl add-br br-swt2

	ovs-vsctl set bridge br-swt2 datapath_type=netdev

	for i in `ls /sys/class/net`
	do
		if [ $i != lo -a $i != 'ovs-system' ]
		then
		    ovs-vsctl add-port br-swt2 $i
		fi
	done
	ovs-vsctl show


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

