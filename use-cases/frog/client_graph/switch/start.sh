#! /bin/bash

sleep 5

#start the ssh server

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

#keep alive the container
while true
do
	sleep 100
done
