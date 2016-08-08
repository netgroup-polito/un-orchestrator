#! /bin/bash

# Traffic comes in from BNG over GRE tunnel(s) on eth1
# and goes out, NAT'ed, at eth0
# eth0 gets its IP automatically assigned and this address is
# then used as NAT source address.

#Set up GRE tunnel (from BNG)
TUN_LOCAL_IP="1.1.1.2"
TUN_REMOTE_IP="24.23.22.21"
TUN_REMOTE_MAC="52:54:00:ca:09:b7"

# Use inner tunnel IPs in the form of 192.168.(100+$i).250
TUN_INNER_IP_PREFIX="192.168"
TUN_INNER_IP_OFFSET=100
TUN_INNER_IP_END=250

ip addr add $TUN_LOCAL_IP dev eth1
ip route add $TUN_REMOTE_IP dev eth1

# Need to have resolution of TUN_REMOTE_IP in our ARP table:
arp -s $TUN_REMOTE_IP $TUN_REMOTE_MAC

for i in {1..6};
do
        tun_key=$((100*256 + $i))
        tun_inner_ip="${TUN_INNER_IP_PREFIX}.$((TUN_INNER_IP_OFFSET+i)).${TUN_INNER_IP_END}"
        tun_name="bng_gre_$i"
        ip tun add ${tun_name} mode gre local ${TUN_LOCAL_IP} remote ${TUN_REMOTE_IP} key ${tun_key} ttl 255
        ip link set ${tun_name} up
        ip addr add ${tun_inner_ip}/24 dev ${tun_name}
done

#Assign the ip addresses dynamically on the internet facing interface
cp /sbin/dhclient /usr/sbin/dhclient && /usr/sbin/dhclient eth0 -v

# Alternative static config:
# ip addr add 192.168.1.30/24 dev eth0
# ip route add default via 192.168.1.240
# arp -s 192.168.1.240 00:04:96:19:84:40


ifconfig

iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

sysctl net.ipv4.ip_forward=1
