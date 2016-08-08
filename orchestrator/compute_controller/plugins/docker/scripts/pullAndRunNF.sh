#!/bin/bash

#Author: Ivano Cerrato
#Date: June 16th 2014
#Brief: pull a NF from a docker repository, and run it.

#command line: sudo ./pullAndRunNF.sh $1 $2 $3 $4 [$5 ...]

#$1 LSI ID				(e.g., 2)
#$2 NF name				(e.g., firewall)
#$3 registry/nf[:tag] 	(e.g., localhost:5000/pcap:latest)
#$4 number_of_ports		(e.g., 2)
#The next $4 *3 parameters are:
#	* the port name to be provided to the container (e.g., vEth0)
#	* the MAC address to be assigned to the port (e.g., aa:bb:cc:dd:ee:ff)
#	* the IP addres/netmask to be assigned to the port (e.g., 10.0.0.1/24)
#In case the MAC address and/or IP address must not be assigned to the port,
#their value is 0.
#The next parameter is a number indicating how many port forwarding must be setup. If not
#zero, the next N*2 elements are: TCP port of the host - TCP port in the container. Note that
#the request for port forwardings cause the creation of a further NIC connected to the docker0
#bridge
#The next parameter is a number indicated how many environment variable must be set up. If
#not zero, the next N elements are in the form "env_variable_name=value"

tmp_file="$1_$2_tmp"

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

#Check if some port forwarding must be set up
num_ports=$4
position_num_forwarding=`expr 4 + $num_ports \* 3 + 1`
num_forwarding=${!position_num_forwarding}

echo -ne "sudo docker run -d -i --name $1_$2 "   > $tmp_file

if [ $num_forwarding != 0 ]
then
	echo [`date`]"[$0] Port remapping required. A furter NIC ('eth0' in the container) is created and connected to 'docker0'"
	
	position_host_port=`expr $position_num_forwarding + 1`
	position_docker_port=`expr $position_num_forwarding + 2`
	
	for (( c=0; c<$num_forwarding; c++ ))
	do	
		host_port=${!position_host_port}
		docker_port=${!position_docker_port}
		
		echo [`date`]"[$0] Port remapping between host TCP port $host_port and VNF TCP port $docker_port"
		
		echo -ne "-p $host_port:$docker_port " >> $tmp_file
		
		position_host_port=`expr $position_host_port + 2`
		position_docker_port=`expr $position_docker_port + 2`
	done
	
	firstnicname=1
	lastnicname=`expr $4 + 1`
else
	# The NIC connected to the docker0 is not needed
	echo -ne "--net=\"none\" " >> $tmp_file
	
	firstnicname=0
	lastnicname=$4
fi

#Check if some environment variables myust be set up
position_num_env_var=`expr $position_num_forwarding + $num_forwarding \* 2 + 1`
num_env_var=${!position_num_env_var}
if [ $num_env_var != 0 ]
then
	echo [`date`]"[$0] Some ($num_env_var) environment variable must be set up"
	
	position_env_var=`expr $position_num_env_var + 1`
	for (( c=0; c<$num_env_var; c++ ))
	do
		variable=${!position_env_var}
		echo [`date`]"[$0]environment variable: $variable"
		position_env_var=`expr $position_env_var + 1`
		
		echo -ne "-e $variable " >> $tmp_file
	done
fi

echo "--privileged=true  $3 /bin/bash" >> $tmp_file

echo [`date`]"[$0] Executing command: '"`cat $tmp_file`"'"

ID=`bash $tmp_file`

#docker run returns 0 in case of success
ret=`echo $?`

if [ $ret -eq 0 ]
then
	echo [`date`]"[$0] Container $2 started with ID: '"$ID"'"
else
	echo [`date`]"[$0] An error occurred while starting the container"
	rm $tmp_file
	exit 0
fi

#Save the binding lsi-nf-docker id on a file
file="$1_$2"
echo $ID >> $file

rm $tmp_file

#The following code configures the network for the container just created.
#It is based on the description provided at
#	http://docs.docker.com/articles/networking/#container-networking

PID=`docker inspect --format '{{ .State.Pid }}' $ID`

sudo mkdir -p /var/run/netns
sudo ln -s /proc/$PID/ns/net /var/run/netns/$PID
	
current=5
current_mac=`expr $current + 1`
current_ip=`expr $current + 2`

for (( c=$firstnicname; c<$lastnicname; c++ ))
do	
	ip link set ${!current} netns $PID
	
	echo [`date`]"[$0] Inserting port ${!current} inside a container. It will have name eth$c"
	
	ip netns exec $PID ip link set dev ${!current} name eth$c
	ip netns exec $PID ip link set eth$c up
	
	if [ ${!current_mac} != 0 ]
	then
		echo [`date`]"[$0] Assigning MAC address '${!current_mac}'"
		ip netns exec $PID ifconfig eth$c hw ether ${!current_mac}
	fi
	
	if [ ${!current_ip} != 0 ]
	then
		echo [`date`]"[$0] Assigning IP configuration '${!current_ip}'"
		ip netns exec $PID ifconfig eth$c ${!current_ip}
	fi
	
#	ip netns exec $PID ifconfig eth$c
	
	current=`expr $current + 3`
	current_mac=`expr $current_mac + 3`
	current_ip=`expr $current_ip + 3`	
done

# Start the script "start.sh" inside the container
echo [`date`]"[$0] Executing command 'sudo docker exec $1_$2 ./start.sh &> $1_$2.log &'"
sudo docker exec $1_$2 ./start.sh &> $1_$2.log &
echo [`date`]"[$0] VNF log available in file '$1_$2.log'"

exit 1
