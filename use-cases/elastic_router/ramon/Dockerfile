FROM debian:testing
MAINTAINER Bertrand Pechenot berpec@acreo.se
RUN apt-get update --fix-missing
# RUN DEBIAN_FRONTEND=noninteractive apt-get install -yq apt-utils wget build-essential libtool git
# RUN DEBIAN_FRONTEND=noninteractive apt-get install -yq automake pkg-config libjson0-dev libzmq3-dev
RUN DEBIAN_FRONTEND=noninteractive apt-get install -yq python python3 python-scipy python3 python3-nacl python3-zmq python3-setuptools net-tools python-pip wget

# Broker default port 
EXPOSE 5555 

# TODO: write a run.sh script that handles ENV variables properly
ADD ./run.sh /bus/
ADD ./ratemon_client.py /bus/
ADD ./ratemon/ /bus/
WORKDIR /
RUN wget https://github.com/Acreo/DoubleDecker/archive/master.tar.gz -O dd.tar.gz
RUN tar xzf dd.tar.gz
RUN rm dd.tar.gz
WORKDIR /DoubleDecker-master/python
RUN python3 setup.py build
RUN python3 setup.py install
RUN pip install sh

WORKDIR /bus/
RUN DEBIAN_FRONTEND=noninteractive apt-get autoremove -yq
RUN DEBIAN_FRONTEND=noninteractive apt-get autoclean -yq
CMD ./run.sh
