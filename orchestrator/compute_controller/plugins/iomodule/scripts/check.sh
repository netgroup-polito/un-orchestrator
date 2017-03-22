#!/bin/bash

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

#Check if golang installed
go version &> /dev/null
retval=$?
if [ $retval -ne 0 ]; then
    echo "[$0] golang needs in order to use iomodule"
	exit 0
fi

#Get GOPATH 
usr=$(printenv SUDO_USER)
tmp=$(sudo -Hiu $usr env | grep "GOPATH")
IFS='=' read -a myarray <<< "$tmp"
GOPATH=${myarray[1]}

#Check hover and iovisorovn installed
HOVERPATH="$GOPATH/bin/hoverd"
IOVISORPATH="$GOPATH/bin/iovisorovnd"

if [ ! -f $HOVERPATH ]
then
	echo "[$0] Hover is not installed"
	exit 0
fi

if [ ! -f $IOVISORPATH ]
then
	echo "[$0] Iovisorovn is not installed"
	exit 0
fi

exit 1
