# Configuration Agent

## Installation

- Enviroment (Tested on ubuntu 14.04.1)

```sh
$ sudo apt-get install python3-setuptools python3-pip libffi-dev
$ git clone https://github.com/pyca/pynacl
$ cd pynacl
$ sudo python3.4 setup.py install
$ sudo pip3 install aiozmq zmq pyang
$ sudo apt-get install xsltproc jing
$ sudo pip3 install netifaces xmltodict netaddr iptools
$ sudo pip3 install --upgrade python-iptables
```

- Double Decker

```
$ pip3 install PyNaCl
$ pip3 install pyzmq
$ pip3 install tornado
$ pip3 install cffi
$ pip3 install urwid

$ git clone https://github.com/Acreo/DoubleDecker-py.git
$ cd DoubleDecker-py
$ sudo python3 setup.py install
```

## Configuration

- Configuration file location

constants.py # For general variables
/<vnf>_config/constants.py # For VNF specific variables

## Start

- Start Configuration agent

```sh
cd generic-nfv-configuration-and-management
sudo ./start_<vnf>_agent.sh
```


