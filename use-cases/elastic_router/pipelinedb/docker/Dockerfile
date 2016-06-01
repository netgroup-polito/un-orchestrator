FROM ubuntu:trusty

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y locales \
  && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
  
ENV LANG en_US.utf8

COPY scripts /scripts/



# install some basics
WORKDIR / 
RUN apt-get install -y wget nano libxml2 libxml2-dev libreadline6 libreadline6-dev check g++ flex bison zlib1g-dev libpq-dev libcurl4-openssl-dev libpython-dev libncurses5-dev
RUN wget -q https://github.com/pipelinedb/pipelinedb/archive/0.8.6.tar.gz
RUN tar xvzf 0.8.6.tar.gz
WORKDIR /pipelinedb-0.8.6
RUN apt-get install -y python
RUN apt-get install -y make
RUN ./configure --with-python
RUN make install
RUN useradd -m pipeline -s "/bin/bash"
RUN echo >> /home/pipeline/.profile
RUN echo "PATH=/usr/local/pipelinedb/bin:$PATH" >> /home/pipeline/.profile

RUN /scripts/install_extras.sh
RUN apt-get install -y python-scipy
EXPOSE 5432
ENV PATH /usr/local/pipelinedb/bin:$PATH
CMD ["/scripts/startpipeline"]
