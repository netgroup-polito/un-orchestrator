#!/bin/bash

#Author: Sergio Nuccio
#Date: October 21st 2015
#Brief: pull a native NF from a remote repository or a folder on this system, and run it. 

#command line: 
#	sudo ./nfs_manager/scripts/native/pullAndRunNativeNF.sh $1 $2 $3 [$4 ...]

#$1 LSI ID										(e.g., 2)
#$2 NF name										(e.g., firewall)
#$3 number_of_ports									(e.g., 2)
#The next $4 parameters are the names of the port of the NF				(e.g., vEth0 vEth1)


if (( $EUID != 0 )) 
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi



tmp_file="$1_$2_tmp"

echo "" > $tmp_file


temp_dir=`echo $1"_"$tmp_file"_"$2`


#check if the update script/executable exists

update="$temp_dir/update"

if [ ! -f $update ]
then
	echo "[$0] Update script for native function '"$2"' not found!"
	exit 0
fi	

ns_name=$1_$2_ns


#configure network for native network function
current=4
for (( c=0; c<$3; c++ ))
do

	#create virtual interface connected to the port of the vswitch (already created)
	#name of the ovs port on the switch: <lsi_id>_<nf_name>_<i> => ${1}_${2}_$((c+1))
	#!not needed anymore
 	#ip link add link ${1}_${2}_$((c+1)) name ${!current} type macvtap

	ip link set ${!current} netns $ns_name
	
	ip netns exec $ns_name ip link set ${!current} up
	ip netns exec $ns_name ip link set ${!current} promisc on
	ip netns exec $ns_name ip link set ${!current} arp on
	
	current=`expr $current + 1`
	
done 

#prepare the command

sudo chmod +x $update

echo -ne sudo ip netns exec $ns_name ./$update $1 $2 $3 > $tmp_file

#passing port names to the script
current=4
for((c=0; c<$3; c++))
do
	echo -ne " " ${!current} >> $tmp_file
	current=`expr $current + 1`
done

#command:= <script_name> <LSI_ID> <NF_NAME> <#PORTS> <PORT_NAMES ...>

echo [`date`]"[$0] Executing command: '"`cat $tmp_file`"'"

bash $tmp_file

rm $tmp_file


exit 1
