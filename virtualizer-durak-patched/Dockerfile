FROM ubuntu:14.04

MAINTAINER Pontus Skoldstrom <ponsko@acreo.se>
RUN apt-get update

RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python-pip
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python-pkg-resources
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install python-setuptools
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install libpython2.7-dev
RUN pip install gunicorn falcon cython requests


EXPOSE 9090

# start in /proxy
WORKDIR /
RUN mkdir virtualizer
WORKDIR /virtualizer
COPY /un_native_nffg_library /virtualizer/un_native_nffg_library
COPY /virtualizer_library /virtualizer/virtualizer_library
COPY /config /virtualizer/config
COPY /config/configuration-ER.ini /virtualizer/config/configuration.ini
ADD *.py /virtualizer/
ADD *.xml /virtualizer/
WORKDIR /virtualizer/virtualizer_library
RUN python setup.py install 
WORKDIR /virtualizer/
CMD ["gunicorn","-b","0.0.0.0:9090", "virtualizer:api", "--timeout", "60"]
