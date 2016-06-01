FROM ubuntu:16.04

MAINTAINER Bertrand Pechenot <berpec@acreo.se>
# based on petergrace/opentsdb 
# Thanks to Peter Grace (original author)
# Thanks to Pontus Sköldström and Tamas Levai

#Install packages
RUN DEBIAN_FRONTEND=noninteractive apt-get -y update \
&& DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends \
                                                  -y \
                                                     make \
                                                     curl \
                                                     autoconf \
                                                     openjdk-8-jdk \
                                                     supervisor \
                                                     openssh-server \
                                                     automake \
                                                     gnuplot-nox \
                                                     unzip \
                                                     python3 \
                                                     python3-zmq \
                                                     python3-requests \
						     python3-setuptools \ 
                                                     python3-pip \
                                                     libffi-dev \
                                                     libsodium-dev \
                                                     python3-dev \
                                                     build-essential \
&&  DEBIAN_FRONTEND=noninteractive apt-get clean all

#Install HBase and scripts
RUN mkdir -p /opt/sei-bin/ 
RUN mkdir -p /data/hbase
RUN mkdir -p /root/.profile.d

ADD http://apache.org/dist/hbase/hbase-0.94.27/hbase-0.94.27.tar.gz /opt/downloads/
RUN tar xzvf /opt/downloads/hbase-*gz 
RUN rm /opt/downloads/hbase-*gz 
RUN ["/bin/bash", "-c", "mv hbase-* /opt/hbase"]

ADD start_hbase.sh /opt/sei-bin/
ADD hbase-site.xml /opt/hbase/conf/
EXPOSE 4242 5555 60000 60010 60030

#Install OpenTSDB and scripts
RUN mkdir /opt/opentsdb
ADD https://github.com/OpenTSDB/opentsdb/archive/master.tar.gz /opt/opentsdb/
WORKDIR /opt/opentsdb 
RUN tar xzf master.tar.gz -C . --strip-components=1
RUN rm master.tar.gz
RUN ./build.sh

ADD start_opentsdb.sh create_tsdb_tables.sh /opt/sei-bin/

#Install Supervisord
RUN mkdir -p /var/log/supervisor
ADD supervisor-hbase.conf supervisor-system.conf supervisor-tsdb.conf supervisor-dd.conf /etc/supervisor/conf.d/ 

# Install DoubleDecker proxy
# ADD ./doubledecker /opt/sei-bin/doubledecker
ADD start_ddproxy.sh ddproxy.py /opt/sei-bin/
WORKDIR /root

RUN pip3 install PyNaCl
ADD https://github.com/acreo/DoubleDecker-py/archive/master.tar.gz /root/
RUN tar xzf master.tar.gz
RUN rm master.tar.gz
WORKDIR /root/DoubleDecker-py-master/
RUN python3 setup.py build
RUN python3 setup.py install

ENV CLIENT_NAME opentsdb
ENV DEALER_PORT tcp://172.17.0.1:5555
RUN chmod +x /opt/sei-bin/*

RUN DEBIAN_FRONTEND=noninteractive apt-get remove -yq make autoconf
# default run command
CMD /usr/bin/supervisord
