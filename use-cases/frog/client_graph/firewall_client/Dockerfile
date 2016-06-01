FROM      ubuntu
MAINTAINER Politecnico di Torino

RUN apt-get update
RUN apt-get install -y iptables bridge-utils

ADD start.sh start.sh
RUN chmod +x start.sh

CMD ./start.sh
