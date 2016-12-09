'''
Created on Dec 26, 2015

@author: fabiomignini
'''
from configuration_agent.utils import Bash

class Interface(object):
    def __init__(self, name = None, status = None, mac_address = None,
                 ipv4_address = None, netmask = '24', _type = None,
                 configuration_type = None, default_gw = None):
        self.name = name
        self.status = status
        self.mac_address = mac_address
        self.ipv4_address = ipv4_address
        self.netmask = netmask
        self.type = _type
        self.configuration_type = configuration_type
        self.default_gw = default_gw
    
    def get_interface_info(self):
        pass
    
    def set_interface_up(self):
        pass
    
    def set_interface(self):
        if self.type != 'config' and self.configuration_type == 'static':
            Bash('ifconfig '+self.name+' '+self.ipv4_address+'/'+self.netmask)
            if self.default_gw is not None:
                Bash('route add default gw '+self.default_gw+' '+self.name)
        elif self.type != 'config' and self.configuration_type == 'dhcp':
            if self.default_gw is not None:
                Bash('route del default gw '+self.default_gw)
            Bash('ifconfig '+self.name+' 0')
            Bash('if [ ! -e "/usr/sbin/dhclient" ]; then cp /sbin/dhclient /usr/sbin/dhclient done fi')
            Bash('/usr/sbin/dhclient '+self.name+' -v')
