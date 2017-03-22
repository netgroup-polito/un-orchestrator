#!/bin/bash

#$1 LSI ID				(e.g., 2)
#$2 NF ID				(e.g., firewall)
#$3 NF name

#Check if script was execute with root privileges
if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

#Check number of parameters
if [ $# -lt 3 ];
then
	echo "[$0] Wrong number of parameters (lsiID nfID iomodulePath nfName uriType number_of_ports, port1, ..., portN)"
	exit 0
fi

ns_name=$1_$2_$3_ns
temp_dir="$1_$2_$3_tmp"

#Delete namespace
echo "[$0] Deleting network namespace $ns_name"
ip netns del $ns_name

#Delete tmp dir and file
echo "[$0] Deleting tmp dir $temp_dir"
rm -rf $temp_dir

exit 1
