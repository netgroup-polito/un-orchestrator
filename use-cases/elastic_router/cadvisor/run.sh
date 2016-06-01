#!/bin/sh
/usr/bin/cadvisor -logtostderr &
if [ -z ${NAME+x} ];
then cname=cadvproxy
else cname=$NAME
fi
if [ -z ${KEY+x} ];
then ckey=a
else ckey=$KEY
fi
python3 main.py $cname /keys/$ckey;