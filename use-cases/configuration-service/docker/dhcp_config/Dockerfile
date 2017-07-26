FROM      ubuntu
MAINTAINER Politecnico di Torino

#########################################################################
#									#
#			VNF feature					#
#									#
#########################################################################


RUN apt-get update && apt-get install -y dnsmasq ssh iptraf

#Prepare the ssh server

RUN mkdir -p /var/run/sshd && mkdir -p /root/.ssh && echo 'root:root' | chpasswd
	
RUN echo "UseDNS no" >> /etc/ssh/sshd_config
RUN sed '/PermitRootLogin without-password/d' /etc/ssh/sshd_config > tmp_file && rm /etc/ssh/sshd_config && mv tmp_file /etc/ssh/sshd_config

RUN apt-get install -y isc-dhcp-server
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
ADD ./start_dhcp_agent.sh start_dhcp_agent.sh
RUN chmod +x start_dhcp_agent.sh
ADD ./configuration_agent configuration_agent

RUN echo "configuration_interface='eth0'" > ./configuration_agent/dhcp_server_config/constants.py
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
ADD ./start_dhcp.sh start_dhcp.sh
RUN chmod +x start_dhcp.sh
ADD ./start.sh start.sh
RUN chmod +x ./start.sh
