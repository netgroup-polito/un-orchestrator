#!/bin/bash

#$1 name of the interface
#$2 mac address

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

echo "ifconfig $1 hw ether $2"
ifconfig $1 hw ether $2

exit 1
