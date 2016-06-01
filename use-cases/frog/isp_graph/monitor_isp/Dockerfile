FROM ubuntu
MAINTAINER Politecnico di Torino

# Dockerfile based on this documentation:
#	http://git.openvswitch.org/cgi-bin/gitweb.cgi?p=openvswitch;a=blob_plain;f=INSTALL;hb=HEAD

RUN apt-get update && apt-get install -y iptraf python-software-properties software-properties-common ssh build-essential 

RUN add-apt-repository ppa:cavedon/ntop && apt-get update && apt-get install -y ntopng

#prepare the ssh server
RUN mkdir -p /var/run/sshd && \
	mkdir -p /root/.ssh && \
	echo 'root:root' | chpasswd
	
RUN echo "UseDNS no" >> /etc/ssh/sshd_config
RUN sed '/PermitRootLogin without-password/d' /etc/ssh/sshd_config > tmp_file && \
	rm /etc/ssh/sshd_config && \
	mv tmp_file /etc/ssh/sshd_config

#prepare ovs	
RUN wget http://openvswitch.org/releases/openvswitch-2.1.2.tar.gz && tar -xvf openvswitch-2.1.2.tar.gz  && rm openvswitch-2.1.2.tar.gz*

RUN cd openvswitch-2.1.2 && ./configure && make -j 4 && make install
RUN mkdir -p /usr/local/etc/openvswitch
RUN cd openvswitch-2.1.2 && ovsdb-tool create /usr/local/etc/openvswitch/conf.db vswitchd/vswitch.ovsschema

ADD configure_ovs.sh /
ADD start.sh /

RUN chmod +x configure_ovs.sh && chmod +x start.sh

CMD ./start.sh
