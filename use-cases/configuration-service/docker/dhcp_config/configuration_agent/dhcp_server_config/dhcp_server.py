'''
Created on Dec 26, 2015

@author: fabiomignini
'''
import re
import logging
import ipaddress
from socket import inet_ntoa
from struct import pack
#import xmltodict 
try:
    import StringIO
except ImportError:
    from io import StringIO
import json
import types
import collections
import os
import inspect
import netifaces
from netaddr import IPNetwork 
import iptools

from pyang.__init__ import Context, FileRepository
from pyang.translators.yin import YINPlugin
from pyang import plugin
from configuration_agent.dhcp_server_config import constants
from configuration_agent.dhcp_server_config.client import Client

from configuration_agent import utils
from configuration_agent.utils import Bash
from configuration_agent.common.interface import Interface
        
class Dhcp(object):
    '''
    Class that configure and export
    the status of a dhcp VNF
    '''
    yang_module_name = 'config-dhcp-server'
    type = 'dhcp'
    
    def __init__(self):
        self.interfaces = []
        self.json_instance = {self.yang_module_name+':'+'interfaces':{'ifEntry':[]}, 
                              self.yang_module_name+':'+'server':{'globalIpPool':{}}}
        self.if_entries = self.json_instance[self.yang_module_name+':'+'interfaces']['ifEntry']
        self.yang_model = self.get_yang()
        # MAC address of the configuration interface
        self.mac_address = utils.get_mac_address(constants.configuration_interface)
        self.dhcp_interfaces = []
        self.clients = {}
    
    def get_json_instance(self):
        '''
        Get the json representing the status
        of the VNF.
        '''
        return json.dumps(self.get_status())
    
    def get_yang(self):
        '''
        Get from file the yang model of this VNF
        '''
        base_path = os.path.realpath(os.path.abspath(os.path.split(inspect.getfile(inspect.currentframe()))[0]))

        with open (base_path+"/"+self.yang_module_name+".yang", "r") as yang_model_file:
            return yang_model_file.read()
    
    def get_status(self):
        '''
        Get the status of the VNF
        '''
        self.get_interfaces()
        self.get_dhcp_configuration()
        self.get_interfaces_dict()
        logging.debug(json.dumps(self.json_instance))
        return self.json_instance
    
    def get_dhcp_configuration(self):
        '''
        Check if the dhcp is enabled and  which
        is the interfaces in which it is enebled.
        '''
        self.json_instance[self.yang_module_name+':'+'server']['globalIpPool'] = self.parse_dhcpd_dot_conf()
        self.json_instance[self.yang_module_name+':'+'server']['clients'] = self.parse_dhcpd_lease()
        with open('/etc/default/isc-dhcp-server', 'r') as isc_dhcp_server_file:
            interface_lines = isc_dhcp_server_file.readlines() 
        interfaces = interface_lines[0].split('INTERFACES="')[1].split('"')[0].split(' ')
        for dhcp_interface in interfaces:
            for interface in self.interfaces:
                if interface.name == dhcp_interface:
                    interface.type = 'dhcp'

    def get_interfaces_dict(self):
        '''
        Get a python dictionary with the interfaces
        of the VNF
        '''
        old_if_entries = self.if_entries
        self.json_instance[self.yang_module_name+':'+'interfaces']['ifEntry'] = [] 
        self.if_entries = self.json_instance[self.yang_module_name+':'+'interfaces']['ifEntry']
        for interface in self.interfaces:
            interface_dict = self.get_interface_dict(interface)
            for old_if_entry in old_if_entries:
                if interface.name == old_if_entry['name']:
                    interface.configuration_type = old_if_entry['configurationType'] 
                    interface_dict['configurationType'] = old_if_entry['configurationType'] 
            self.if_entries.append(interface_dict)
    
    def get_interface_dict(self, interface):
        dict = {}
        dict['name'] = interface.name
        if interface.configuration_type is not None:   
            dict['configurationType'] = interface.configuration_type
        else:
            dict['configurationType'] = 'not_defined'
        if interface.type is not None:
            dict['type'] = interface.type
        else:
            dict['type'] = 'not_defined'
        if interface.ipv4_address is not None and interface.ipv4_address != "":
            dict['address'] = interface.ipv4_address
        if interface.default_gw is not None and interface.ipv4_address != "":
            dict['default_gw'] = interface.default_gw
        return dict
    
    def set_status(self, json_instance):
        '''
        Set the status of the VNF starting from a
        json instance
        '''
        logging.debug(json_instance)
        if_entries = json_instance[self.yang_module_name+':'+'interfaces']['ifEntry']
        interfaces = []
        self.dhcp_interfaces = []
        for interface in if_entries:
            # Set interface
            logging.debug(interface)
            if 'default_gw' not in interface:
                default_gw = None
            else:
                default_gw = interface['default_gw']
            if 'address' not in interface:
                address = None
            else:
                address = interface['address']
            new_interface = Interface(name = interface['name'], 
                                        ipv4_address= address,
                                        _type = interface['type'],
                                        configuration_type= interface['configurationType'],
                                        default_gw = default_gw)
            if new_interface.type == 'dhcp':
                self.dhcp_interfaces.append(new_interface)
            new_interface.set_interface()
            interfaces.append(new_interface)
        self.if_entries = if_entries
        self.json_instance = json_instance
        self.if_entries = self.json_instance[self.yang_module_name+':'+'interfaces']['ifEntry']
        
        self.get_interfaces()
        self.get_interfaces_dict()

        self.configure_dhcp(json_instance[self.yang_module_name+':'+'server']['globalIpPool'])    
    
    def parse_dhcpd_lease(self):
        lease = re.compile("^lease ")
        starts = re.compile("^  starts")
        ends = re.compile("^  ends")
        ethernet = re.compile("^  hardware ethernet")
        hostname = re.compile("^  client-hostname")
        binding = re.compile("^  binding state")
        final_pattern = re.compile("^}")
        self.clients = {}
        with open('/var/lib/dhcp/dhcpd.leases') as lease_file:
            lease_lines = lease_file.readlines()
        
        state = None
        mac_address = None
        ip = None
        for lease_line in lease_lines:
            lease_line.rstrip()
            
            if lease.match(lease_line):
                data = 1
            elif final_pattern.match(lease_line):
                if state == 'active' and mac_address not in self.clients:
                    logging.debug("Client mac: "+mac_address+" ip: "+ip)
                    client = Client(mac_address = mac_address, ip_address = ip)
                    self.clients[mac_address] = client
            try:
                data
            except:
                # Data is not defined
                pass
            else:
                # Data is defined
                if lease.match(lease_line):
                    ip = lease_line.split(' ')[1]
                elif starts.match(lease_line):
                    pass
                elif ends.match(lease_line):
                    pass
                elif ethernet.match(lease_line):
                    mac_address = lease_line.split(' ')[4].split(';')[0]
                    logging.debug("mac_address: "+mac_address)
                elif hostname.match(lease_line):
                    pass
                elif binding.match(lease_line):
                    logging.debug(lease_line)
                    state = lease_line.split(' ')[4].split(';')[0]
                    logging.debug("state: "+state)
        clients_dict = []
        for client in self.clients.values():
            client_dict = {}
            client_dict['mac_address'] = client.mac_address
            client_dict['ip'] = client.ip_address
            clients_dict.append(client_dict)
        return clients_dict
    
    def parse_dhcpd_dot_conf(self):
        with open('/etc/dhcp/dhcpd.conf') as dhcpd_file:
            dhcpd_lines = dhcpd_file.readlines()
        
        dhcp_server_conf = {}
        dhcp_server_conf['defaultLeaseTime'] = dhcpd_lines.pop(0).split('default-lease-time ')[1].split(';')[0]
        dhcp_server_conf['maxLeaseTime'] = dhcpd_lines.pop(0).split('max-lease-time ')[1].split(';')[0]
        dhcp_server_conf['gatewayIp'] = {}
        dhcp_server_conf['gatewayIp']['gatewayMask'] = dhcpd_lines.pop(0).split('option subnet-mask ')[1].split(';')[0]
        dhcp_server_conf['gatewayIp']['gatewayIp'] = dhcpd_lines.pop(0).split('option routers ')[1].split(';')[0]
        dhcp_server_conf['domainNameServer'] = dhcpd_lines.pop(0).split('option domain-name-servers ')[1].split(';')[0]
        dhcp_server_conf['domainName'] = dhcpd_lines.pop(0).split('option domain-name "')[1].split('";')[0]
        dhcp_server_conf['sections'] = {}
        dhcp_server_conf['sections']['section'] = []
        dhcpd_lines.pop(0)
        dhcpd_lines.pop(len(dhcpd_lines)-1)
        for dhcpd_line in dhcpd_lines:
            section = {}
            section['sectionStartIp'] = dhcpd_line.split('    range ')[1].split(' ')[0]
            section['sectionEndIp'] = dhcpd_line.split('    range ')[1].split(' ')[1].split(';')[0]
            dhcp_server_conf['sections']['section'].append(section)
        return dhcp_server_conf
           
    def configure_dhcp(self, dhcp_server_conf):
        '''
        example of configuration
        
        default-lease-time 600;
        max-lease-time 7200;
        option subnet-mask 255.255.255.0;
        option broadcast-address 192.168.1.255;
        option routers 192.168.1.254;
        option domain-name-servers 192.168.1.1, 192.168.1.2;
        option domain-name "mydomain.example";
        
        subnet 192.168.1.0 netmask 255.255.255.0 {
            range 192.168.1.10 192.168.1.100;
            range 192.168.1.150 192.168.1.200;
        }
        '''
        with open('/etc/dhcp/dhcpd.conf', 'w') as dhcpd_file:
            dhcpd_file.write('default-lease-time '+dhcp_server_conf['defaultLeaseTime']+';\n')
            dhcpd_file.write('max-lease-time '+dhcp_server_conf['maxLeaseTime']+';\n')
            dhcpd_file.write('option subnet-mask '+dhcp_server_conf['gatewayIp']['gatewayMask']+';\n')
            dhcpd_file.write('option routers '+dhcp_server_conf['gatewayIp']['gatewayIp']+';\n')
            dhcpd_file.write('option domain-name-servers '+dhcp_server_conf['domainNameServer']+';\n')
            dhcpd_file.write('option domain-name "'+dhcp_server_conf['domainName']+'";\n')
            network = str(self.get_network(dhcp_server_conf['gatewayIp']['gatewayMask'], dhcp_server_conf['gatewayIp']['gatewayIp']))
            dhcpd_file.write('subnet '+network+' netmask '+dhcp_server_conf['gatewayIp']['gatewayMask']+' {\n')
            for section in dhcp_server_conf['sections']['section']:
                dhcpd_file.write('    range '+section['sectionStartIp']+' '+section['sectionEndIp']+';\n')
            dhcpd_file.write('}')
            dhcpd_file.truncate()
            
        # Set interfaces
        isc_dhcp_server = 'INTERFACES="'
        for index, interface in enumerate(self.dhcp_interfaces):
            if index != 0:
                isc_dhcp_server += ' '
            isc_dhcp_server +=  interface.name
        isc_dhcp_server += '"'
        with open('/etc/default/isc-dhcp-server', 'w') as isc_dhcp_server_file:
            isc_dhcp_server_file.write(isc_dhcp_server)
            isc_dhcp_server_file.truncate()
        
        # Restart service
        Bash('service isc-dhcp-server restart')
        if len(self.dhcp_interfaces) == 0:
            Bash('service isc-dhcp-server stop')
        
    def get_network(self, netmask, ip_address):
        netmask = str(iptools.ipv4.netmask2prefix(netmask))
        ip = IPNetwork(ip_address+'/'+netmask)           
        return ip.network
    
    def get_interfaces(self):
        '''
        Retrieve the interfaces of the VM
        '''
        interfaces = netifaces.interfaces()
        self.interfaces = []
        for interface in interfaces:
            if interface == 'lo':
                continue
            default_gw = ''
            configuration_type = None
            gws = netifaces.gateways()
            if gws['default'] != {} and gws['default'][netifaces.AF_INET][1] == interface:
                default_gw = gws['default'][netifaces.AF_INET][0]
            interface_af_link_info = netifaces.ifaddresses(interface)[17]
            if 2 in netifaces.ifaddresses(interface):
                interface_af_inet_info = netifaces.ifaddresses(interface)[2]
                ipv4_address = interface_af_inet_info[0]['addr']
                netmask = interface_af_inet_info[0]['netmask']
            else:
                ipv4_address = ""
                netmask = ""
            if interface == constants.configuration_interface:
                _type = 'config'
                configuration_type = 'dhcp'
            else:
                _type = 'not_defined'
            self.interfaces.append(Interface(name = interface, status = None, 
                      mac_address = interface_af_link_info[0]['addr'],
                      ipv4_address = ipv4_address,
                      netmask = netmask,
                      default_gw = default_gw,
                      _type = _type,
                      configuration_type = configuration_type))
        
    def base_conf(self):
        Bash('echo "UseDNS no" >> /etc/ssh/sshd_config')
        
logging.basicConfig(level=logging.DEBUG)
