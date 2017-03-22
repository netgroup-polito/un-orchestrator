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

#Some variables
temp_dir="$1_$2_$4_tmp"
path=$3
uri_type=$5
number_of_ports=$6

hoverLogFile=hoverLog$2
iovisorLogFile=iovisorLog$2
yamlfile=$1_$2_$4.yaml

#Get GOPATH 
usr=$(printenv SUDO_USER)
gopathTmp=$(sudo -Hiu $usr env | grep "GOPATH")
IFS='=' read -a myarray <<< "$gopathTmp"
GOPATH=${myarray[1]}
HOVERPATH="$GOPATH/bin/hoverd"
IOVISORPATH="$GOPATH/bin/iovisorovnd"


#Create tmp dir
mkdir $temp_dir &> /dev/null
chown -R $usr $temp_dir &> /dev/null
chmod +r $temp_dir &> /dev/null

#Iomodule archive contains
#	1)description file(module, link, ext_if)
#	2)(optional)additional commands for setup

#Extract the archive in a temporary directory
tar -xzf $3 -C $temp_dir &> /dev/null
ret=`echo $?`

if [ $ret -eq 0 ]
then
	echo "[$0] Successful extracted from archive"
else
	echo "[$0] Unable to extract iomodule function '"$3"' from the archive"
	
	rm $3
	rm -rf $temp_dir
	
	exit 0
fi

#Check if description file exists
description="$temp_dir/nf_description.yaml"
if [ ! -f $description ] 
then
	echo "[$0] Description file not found in archive $3!"
	exit 0
fi	

#Check if optional files exist
command_found=false
command_file="$temp_dir/extraCommands.sh"
if [ -f $command_file ]
then
	#echo "[$0] Command script file found in archive $3!"
	command_found=true
fi	
chown -R $usr $command_file &> /dev/null
chmod +x $command_file &> /dev/null

#Configure network namespaces and add veths, created by UN, to ns
#These veths link iomodolues and nth lsi
#Unique network namespace name
ns_name=$1_$2_$4_ns
ip netns add $ns_name

#IMPORTANT! Activate loopback if in the new network namespace
sudo ip netns exec $ns_name ip link set dev lo up

#Prepare ext_if
port=7
index=0
port_names=""
for ((i=0; i<$number_of_ports; i++))
do	
	ip link set ${!port} netns $ns_name
	ip netns exec $ns_name ip link set dev ${!port} name eth$index
	ip netns exec $ns_name ip link set dev eth$index up
	#ip netns exec $ns_name ip link set dev eth$index promisc on
	#ip netns exec $ns_name ip link set dev eth$index arp on
	
	port_names[$index]=$(echo "eth$index")	
	let "index+=1"
	let "port+=1"
	
done
#echo ${port_names[*]}

#Run extraCommands script
if [ "$command_found" = true ] ;
then		
	args=""	
	#Prepare args to run script
	for ((i=0; i<$number_of_ports; i++))
	do
		args="$args ${port_names[$i]}"
	done	
	echo "[$0] Run command script $command_file $number_of_ports $args"
fi

#Generate yaml file
IFS=$'\n'
modules=($(cat "$temp_dir/nf_description.yaml"))
index1=0
index2=0
for (( i=0; i<${#modules[@]}; i+=1 ))
do
	#Get modules name
	if [[ ${modules[$i]} == *"name:"* && ${modules[$i+1]} == *"type:"* ]];
	then
		IFS=":" read -ra old_module_name <<< "${modules[$i]}"
		for ((j=1; j<${#old_module_name[@]}; j++))
		do
    		#Remove whitespace
    		clean_module_name=$(echo "${old_module_name[$j]//[[:blank:]]/}")
    		#Check if this name has already been saved
    		if [[ ! "${new_module_names[@]}" =~ "$clean_module_name" ]];
    		then
    			#Save old module name
    			old_module_names[$index1]=$clean_module_name
    			#Save new module name
    			new_module_names[$index1]=$(echo "$1_$clean_module_name")
    			let index1=$index1+1
    			
    			#echo ${new_module_names[$index1]}
    			#echo "Array length:${#new_module_names[@]}"
    			#echo "Index:$index1"
    		fi
		done
	fi
	
	#Get external interfaces name
	if [[ ${modules[$i]} == *"iface:"* ]];
	then
		IFS=":" read -ra old_iface_name <<< "${modules[$i]}"
		for ((j=1; j<${#old_iface_name[@]}; j++))
		do
    		#Remove whitespace
    		clean_iface_name=$(echo "${old_iface_name[$j]//[[:blank:]]/}")
    		#Check if this name has already been saved 
    		if [[ ! "${old_iface_names[@]}" =~ "$clean_iface_name" ]];
    		then
    			#Save old iface name
    			old_iface_names[$index2]=$clean_iface_name
    			let index2=$index2+1
    			
    			#echo ${old_iface_names[$index2]}
    			#echo "Array length:${old_iface_names[@]}"
    			#echo "Index:$index2"
    		fi
		done
	fi
	
	echo ${modules[$i]} >> $yamlfile
done
#cat $yamlfile
#echo ${old_module_names[*]}
#echo ${new_module_names[*]}
#echo ${old_iface_names[*]}

chown $usr $yamlfile &> /dev/null 
chmod +r $yamlfile &> /dev/null 


#Replace value in tmp_file
for ((k=0; k<${#old_module_names[@]}; k++ ))
do
 	sed -i "s/${old_module_names[$k]}/${new_module_names[$k]}/g" $yamlfile
 	#echo "sed -i "s/${old_module_names[$k]}/${new_module_names[$k]}/g" $yamlfile"
done

for ((l=0; l<${#old_iface_names[@]}; l++))
do
	sed -i "s/${old_iface_names[$l]}/${port_names[$l]}/g" $yamlfile
	#echo "sed -i "s/${old_iface_names[$l]}/${port_names[$l]}/g" $yamlfile"
done
#cat $yamlfile

#Clear log file 
>$hoverLogFile
chown $usr $hoverLogFile &> /dev/null 
chmod +r $hoverLogFile &> /dev/null 
>$iovisorLogFile
chown $usr $iovisorLogFile &> /dev/null 
chmod +r $iovisorLogFile &> /dev/null 

mv $yamlfile ./$temp_dir
mv $hoverLogFile ./$temp_dir
mv $iovisorLogFile ./$temp_dir


hoverPort=5002
let hoverPort=$hoverPort+$2
#Start hover and iovisor in the created network namespace
#echo "sudo ip netns exec $ns_name $HOVERPATH -listen 127.0.0.1:$hoverPort 1>>./$temp_dir/$hoverLogFile 2>>./$temp_dir/$hoverLogFile &"
ip netns exec $ns_name $HOVERPATH -listen 127.0.0.1:$hoverPort 1>>./$temp_dir/$hoverLogFile 2>>./$temp_dir/$hoverLogFile &

#Wait hover get up otherwise operations fail
echo "[$0] Start Hover..."
sleep 10
echo "[$0] Hover is up!"

echo "[$0] Start iovisorovn..."
#echo "sudo ip netns exec $ns_name $IOVISORPATH -file $yamlfile -hover http://127.0.0.1:$hoverPort 1>>./$temp_dir/$iovisorLogFile 2>>./$temp_dir/$iovisorLogFile &"
ip netns exec $ns_name $IOVISORPATH -file ./$temp_dir/$yamlfile -hover http://127.0.0.1:$hoverPort 1>>./$temp_dir/$iovisorLogFile 2>>./$temp_dir/$iovisorLogFile &
echo "[$0] Iovisorovn is up!"


exit 1
