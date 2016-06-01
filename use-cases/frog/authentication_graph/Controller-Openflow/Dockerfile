FROM      ubuntu
MAINTAINER Politecnico di Torino

RUN apt-get update && apt-get install -y ssh python iptraf

#Prepare the ssh server
RUN mkdir -p /var/run/sshd && \
	mkdir -p /root/.ssh && \
	echo 'root:root' | chpasswd
	
RUN echo "UseDNS no" >> /etc/ssh/sshd_config
RUN sed '/PermitRootLogin without-password/d' /etc/ssh/sshd_config > tmp_file && \
	rm /etc/ssh/sshd_config && \
	mv tmp_file /etc/ssh/sshd_config
	
#prepare the ip forger openflow controller

ADD pox controller
ADD ext/* controller/ext/
ADD controller.conf controller/controller.conf
ADD client_mac.sh controller/client_mac.sh

ADD start.sh start.sh

RUN chmod +x start.sh
RUN chmod +x controller/client_mac.sh

CMD ./start.sh
