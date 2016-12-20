#!/bin/bash



#Brief: pull a NF from a remote repository and run it. 

#command line: 
#	sudo ./retrieveImage.sh $1 $2 $3 $4

#1 NF name
#$2 url									
#$3 hash url	
#4 directory images							


archive_file=`echo "$1_$3"`

if (( $EUID != 0 )) 
then
	echo "[$0] This script must be executed with ROOT privileges"
	exit 0
fi
if [ ! -d "$4" ]; then
	mkdir $4
fi
if [ ! -f "$4/$archive_file" ]; then
	echo "[$0] Downloading NF image"
	wget $2 -O $4/$archive_file
else
	echo "[$0] The NF image is already available"
fi
#wget returns 0 in case of success

ret=`echo $?`

if [ $ret -eq 0 ]
then
	echo "[$0] Function '"$1"' retrieved"
else
	echo "[$0] Unable to retrieve function '"$1"'"
	sudo rm $4/$archive_file
	exit 0
fi


exit 1
