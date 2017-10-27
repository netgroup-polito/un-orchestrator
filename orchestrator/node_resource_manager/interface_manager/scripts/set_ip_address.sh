#!/bin/bash

#$1 name of the interface
#$2 configuration type
#$3 ip address (for static configuration)

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

case "$2" in
   "DHCP")
        echo "dhclient $1 -v"
        dhclient $1 -v
   ;;
   "static")
		echo "ifconfig $1 $3"
		ifconfig $1 $3
   ;;
esac

ip link set $1 up

exit 1
