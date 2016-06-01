FROM      ubuntu
MAINTAINER Politecnico di Torino

RUN apt-get update && apt-get install -y ssh iptraf iptables

#Prepare the ssh server

RUN mkdir -p /var/run/sshd && \
	mkdir -p /root/.ssh && \
	echo 'root:root' | chpasswd
	
RUN echo "UseDNS no" >> /etc/ssh/sshd_config
RUN sed '/PermitRootLogin without-password/d' /etc/ssh/sshd_config > tmp_file && \
	rm /etc/ssh/sshd_config && \
	mv tmp_file /etc/ssh/sshd_config
	
#Prepare the DHCP server
ADD sysctl.conf /etc/sysctl.conf
ADD ./start_nat.sh start_nat.sh

RUN chmod +x start_nat.sh

CMD ./start_nat.sh
