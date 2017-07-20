#!/bin/bash

#Dependencies : apt-get install libguestmount jq

if [ "$#" -ne 8 ]; then
	echo "USAGE: $0 CONFIG_SERVER CONFIG_USERNAME CONFIG_PASSWORD FUNCTIONAL_CAPABILITY TENANT_ID GRAPH_ID VNF_ID QCOW2_DISK_PATH"
	exit 1
fi

CONFIG_SERVER=$1
CONFIG_USER=$2
CONFIG_PASS=$3
FUNCTIONAL_CAPABILITY=$4
TENANT_ID=$5
GRAPH_ID=$6
VNF_ID=$7
IMAGE_PATH=$8

RET=0

wget $CONFIG_SERVER/config/files/$FUNCTIONAL_CAPABILITY -O /dev/null 2>&1 | grep "NOT FOUND" >> /dev/null
if [ $? -eq 0 ]; then
	#This VNF does not need datadisk
	echo "The VNF $VNF_ID ( Functional capability $FUNCTIONAL_CAPABILITY ) does not require metadata files"
	exit 1
fi

RANDOM_SUFFIX=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)

#guestunmount mntpoint$RANDOM_SUFFIX  2>&1 >> /dev/null
#rm -r mntpoint$RANDOM_SUFFIX 2>&1 >> /dev/null

mkdir -p mntpoint$RANDOM_SUFFIX
guestmount -a $IMAGE_PATH -i mntpoint$RANDOM_SUFFIX
if [ $? -ne 0 ]; then
	echo "Error during the mount of the images $IMAGE_PATH (VNF $VNF_ID)"
	guestunmount mntpoint$RANDOM_SUFFIX
	rm -r mntpoint$RANDOM_SUFFIX
	exit 2
fi

cd mntpoint$RANDOM_SUFFIX
mkdir -p metadata
cd metadata
wget $CONFIG_SERVER/config/files/$FUNCTIONAL_CAPABILITY -O files.json

if [ $? -eq 0 ]; then
	for file in $(jq -c ".[]" files.json); do
		FILENAME=$(echo $file | tr -d '"')

		wget $CONFIG_SERVER/config/files/$TENANT_ID/$GRAPH_ID/$VNF_ID/$FILENAME -O $FILENAME

		if [ $? -ne 0 ]; then
			echo "Error during the download of $FILENAME (VNF $VNF_ID)"
			RET=3
			break
		fi
	done
else
	echo "Error during the download of the file list for $FUNCTIONAL CAPABILITY (VNF $VNF_ID)"
	RET=4
fi

cd ..
cd ..
guestunmount mntpoint$RANDOM_SUFFIX
if [ $? -ne 0 ]; then
	echo "Error during the unmount of the datadisk $IMAGE_PATH (VNF $VNF_ID)"
	rm -r mntpoint$RANDOM_SUFFIX
	exit 5
fi

rm -r mntpoint$RANDOM_SUFFIX
exit $RET
