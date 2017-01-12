#!/bin/bash

#Author: Ivano Cerrato
#Date: June 26th 2014
#Brief: pull a NF from a remote repository or a folder on this system, and run it.

#command line:
#	sudo ./nfs_manager/scripts/dpdk/pullAndRunNF.sh 2 example https://dl.dropboxusercontent.com/u/26069382/nf 2 2 2 2_example_1 2_example_2


#$1 LSI ID						(e.g., 2)
#$2 NF name						(e.g., firewall)
#$3 path file						(e.g., un-orchestrator/orchestrator/NFimages/archive.gz)
#$4 core mask					(e.g., 2) XXX: it must be in decimal. The conversion in exadecimal is done in this script
#$5 number of memory channels	(e.g., 2)
#$6 number_of_ports				(e.g., 2)
#The next $6 parameters are port names to be provided to the NF

tmp_file="$1_$2_tmp"

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

echo "" > $tmp_file
exec_name=$3

sudo chmod +x $exec_name

#prepare the command
echo -ne sudo ./$exec_name -c `echo "obase=16; $4" | bc` -n $5 --proc-type=secondary -- " " > $tmp_file

current=7
for (( c=0; c<$6; c++ ))
do
 	echo -ne --p ${!current} " ">> $tmp_file
	
	current=`expr $current + 1`
done

echo `echo --s $1_$2 --l $1_$2.log` >> $tmp_file

echo "[$0] Executing command: '"`cat $tmp_file`"'"

sudo bash $tmp_file &

sleep 1

#Save the binding lsi-nf-dpdk pid exec on a file
index=0
ps aux | grep "./$exec_name" | grep -v "grep" |  awk {'print $2'} | while read p
do
	if [ $index -eq 1 ]; then
		file="$1_$2"
		echo $p $exec_name >> $file
		break;
	else
		index=`expr $index + 1`
	fi
done

sudo rm $tmp_file

exit 1
