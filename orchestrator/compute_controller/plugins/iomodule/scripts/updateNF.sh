#!/bin/bash

#$1 LSI ID														(e.g., 2)
#$2 NF ID														(e.g., 0000000_1)
#$3 path/uri where iomodule archive is							(e.g., NFimages/archive.gz)
#$4 NF name														(e.g., switch)
#$5 URI type													(e.g., local/remote)
#$6 number_of_ports												(e.g., 2)
#The next $6 parameters are the names of the port of the NF	

#Check if script was execute with root privileges
if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

#Check number of parameters
if [ $# -lt 6 ];
then
	echo "[$0] Wrong number of parameters (lsiID nfID iomodulePath nfName uriType number_of_ports, port1, ..., portN)"
	exit 0
fi

#Get network interface
ns_name="$1_$2_$4_ns"
tmp=$(ip netns exec $ns_name ls /sys/class/net)

#Move interfaces in tmp ns
tmp_ns="tmp_ns"
ip netns add $tmp_ns

if=""
index=0
IFS=$' ' read -ra interfaces <<< "$tmp"
for ((i=0; i<${#interfaces[@]}; i++))
do
	if [ "${interfaces[$i]}" != "lo" ];
	then
		echo "${interfaces[$i]}"
		ip netns exec $ns_name ip link set ${interfaces[$i]} netns $tmp_ns
		if=$(echo "$if ${interfaces[$i]}")
		let "index+=1"
	fi
done

#Run stopNF and runNF
sh ./stopNF.sh $1 $2 $4 &>/dev/null &
ret=`echo $?`
if [ $ret -eq 1 ]
then
	echo "[$0] Successful extracted from archive"
	sh ./runNF.sh $1 $2 $3 $4 $5 $index $if &>/dev/null &
	ret=`echo $?`
	if [ $ret -eq 1 ]
	then
		echo "[$0] Nf $1 update"
		ip netns del $tmp_ns
		exit 1
	else
		echo "[$0] Unable to update NF $1"	
		ip netns del $tmp_ns
		exit 0
	fi
else
	echo "[$0] Unable to update NF $1"	
	ip netns del $tmp_ns
	exit 0
fi

exit 1
