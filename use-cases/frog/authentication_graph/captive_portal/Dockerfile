FROM      ubuntu
MAINTAINER Politecnico di Torino

# This file is based on the guide provided at:
# 	http://pietervogelaar.nl/ubuntu-12-04-install-jetty-9 

RUN apt-get update && apt-get install -y --no-install-recommends openjdk-7-jdk ssh build-essential python python-pip python-dev screen build-essential lsof tomcat7 authbind
RUN pip install --upgrade cython falcon requests gunicorn jsonschema
#Prepare the ssh server
RUN mkdir -p /var/run/sshd && \
	mkdir -p /root/.ssh && \
	echo 'root:root' | chpasswd
	
RUN echo "UseDNS no" >> /etc/ssh/sshd_config
RUN sed '/PermitRootLogin without-password/d' /etc/ssh/sshd_config > tmp_file && \
	rm /etc/ssh/sshd_config && \
	mv tmp_file /etc/ssh/sshd_config

#Prepare the captive portal
ADD server.xml /var/lib/tomcat7/conf/server.xml
ADD tomcat7.conf /etc/default/tomcat7
RUN sudo touch /etc/authbind/byport/80 && chmod 500 /etc/authbind/byport/80 && chown tomcat7 /etc/authbind/byport/80
RUN rm -rf /var/lib/tomcat7/webapps/*
ADD ROOT.war /var/lib/tomcat7/webapps/ROOT.war

#Add the takeMac server

ADD TakeMac /opt/TakeMac/

#Add the scripts to run the function
ADD start_cp.sh start_cp.sh

RUN chmod +x start_cp.sh

CMD ./start_cp.sh
