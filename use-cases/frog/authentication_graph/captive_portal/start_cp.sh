#! /bin/bash

#wait for interfaces initialization
sleep 4

#Set the address of the captive portal

ifconfig  eth0 192.168.4.3/24 up
route add default gw 192.168.4.254

#start the takeMac service
cd /opt/TakeMac

gunicorn -b 192.168.4.3:81 main:app -t 100 &

#Start the captive portal
service tomcat7 start
echo "Captive portal started"


#Keep the container alive
while true
do
	sleep 100
done

