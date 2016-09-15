FROM      osrg/ryu
MAINTAINER iMinds & Acreo

#RUN apt-get update && apt-get install -y \
#	iproute \
#	net-tools \
#	iputils-ping \
#	curl

RUN apt-get update
RUN apt-get install -yq iproute net-tools iputils-ping curl python-zmq openssh-server
RUN pip install ipaddress
RUN pip install --upgrade pip

# dependencies for rest api
RUN pip install flask
RUN pip install flask-restful

# setup ssh login
RUN echo 'root:root' | chpasswd
RUN sed -i 's/PermitRootLogin without-password/PermitRootLogin yes/' /etc/ssh/sshd_config

ADD authorized_keys /root/.ssh/authorized_keys

# set permissions correctly, otherwise ssh will not be able to login via ssh keys 
RUN chmod 700 /root/.ssh
RUN chmod 600 /root/.ssh/authorized_keys

#ADD ryu_ddclient.py ryu_ddclient.py

# setup DoubleDecker
RUN apt-get update
#RUN apt-get install -yq python3 python3-zmq python3-pip libffi-dev libsodium-dev 
RUN apt-get install -yq gcc python-setuptools python-dev python-zmq python-pip libffi-dev libsodium-dev python-nacl
RUN pip install jsonrpcserver
RUN pip install wheel
#RUN pip install cffi
#RUN pip install pynacl
RUN pip install gipc
RUN pip install future
RUN pip install tornado
RUN wget https://github.com/acreo/DoubleDecker-py/archive/master.tar.gz -O dd.tar.gz
RUN tar xzf dd.tar.gz && rm dd.tar.gz
WORKDIR /root/DoubleDecker-py-master/
RUN python setup.py build
RUN python setup.py install


WORKDIR /root/

# copy key file (should be same key file as used on UN server)
COPY ./a-keys.json ./a-keys.json

# setup ryu files
COPY ./ryu_app ./ryu_app

ADD start.sh start.sh
RUN chmod +x start.sh

CMD ./start.sh
