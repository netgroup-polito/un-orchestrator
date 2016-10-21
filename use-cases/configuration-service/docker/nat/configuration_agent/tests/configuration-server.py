#!/usr/bin/python3
# -*- coding: utf-8 -*-

from doubledecker import client
from configuration_agent import constants

from threading import Thread
import sys
import signal
import time
import logging
import json


class ConfigurationServer(client.ClientUnsafe):

    def __init__(self):
        self.counter = 0
        self.working_thread = None
        self.initial_registration()
        self.working_thread.join()
        
    
    
    def registration(self, name, dealerurl, customer):
        super().__init__(name=name.encode('utf8'), dealerurl=dealerurl, customer=customer.encode('utf8'))
        self.working_thread = Thread(target=self.start)
        self.working_thread.start()
        
        
    def initial_registration(self):
        self.registration(name = 'configuration_server',
                          dealerurl = constants.dealer,
                          customer='public')
    
    def subscribe_for_tenant_association_phase(self):
        self.subscribe('tenant_association', 'noscope')
        
    def subscribe_vnf_configuration(self):
        self.subscribe('nat', 'noscope')
        self.subscribe('firewall', 'noscope')
        self.subscribe('dhcp', 'noscope')
        
    def config(self, name, dealerURL, customer):
        super().config(name, dealerURL, customer)

    def on_data(self, dest, msg):
        logging.debug(msg)
    
    def on_discon(self):
        super().on_discon()
        
    def on_pub(self, src, topic, msg):
        msg = msg[0]
        logging.debug("on_pub")
        logging.debug(src)
        logging.debug(topic)
        logging.debug(msg)
        # TODO: some way to retrieve the tenant of the customer
        if topic.decode() == 'tenant_association':
            self.sendmsg("default."+src.decode(), '89fcd,nat_vm')
        elif self.counter == 0:
            if topic.decode() == 'nat':
                conf = {'config-nat:interfaces': {'ifEntry': [{'address': '10.0.2.21', 'configurationType': 'static', 'name': 'eth0', 'type': 'wan'}]}}
            elif topic.decode() == 'dhcp':
                conf = {'config-dhcp-server:interfaces': {'ifEntry': [{'address': '10.0.2.17', 'name': 'eth0', 'type': 'data', 'configurationType':'static'}]}, "config-dhcp-server:server":{"globalIpPool":{"ipPoolName":"pippo","gatewayIp":{"gatewayIp":"10.0.0.1","gatewayMask":"255.255.255.0"},"sections":{"section":[{"sectionIndex":"1","sectionStartIp":"10.0.0.100","sectionEndIp":"10.0.0.200"}]},"maxLeaseTime":"100","defaultLeaseTime":"10","domainNameServer":"8.8.8.8","domainName":"polito.it"}}}
            elif topic.decode() == 'firewall':
                conf = {"config-firewall:firewall":{"policy":[{"policy-name":"test-policy","action":"drop","in-interface":"eth0","source-address":"10.0.0.2","source-mask":"255.255.255.0","destination-address":"10.0.5.1","destination-mask":"255.255.255.0","protocol":"tcp","source-port":"2000","destination-port":"1000"}]},"config-firewall:interfaces":{"ifEntry":[{"address":"10.0.2.17","configurationType":"static","name":"eth0","type":"not_defined"}]}}
            #conf = {'config-nat:interfaces': {'ifEntry': [{'address': '10.0.2.19', 'configurationType': 'static', 'name': 'eth0', 'type': 'not_defined'}]}}
            logging.debug("Configuration json: "+json.dumps(conf))
            self.sendmsg('89fcd.'+src.decode(), json.dumps(conf))
            self.counter += 1
        
    def on_reg(self):
        logging.debug('registred')
        self.subscribe_for_tenant_association_phase()
        self.subscribe_vnf_configuration()
        
    def unsubscribe(self):
        super().unsubscribe()
        sys.exit(0)
    
    def start(self):
        logging.basicConfig(level=logging.DEBUG)
        super().start()
   
    def signal_handler(self, signal, frame):
        logging.debug("stop")
        self.shutdown()    
        sys.exit(0)
        
if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    genclient = ConfigurationServer()