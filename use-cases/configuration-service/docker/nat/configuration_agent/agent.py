#!/usr/bin/python3
# -*- coding: utf-8 -*-

from doubledecker import clientSafe
from configuration_agent import utils
from configuration_agent import constants

from threading import Thread
import logging
import json
import time

class ConfigurationAgent(clientSafe.ClientSafe):
    '''
    Configuration agent, this class is in charge to 
    talk with the configuration server to provide
    the configuration to a VNF.
    '''

    def __init__(self, vnf):
        self.tenant_id = None
        self.vnf_name = None
        self.publishable = False
        
        self.vnf = vnf

        #  Tenant association phase
        self.phase = "TenantAssociation"
        self.initial_registration()

        # New registration with the right tenant id
        self.phase = "NewRegistration"
        self.new_registration()  
    
    def registration(self, name, dealerurl, customer, keyfile):
        super().__init__(name=name.encode('utf8'), 
                         dealerurl=dealerurl, 
                         customer=customer.encode('utf8'),
                         keyfile=keyfile)
        thread = Thread(target=self.start)
        thread.start()
        return thread

    def initial_registration(self):
        assert(self.vnf.mac_address is not None)

        thread = self.registration(name = self.vnf.mac_address,
                          dealerurl = constants.dealer,
                          customer='default',
                          keyfile = constants.keyfile)
        thread.join()

    def new_registration(self):
        # The VNF has received its own tenant id 
        # and vnf name, now can register itself
        # to the DD with the right tenant ID

        assert(self.tenant_id is not None)
        assert(self.vnf_name is not None)

        thread = self.registration(name = self.vnf.mac_address,
                          dealerurl = constants.dealer,
                          customer = self.tenant_id,
                          keyfile = constants.keyfile)	

        while True:
            # Export the status every 15 seconds
            time.sleep(15)
            self.publish_status()
            logging.debug('publish status')
        thread.join()

    def tenant_association_phase(self):
        self.publish_public('public.tenant_association', 'tenant association request')

    def default_configuration_phase(self):
        self.publish_public('public.default_configuration', 'default configuration request')

    def configuration_subscription(self):
        self.subscribe('/'+self.vnf.type+'/'+self.vnf_name, 'noscope')
        
    def publish_status(self):
        if self.publishable:
            self.publish_public('public.status_exportation', self.vnf.get_json_instance())
        
    def configuration(self):
        self.configuration_subscription()
        self.publish_status()
        logging.debug('publish status')
        
    def config(self, name, dealerURL, customer):
        super().config(name, dealerURL, customer)

    def on_data(self, dest, msg):
        logging.debug(self.phase)
        if self.phase == "TenantAssociation":
            #
            #The VNF has received its own tenant id 
            # and vnf name, now can close the connection
            # with the DD with the 'default' tenant
            msg = msg.decode()
            logging.debug (msg)
            try:
                self.tenant_id = msg.split(',')[0]
                self.vnf_name = msg.split(',')[1]
            except Exception as ex:
                logging.debug (ex)
            self.shutdown()
        elif self.phase == "DefaultConfiguration":
            msg = msg.decode()
            if msg == "":
                logging.debug("No default configuration")
            else:
                logging.debug('configuring json: '+msg)
                # Validate json

                exit_code, output = utils.validate_json(msg, self.vnf.yang_model)
                if exit_code is not None:
                    raise Exception("Wrong configuration file: "+output)
                else:
                    logging.debug("Good validation!")
                    self.publishable = True
            
                # Configure VNF
                self.vnf.set_status(json.loads(msg))
    
            #Configuration phase start
            self.phase = "Configuration"
            self.configuration()
        elif self.phase == "Configuration":
            if not self.publishable:
                self.publishable = True

            msg = msg.decode()
            logging.debug('configuring json: '+msg)
            # Validate json
            exit_code, output = utils.validate_json(msg, self.vnf.yang_model)
            if exit_code is not None:
                raise Exception("Wrong configuration file: "+output)
            else:
                logging.debug("Good validation!")
            
            # Configure VNF
            self.vnf.set_status(json.loads(msg))
            # Export again the status
            self.publish_status()
        else:
            logging.debug('on_data')
            logging.debug(msg)
            
    def on_discon(self):
        pass
        
    def on_pub(self):
        pass
        
    def on_reg(self):
        if self.phase == "TenantAssociation":
            self.tenant_association_phase()
        elif self.phase == "NewRegistration":
            logging.debug('new registration')
            # Pub status, and ask for configuration
            #  Default configuraiotn phase
            self.phase = "DefaultConfiguration"
            logging.debug("Asking for a default configuration...")
            self.default_configuration_phase()  
        else:
            logging.debug('on_reg')
        
    def unsubscribe(self):
        super().unsubscribe()
    
    def start(self):
        logging.basicConfig(level=logging.DEBUG)
        super().start()
        
    def on_error(self, code, msg):
        logging.debug(code + ": " + msg)
