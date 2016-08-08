#! /bin/bash

#Author: Ivano Cerrato
#Date: June 16th 2014
#Brief: stop a NF

#command line: sudo ./stopNF.sh $1 $2

#$1 LSI ID				(e.g., 2)
#$2 NF name				(e.g., firewall)

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

file="$1_$2"
PID=`docker inspect --format '{{ .State.Pid }}' $file`

echo "[$0] Executing command: '"sudo docker kill `cat $file`"'"
sudo docker kill `cat $file`

echo "[$0] Executing command: '"sudo docker rm `cat $file`"'"
sudo docker rm `cat $file`

rm -f /var/run/netns/$PID

#docker kill returns 0 in case of success
retVal=`echo $?`

if [ $retVal -eq 0 ]
then
	echo "[$0] Container stopped"
else
	echo "[$0] An error occurred while trying to stop the container"
	exit 0
fi

rm $file $file.log

exit 1
