FROM      ubuntu
MAINTAINER Politecnico di Torino

RUN apt-get update && apt-get install -y dnsmasq ssh iptraf

#Prepare the ssh server

RUN mkdir -p /var/run/sshd && \
	mkdir -p /root/.ssh && \
	echo 'root:root' | chpasswd
	
RUN echo "UseDNS no" >> /etc/ssh/sshd_config
RUN sed '/PermitRootLogin without-password/d' /etc/ssh/sshd_config > tmp_file && \
	rm /etc/ssh/sshd_config && \
	mv tmp_file /etc/ssh/sshd_config
	
#Prepare the DHCP server

ADD default.conf /etc/dnsmasq.conf

ADD start_dhcp.sh start_dhcp.sh

RUN chmod +x start_dhcp.sh

CMD ./start_dhcp.sh
