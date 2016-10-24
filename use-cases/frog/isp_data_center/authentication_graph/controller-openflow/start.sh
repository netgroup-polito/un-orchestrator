#! /bin/bash

ifconfig eth0 192.168.4.4/24
netstat

cd /controller
./client_mac.sh
netstat

#start the IP forger
cd /controller
./pox.py IP_forger
