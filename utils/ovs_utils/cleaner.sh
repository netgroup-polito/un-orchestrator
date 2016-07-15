#! /bin/bash
#REMOVE BRDIGES
sudo ovs-vsctl show | grep Bridge | awk {'print $2'} | cut -d '"' -f 2 | while read b ; do sudo ovs-vsctl del-br $b; done

#REMOVE DOCKER
sudo docker ps -a | awk {'print $1'} | while read d; do sudo docker rm -f $d; done

