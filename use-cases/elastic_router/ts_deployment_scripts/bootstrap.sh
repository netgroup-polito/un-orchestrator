#!/usr/bin/env bash

username=$1
password=$2

echo u:$username,p

sed -ie 's/main/main contrib/' /etc/apt/sources.list
apt-get update
apt-get install -y virtualbox-guest-dkms virtualbox-guest-utils virtualbox-guest-x11
apt-get install -y xfce4 lightdm
cp /vagrant/files/lightdm.conf /etc/lightdm

#
# Epoxide
#
apt-get install -y emacs git graphviz
git clone --depth=1 https://github.com/nemethf/epoxide
mkdir -p /home/vagrant/.emacs.d
cp /vagrant/files/init.el /home/vagrant/.emacs.d

#
# AutoTPG
#
apt-get install -y apt-transport-https ca-certificates
apt-key adv --keyserver hkp://p80.pool.sks-keyservers.net:80 --recv-keys 58118E89F3A912897C070ADBF76221572C52609D
cat >/etc/apt/sources.list.d/docker.list <<EOF
deb https://apt.dockerproject.org/repo debian-jessie main
EOF
apt-get update
apt-get install -y --force-yes docker-engine
adduser vagrant docker
if false; then
  # The plan was to pull the build image from a hub, but instead we
  # build the image with the following commands.
  mkdir -p /home/vagrant/AutoTPG
  cd /home/vagrant/AutoTPG
  cat >Dockerfile <<EOF
FROM      ubuntu
MAINTAINER iMinds

RUN apt-get update
RUN apt-get install -y build-essential default-jdk ant python-dev wget unzip
RUN wget http://users.intec.ugent.be/unify/autoTPG-CONTROLLER.zip

RUN unzip autoTPG-CONTROLLER.zip
WORKDIR floodlight-plus/
RUN make

EXPOSE 8080
EXPOSE 6633

CMD ./floodlight.sh
EOF
  docker build -t autotpg .
else
  docker login -u $username -p $password gitlab.testbed.se:5000
  docker pull gitlab.testbed.se:5000/autotpg:latest
fi
  
#
# RQE
#

docker login -u $username -p $password gitlab.testbed.se:5000
docker pull gitlab.testbed.se:5000/rqe:latest

#
# Double Decker
#
docker pull gitlab.testbed.se:5000/doubledecker:latest

#
# Housekeeping
#
apt-get clean
/etc/init.d/lightdm restart
