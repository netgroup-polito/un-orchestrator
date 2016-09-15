import re
import urllib2
import copy
import ipaddress

from ryu.ofproto import ofproto_v1_3, ofproto_v1_3_parser
from ryu.lib.packet import ether_types, ethernet, ipv4, tcp, arp, udp, icmp, vlan, ipv6, lldp
import logging
#Set the logger
logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

class DP:
    """
    class to hold the Data Path instances (ovs switches) of the elastic router
    """
    def __init__(self, nffg_name, ovs_id, ports=[]):
        # ovs bridge name, name from nffg, same as id in nffg
        self.name = nffg_name
        # ovs id from nffg
        self.id = ovs_id
        # ovs bridge datapath id in ryu
        self.datapath_id = None
        # ovs bridge datapath object in ryu
        self.datapath = None
        # ovs bridge is detected by control app via openflow
        self.registered = False
        # ovs bridge ports
        self.ports = []
        for port in ports:
            if isinstance(port, basestring):
                port_id = len(self.ports)+1
                new_port = DPPort(port, port_id)
                new_port.DP = self
                self.ports.append(new_port)
            elif isinstance(port, DPPort):
                port.DP = self
                self.ports.append(port)

        self.mac_to_port = {}

        # statistics of this DP
        self.port_txstats_packets = {}
        self.port_rxstats_packets = {}
        self.port_txrate_packets = {}
        self.port_rxrate_packets = {}
        self.port_txstats_bytes = {}
        self.port_rxstats_bytes = {}
        self.port_txrate_bytes = {}
        self.port_rxrate_bytes = {}
        self.previous_monitor_time = {} #store time of monitor data to calculate correct timedelta and rx_rate

        # openflow table of this DP
        # contains tuple [(match_dict{}, actions[], priority),
        self.oftable = []
        self.set_default_oftable_new()

        # oftable to translate upon scaling out
        self.scale_port_dict = {}

    def set_default_oftable_new(self):
        # send new ARP packets to controller
        priority = 2
        match_dict = create_dictionary(eth_type=ether_types.ETH_TYPE_ARP)
        ofproto = ofproto_v1_3
        parser = ofproto_v1_3_parser
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.oftable.append((match_dict, actions, priority))

        # send new ipv4 packets to controller
        priority = 2
        match_dict = create_dictionary(eth_type=ether_types.ETH_TYPE_IP)
        ofproto = ofproto_v1_3
        parser = ofproto_v1_3_parser
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.oftable.append((match_dict, actions, priority))

        # drop all other packets
        priority = 1
        match_dict = create_dictionary()
        actions = []
        self.oftable.append((match_dict, actions, priority))

    # process routing table from config file
    # only called for the initial nffg, gets translated on scaling
    def set_routing_table(self, routing_table):
        for routing_entry in routing_table:
            dest_sap = routing_entry['SAP']
            # find output port on this DP to reach dest_sap
            out_portno = None
            for port in self.ports:
                if port.port_type == DPPort.External and port.linked_port.ifname == dest_sap:
                    out_portno = port.number
                    break
                elif port.linked_port.port_type == DPPort.Internal \
                    and port.linked_port.forward_extport.linked_port.ifname == dest_sap:
                    out_portno = port.number
                    break

            if out_portno is None:
                logging.info("No out port found for destination: {0}".format(dest_sap))
                return

            priority = 30
            parser = ofproto_v1_3_parser
            actions = [parser.OFPActionOutput(out_portno)]

            match_list = []
            for net in routing_entry['net']:
                network = ipaddress.ip_network(net)
                ip_match = (network.network_address.__str__(), network.netmask.__str__())
                match_dict = create_dictionary(eth_type=ether_types.ETH_TYPE_IP, ipv4_dst=ip_match)
                match_list.append(match_dict)
                #logging.info("{0} set routing entry {1}".format(self.name, ip_match))

            for match_dict in match_list:
                self.oftable.append((match_dict, actions, priority))



    # call this function when new DP is registered and port numbers are known
    def set_default_oftable_scale(self):
        # add default flow entries for new DP
        external_ports = [port for port in self.ports if port.port_type == DPPort.External]
        internal_ports = [port for port in self.ports if port.port_type == DPPort.Internal]
        # assume only 1 external port possible
        if len(external_ports) == 1:
            out_port_number = external_ports[0].number
            for int_port in internal_ports:
                priority = 10
                match_dict = create_dictionary(in_port=int_port.number)
                parser = ofproto_v1_3_parser
                actions = [parser.OFPActionOutput(out_port_number)]
                self.oftable.append((match_dict, actions, priority))
         # for multiple external ports, use VLAN IDs


    # call this function when new DP is registered and port numbers are known
    def translate_oftable_scale(self):

        logging.info("of table translation disabled to limit flow entries")
        return

        scale_port_dict = self.scale_port_dict

        # add new flow_entries

        # make list of unique old_DPs, which flow table needs to be migrated
        old_DP_list = []
        for old_in_port in scale_port_dict:
            old_DP = old_in_port.DP
            old_DP_list.append(old_DP)

        old_DP_list = list(set(old_DP_list))

        for old_DP in old_DP_list:

            ofproto = old_DP.datapath.ofproto
            #new_in_port = scale_port_dict[old_in_port]
            #new_DP = new_in_port.DP

            '''
            # new_DP should be the same as self
            if new_DP.name is not self.name:
                logging.info('This DP:{0} checked DP: {1}, try next...'.format(self.name, new_DP.name))
                continue
            '''

            for match_dict, actions, priority  in old_DP.oftable:

                # check if this oftable entry contains a port to be translated
                # (the default port entries have other match entries eg. eth_type)

                # only translate entries added by packet_in handler (has priortity = 20)
                if priority != 20:
                    continue

                logging.info('old DP match dict: {0}'.format(match_dict))
                logging.info('old DP actions dict: {0}'.format(actions))
                #logging.info('old_in_port number: {0}'.format(old_in_port.number))
                #logging.info('old_in_port : {0}'.format(old_in_port.ifname))
                #logging.info('new_in_port : {0}'.format(new_in_port.ifname))


                new_match_dict = copy.deepcopy(match_dict)
                # set new in_port
                if 'in_port' in match_dict:
                    old_in_portno = match_dict['in_port']
                    old_in_port = old_DP.get_port_by_number(old_in_portno)
                    new_in_port = scale_port_dict[old_in_port]
                    if new_in_port in self.ports:
                        # this in_port match needs to be transalated to this DP
                        new_match_dict['in_port'] = new_in_port.number
                    else:
                        # this openflow entry is not for this DP, or is an internal port
                        continue

                    '''
                    if match_dict['in_port'] == old_in_port.number:
                        new_match_dict['in_port'] = new_in_port.number
                    else:
                        # this openflow entry is not for this DP, or is an internal port
                        continue
                    '''
                # also routing entries need to be translated
                # -> routing entries are set by set_routing_table()
                #elif 'ipv4_dst' in match_dict:
                #    new_match_dict = copy.deepcopy(match_dict)
                #else:
                    # continue


                # set new out_port
                new_actions = copy.deepcopy(list(actions))
                for i in range(0, len(actions)):
                    # skip LOCAL port
                    if actions[i].port > 100:
                        continue
                    elif actions[i].port == ofproto.OFPP_FLOOD:
                        new_actions[i].port = ofproto.OFPP_FLOOD
                    else:
                        old_outportno = actions[i].port
                        logging.info('old out portno: {0}'.format(old_outportno))
                        old_outport = old_DP.get_port_by_number(old_outportno)
                        logging.info('old out port: {0}'.format(old_outport.ifname))

                        #for port in scale_out_port_dict:
                        #    logging.info('scale_out_port_dict: {0}'.format(port.ifname))

                        if old_outport.port_type == DPPort.Internal:
                            old_outport_ext = old_outport.linked_port.forward_extport
                            new_outport = scale_port_dict[old_outport_ext]

                        elif old_outport.port_type == DPPort.External:
                            new_outport = scale_port_dict[old_outport]

                        dest_DP = new_outport.DP
                        logging.info('new out port : {0}'.format(new_outport.ifname))

                        if dest_DP.name == self.name:
                            # in and out port are on the same DP, or DPs are scaled in
                            new_outportno = new_outport.number
                            new_actions[i].port = new_outportno
                            logging.info('same DP, new local out port number: {0}'.format(new_outportno))
                        else:
                            # else  ports are on different DPs
                            for port in dest_DP.ports:
                                if port.port_type == DPPort.External: continue
                                # out port is located on another DP as in port, check the internal ports
                                if port.linked_port.DP.name == self.name and \
                                        port.forward_extport == new_outport:
                                    #logging.info('new out port remote: {0}'.format(port.ifname))
                                    local_outport = port.linked_port
                                    logging.info('new out port local: {0}'.format(local_outport.ifname))
                                    new_outportno = local_outport.number
                                    logging.info('new local out port number: {0}'.format(local_outport.number))
                                    new_actions[i].port = int(new_outportno)
                                    break

                    self.oftable.append((new_match_dict, new_actions, priority))
        # reset dictionary
        #self.scale_out_port_dict = {}

    def translate_mactable_scale(self):

        scale_port_dict = self.scale_port_dict
        # make list of unique old_DPs, which flow table needs to be migrated
        old_DP_list = []
        for old_in_port in scale_port_dict:
            old_DP = old_in_port.DP
            old_DP_list.append(old_DP)

        old_DP_list = list(set(old_DP_list))

        for old_DP in old_DP_list:
            for mac_src in old_DP.mac_to_port:
                old_mac_portno = old_DP.mac_to_port[mac_src]
                old_mac_port = old_DP.get_port_by_number(old_mac_portno)

                if old_mac_port.port_type == DPPort.Internal:
                    old_outport_ext = old_mac_port.linked_port.forward_extport
                    new_outport = scale_port_dict[old_outport_ext]

                elif old_mac_port.port_type == DPPort.External:
                    new_outport = scale_port_dict[old_mac_port]

                dest_DP = new_outport.DP

                new_outportno = None
                if dest_DP.name == self.name:
                    # in and out port are on the same DP, or DPs are scaled in
                    new_outportno = new_outport.number
                else:
                    # else  ports are on different DPs
                    for port in dest_DP.ports:
                        if port.port_type == DPPort.External: continue
                        # out port is located on another DP as in port, check the internal ports
                        if port.linked_port.DP.name == self.name and \
                                port.forward_extport == new_outport:
                            #logging.info('new out port remote: {0}'.format(port.ifname))
                            local_outport = port.linked_port
                            #logging.info('new out port local: {0}'.format(local_outport.ifname))
                            new_outportno = local_outport.number
                            #logging.info('new local out port number: {0}'.format(local_outport.number))
                            break
                if new_outportno is not None:
                    self.mac_to_port[mac_src] =  new_outportno
                else:
                    logging.info('no new output port found for mac : {0}'.format(mac_src))


    def get_port(self, port_name=None):
        for port in self.ports:
            if port.ifname == port_name:
                return port

    def get_port_by_number(self, number):
        for port in self.ports:
            if port.number == number:
                return port

    def add_port(self,  ifname, port_type=None, linked_port=None ):
        port_id = len(self.ports) + 2 #because DP mgmt port and control port is not added here, only the data ports
        new_port = DPPort(ifname, port_id, DP_parent=self, port_type=port_type, linked_port=linked_port)
        self.ports.append(new_port)
        return new_port

    def check_connected(self, linked_DP):
        """
        return true is linked_DP is connected with a port to this DP
        """
        for port in self.ports:
            try:
                if port.linked_port.DP is linked_DP:
                    return True
            except:
                continue
        return False


class DPPort:
    """
    class to hold the ports of the DP instances in the elastic router
    """

    Internal = 1
    External = 2
    SAP = 3

    def __init__(self, ifname, id, port_number=None, DP_parent=None, port_type=None, linked_port=None, forward_extport=None):
        self.ifname = ifname
        self.id = id #port id in nffg
        self.number = port_number
        self.port_type = port_type #internal or external or SAP link
        self.DP = DP_parent
        self.linked_port = linked_port
        self.forward_extport = forward_extport # external port where an internal port forwards to on the same DP
        #self.virtualizer_port = virtualizer_port # virtualizer port object


    @staticmethod
    def get_datapath_name(ifname):
        # find datapath name starting with 'ER'
        match = re.search('\Aovs\d*(?=_eth\d*)', ifname)
        if match:
            return match.group()
        else:
            return None

def create_dictionary(**kwargs):
    return kwargs