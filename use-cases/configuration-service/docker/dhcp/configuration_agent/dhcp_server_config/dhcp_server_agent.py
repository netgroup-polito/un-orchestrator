'''
Created on Dec 28, 2015

@author: fabiomignini
'''
import logging

from configuration_agent.agent import ConfigurationAgent
from configuration_agent.dhcp_server_config.dhcp_server import Dhcp

logging.basicConfig(level=logging.DEBUG)
ConfigurationAgent(Dhcp())