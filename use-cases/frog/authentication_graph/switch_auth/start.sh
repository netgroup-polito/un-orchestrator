#! /bin/bash
sleep 5

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

sleep 5

./configure_ovs.sh start


while true
do
	sleep 1
	for i in `ls /sys/class/net`
	do
                if [ $i != lo -a $i != 'ovs-system' -a $i != 'br-auth' -a $i != 'ovs-netdev' ]; then
                    ovs-vsctl --may-exist add-port br-auth $i
                fi
        done
done
