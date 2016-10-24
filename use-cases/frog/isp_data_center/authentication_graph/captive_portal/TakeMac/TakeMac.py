'''
Created on 26/ago/2014

@author: ivano
'''

import subprocess
import falcon
import logging

class TakeMac(object):
    '''
    classdocs
    '''
    ifconfig = "ifconfig "
    interface = "eth0 "
    pipe = """| grep HWaddr | awk '{ print $5 }'"""

    def __init__(self,mac_address):
        '''
        Constructor
        '''
        self.mac_address = mac_address
        
    def on_get(self, request, response):
        logging.debug("Transmitting the mac_address: " + self.mac_address)
#         headers = {'Content-Type': 'application/json'}
        body = "{ mac_address: " + self.mac_address + " }"
        response.set_header('Content-Type', 'application/json')
        response.body = body
        response.status = falcon.HTTP_200
        exit
