FROM ubuntu:latest

MAINTAINER Umar Toseef umar.toseef@eict.de
RUN apt-get update

RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python3-pip
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python3-pkg-resources
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python3-setuptools
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python3-zmq
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python3-urwid
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python3-tornado
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install libffi-dev
RUN pip3 install pynacl
RUN pip3 install jsonrpcserver
RUN pip3 install jsonrpcclient

# Grab cadvisor from the staging directory.
ADD cadvisor /usr/bin/cadvisor
EXPOSE 8080

# start in /proxy
WORKDIR /proxy

ADD cadvproxy /proxy/cadvproxy
ADD doubledecker /proxy/doubledecker
COPY main.py /proxy
COPY run.sh /proxy
CMD ./run.sh