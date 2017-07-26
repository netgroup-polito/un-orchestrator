FROM      ubuntu
MAINTAINER Politecnico di Torino

#########################################################################
#									#
#			VNF feature					#
#									#
#########################################################################

RUN apt-get update && apt-get install -y iptables
ADD sysctl.conf /etc/sysctl.conf

#########################################################################
#									#
#			Configuration Agent				#
#									#
#########################################################################
ENV TERM xterm
RUN apt-get install -y htop net-tools nano vim iputils-ping #Tools for debug and management, xterm variable required in order to run nano

RUN apt-get install -y python3-setuptools python3-pip libffi-dev git sudo

RUN git clone https://github.com/pyca/pynacl.git
WORKDIR pynacl
RUN python3 setup.py install

WORKDIR /
RUN pip3 install aiozmq  
RUN pip3 install zmq
RUN pip3 install pyang
RUN apt-get install -y xsltproc
RUN apt-get install -y jing
RUN pip3 install netifaces
RUN pip3 install xmltodict
RUN pip3 install --upgrade python-iptables
RUN pip3 install netaddr
RUN pip3 install iptools

RUN pip3 install PyNaCl
RUN pip3 install pyzmq
RUN pip3 install tornado
RUN pip3 install cffi
RUN pip3 install urwid

RUN git clone https://github.com/Acreo/DoubleDecker-py.git
WORKDIR DoubleDecker-py
RUN python3 setup.py install

RUN mkdir /etc/doubledecker
ADD a-keys.json /etc/doubledecker/a-keys.json
ADD b-keys.json /etc/doubledecker/b-keys.json
ADD public-keys.json /etc/doubledecker/public-keys.json

WORKDIR /
ADD ./start_nat_agent.sh start_nat_agent.sh
RUN chmod +x start_nat_agent.sh
ADD ./configuration_agent configuration_agent

RUN echo "configuration_interface='eth0'" > ./configuration_agent/nat_config/constants.py
RUN echo "# Configuration Agent - configuration file \ndealer = 'tcp://10.0.0.2:5555' \nkeyfile = '/etc/doubledecker/a-keys.json' " > ./configuration_agent/constants.py

#########################
#
# DATADISK EMULATION
#
#########################

RUN mkdir datadisk
ADD ./metadata datadisk/metadata
RUN chmod 777 datadisk/metadata

#########################################################################
#									#
#			Script boot					#
#									#
#########################################################################

WORKDIR /
ADD ./start_nat.sh start_nat.sh
RUN chmod 777 start_nat.sh
ADD start.sh start.sh
RUN chmod 777 start.sh
