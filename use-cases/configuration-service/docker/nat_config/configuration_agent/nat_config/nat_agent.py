'''
Created on Dec 28, 2015

@author: fabiomignini
'''
import logging

from configuration_agent.agent import ConfigurationAgent
from configuration_agent.nat_config.nat import Nat

logging.basicConfig(level=logging.DEBUG)
ConfigurationAgent(Nat())