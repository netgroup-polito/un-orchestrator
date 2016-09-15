#! /bin/bash


echo "Control app container started"

#echo "start ssh"
#service ssh start

echo "start ryu"
cd ryu_app/
ryu-manager ctrl_app_er_un_v5.py



