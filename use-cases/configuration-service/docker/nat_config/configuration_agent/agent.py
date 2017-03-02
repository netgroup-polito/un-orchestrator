#!/usr/bin/python3
# -*- coding: utf-8 -*-

from doubledecker import clientSafe
from configuration_agent import utils
from configuration_agent import constants

from threading import Event
from threading import Thread
import logging
import json
import time
import os
import sys


class ConfigurationAgent(clientSafe.ClientSafe):
    """
    Configuration agent, this class is in charge to
    talk with the configuration server to provide
    the configuration to a VNF.
    """

    def __init__(self, vnf):
        self.tenant_id = None
        self.vnf_name = None
        self.vnf_id = None
        self.publishable = False
        self.is_registered_to_cs = False
        self.is_registered_to_dd = False
        self.registered_to_dd = Event()
        self.registered_to_cs = Event()
        self.last_published_status = None
        self.datadisk_path = "/datadisk"
        self.metadata_file = self.datadisk_path + "/metadata"
        self.vnf = vnf

        assert os.path.isdir(self.datadisk_path) is True, "datadisk not mounted onto the VNF"
        assert os.path.exists(self.metadata_file) is True, "metadata file not found in datadisk"
        self.read_data_disk(self.metadata_file)
        self.start_agent()

    def read_data_disk(self, ds_metadata):
        try:
            metadata = open(ds_metadata, "r")
            for line in metadata.readlines():
                words = "".join(line.split()).split('=')
                if words[0] == "tenant-id":
                    self.tenant_id = words[1]
                elif words[0] == "vnf-name":
                    self.vnf_name = words[1]
                elif words[0] == "vnf-id":
                    self.vnf_id = words[1]
                else:
                    logging.debug("unknown keyword in metadata: " + words[0])
        except Exception as e:
            logging.debug("Error during metadata reading.\n" + str(e))
            sys.exit(1)
        finally:
            if metadata is not None:
                metadata.close()

        assert self.tenant_id is not None, "tenant-id key not found in metedata file"
        assert self.vnf_name is not None, "vnf-name key not found in metedata file"
        assert self.vnf_id is not None, "vnf-id key not found in metedata file"

    def registration(self, name, dealerurl, customer, keyfile):
        super().__init__(name=name.encode('utf8'),
                         dealerurl=dealerurl,
                         customer=customer.encode('utf8'),
                         keyfile=keyfile)
        thread = Thread(target=self.start)
        thread.start()
        return thread

    def start_agent(self):
        self.registered_to_dd.clear()
        self.registered_to_cs.clear()

        assert self.vnf.mac_address is not None, "Mac address is undefined"
        logging.debug("Registering to the message broker")
        thread = self.registration(name=self.vnf.mac_address,
                                   dealerurl=constants.dealer,
                                   customer=self.tenant_id,
                                   keyfile=constants.keyfile)

        while self.is_registered_to_dd is False:  # waiting for the agent to be registered to DD broker
            self.registered_to_dd.wait()
        while self.is_registered_to_cs is False:  # waiting for the agent to be registered to the configuration service
            logging.debug("Waiting for registration...")
            if not self.registered_to_cs.wait(5):
                if self.is_registered_to_cs is False:
                    self.vnf_registration()
        logging.debug("Registration successful")
        while True:
            # Export the status every 15 seconds
            time.sleep(3)
            self.publish_status()
        thread.join()

    def publish_status(self):
        if self.publishable:
            if self.last_published_status != self.vnf.get_json_instance():
                logging.debug("Publishing a new status")
                logging.debug("NEW STATUS: " + self.vnf.get_json_instance())
                self.publish_public('public.status_exportation', self.vnf.get_json_instance())
                self.last_published_status = self.vnf.get_json_instance()

    def config(self, name, dealerURL, customer):
        super().config(name, dealerURL, customer)

    def on_data(self, dest, msg):
        if not self.publishable:
            self.publishable = True

        msg = msg.decode()
        logging.debug("msg received: " + msg + " expected " + "REGISTERED " + self.tenant_id + '.' + self.vnf_id)
        if msg == "REGISTERED " + self.tenant_id + '.' + self.vnf_id:
            self.is_registered_to_cs = True
            self.registered_to_cs.set()
            return
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

    def on_discon(self):
        pass

    def on_pub(self):
        pass

    def on_reg(self):
        self.is_registered_to_dd = True
        self.registered_to_dd.set()
        self.vnf_registration()

    def vnf_registration(self):
        msg = ""
        msg += "vnf-id:" + self.vnf_id + "\n"
        msg += "vnf-name:" + self.vnf_name + "\n"
        msg += "tenant-id:" + self.tenant_id
        logging.debug('Registering to the configuration service: ' + msg)
        self.publish_public('public.vnf_registration', msg)

    def unsubscribe(self):
        super().unsubscribe()

    def start(self):
        logging.basicConfig(level=logging.DEBUG)
        super().start()

    def on_error(self, code, msg):
        logging.debug(code + ": " + str(msg))
