#!/bin/bash

if [ $# -ne 1 ] ; then
	echo "USAGE: $0 mac-address"
	echo ""
	echo "This script return the name of the VM that own the specified mac-address"
	exit 2
fi

MAC=$1

[[ "$MAC" =~ ^([a-fA-F0-9]{2}:){5}[a-fA-F0-9]{2}$ ]]

if [ $? -ne 0 ]; then
	echo "'$MAC' is not a valid mac-address"
	exit 3
fi

RET=1
for l in $(virsh list --all | awk '{print $2}' | grep "_"); do
	virsh dumpxml $l | grep $1 >> /dev/null
	if [ $? -eq 0 ]; then
		echo "The VM '$l' owns the mac-address $1"
		RET=0
	fi
done

exit $RET
