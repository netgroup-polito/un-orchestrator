FROM      ubuntu
MAINTAINER Politecnico di Torino

RUN apt-get update && apt-get install -y iptables

ADD sysctl.conf /etc/sysctl.conf
ADD ./start.sh start.sh

RUN chmod +x start.sh
