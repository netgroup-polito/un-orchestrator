#!/bin/bash
while : ; do
    [[ -f "./mac" ]] && break
    wget 192.168.4.3:81/mac
    sleep 1
done

mac=`wget -S -O - 192.168.4.3:81/mac | grep : | awk '{print $3}'`
cd /controller
cat controller.conf | sed s/16:f0:9b:3e:44:c4/$mac/ > controller_new.conf
mv controller_new.conf controller.conf
