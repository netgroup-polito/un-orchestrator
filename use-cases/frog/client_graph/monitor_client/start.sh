#! /bin/bash

sleep 5

#start the ssh server
#service ssh start
#echo "ssh service started"

#eth0 is used for ssh and ntop
#ifconfig eth0 192.168.1.10/24

#start ovs

#FIXME: put the two next commands in the Dockerfile?
if [ ! -d /dev/net ]; then
    su -c "mkdir -p /dev/net"
fi

if [ ! -c /dev/net/tun ]; then
    su -c "mknod /dev/net/tun c 10 200"
fi

ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock \
	--remote=db:Open_vSwitch,Open_vSwitch,manager_options --pidfile --detach

ovs-vsctl --no-wait init
ovs-vswitchd --pidfile --detach

./configure_ovs.sh start

echo "Starting NTOP"
#ntop --set-admin-password
sudo service redis-server start
sudo service ntopng start

#/sbin/dhclient /usr/sbin/dhclient && /usr/sbin/dhclient eth0 -v

ifconfig

#keep alive the container
while true
do
	sleep 100
done
