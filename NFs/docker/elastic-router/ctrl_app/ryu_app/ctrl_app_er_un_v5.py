__author__ = 'Steven Van Rossem'

from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3, ofproto_v1_0
from ryu.lib.packet import packet
from ryu.lib.packet import ethernet, ipv4, tcp, arp, udp, icmp, vlan, ipv6, lldp
from ryu.ofproto import ether, inet
from ryu.lib import hub
from ryu.topology import switches
from ryu.topology.event import EventSwitchEnter, EventSwitchLeave, EventSwitchReconnected

from operator import attrgetter
import logging
#Set the logger
logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

import urllib2
import copy
import json
import os
import time

#from virtualizer_er import *
from json_er import import_json_file
from er_utils import *
from er_monitor import *
from er_ddclient import er_ddclient
import er_rest_api
from shutil import copyfile
from gui_server.gui_server import web_server
from gui_server.graph_builder import build_graph, build_graph_list, change_graph

# this version must use xml based nffg
os.environ["NFFG_FORMAT"] = "xml"
import er_nffg

from eventlet.green import zmq

class ElasticRouter(app_manager.RyuApp):

    _CONTEXTS = {
        'switches': switches.Switches
    }

    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    # TODO: put globals in separate file
    # cf-or address set from the nnfg as ENV variable
    un_cf_or_env = '172.17.0.1:8080'
    try:
        cfor_env = os.environ['CFOR']
    except:
        cfor_env = un_cf_or_env

    REST_Cf_Or =  'http://' + cfor_env
    REST_UN_Cf_Or = 'http://' + un_cf_or_env

    # rest api port address set from the nnfg as ENV variable
    try:
        rest_api_port = os.environ['REST_API_PORT']
    except:
        rest_api_port = 5000

    # rest api port address set from the nnfg as ENV variable
    try:
        gui_port = os.environ['GUI_PORT']
    except:
        gui_port = 8888

    # ip address of host machine
    try:
        host_ip = os.environ['HOST_IP']
    except:
        #host_ip = 'durak.testbed.se'
        host_ip = 'localhost'

    # ip address of host machine
    try:
        auto_scale = ('true' in os.environ['AUTO_SCALE'].lower())
    except:
        auto_scale = False

    # ip address of host machine
    try:
        DD_enable = not ('false' in os.environ['DD_ENABLE'].lower())
    except:
        DD_enable = True


    # file name + location (execution dir) of the config file to load
    # TODO load json config file from rest api
    CONFIG_FILE = 'elastic_router_config.json'

    def __init__(self, *args, **kwargs):
        super(ElasticRouter, self).__init__(*args, **kwargs)

        self.logger.setLevel(logging.DEBUG)
        self.logger.debug('Cf-Or interface: {0}'.format(self.REST_Cf_Or))

        # initialize last scaling direction variable
        self.scale_dir = None

        # ovs switches attached to elastic router
        self.DP_instances = {}
        # ovs dpid to DP object
        self.DPIDtoDP = {}


        # get config file with routing table
        self.config = import_json_file(self.CONFIG_FILE)

        self.VNFs_to_be_deleted = []

        #self.logger.debug('DP instances: {0}'.format(self.DP_instances))

        upper = self.config["ingress_rate_upper_threshold"]
        lower = self.config["ingress_rate_lower_threshold"]
        self.monitorApp = ElasticRouterMonitor(self, upper_threshold=upper, lower_threshold=lower)

        # start DD client
        if self.DD_enable:
            self.zmq_ = er_ddclient(self.monitorApp, self)

        #check if ctrl VNF is in the nffg, otherwise wait and poll for updated nffg
        found_ctrl = False
        while not found_ctrl:
            # parse the nffg from the Cf-Or
            self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)
            # check if ctrl is a vnf in the nffg
            found_ctrl = er_nffg.check_vnf_in_nffg(self.nffg, 'ctrl')
            if not found_ctrl:
                self.logger.info("vnf name: {0} not found in nffg, polling again via cf-or...".format('ctrl'))
                time.sleep(1)
            else:
                self.logger.info("vnf name: {0} found in nffg".format('ctrl'))


        # start gui web server to visualize ER topology as seen by ctrl app
        # use this pipe to communicate to the gui_server
        guest_port = self.gui_port
        host_port = er_nffg.get_mapped_port(self.nffg, 'ctrl', guest_port)

        if host_port is None:
            self.logger.info('mappped gui port not found, trying now with the local UN nffg')
            un_nffg = er_nffg.get_nffg(self.REST_UN_Cf_Or)
            host_port = er_nffg.get_mapped_port(un_nffg, 'ctrl', guest_port)

        # open zmq bus to send updated nffg's to the gui
        CTX = zmq.Context(1)
        self.gui_nffg1_sender = zmq.Socket(CTX, zmq.PUSH)
        self.gui_nffg1_sender.connect('ipc:///tmp/nffg1_message')
        self.gui_nffg2_sender = zmq.Socket(CTX, zmq.PUSH)
        self.gui_nffg2_sender.connect('ipc:///tmp/nffg2_message')
        self.gui_nffg3_sender = zmq.Socket(CTX, zmq.PUSH)
        self.gui_nffg3_sender.connect('ipc:///tmp/nffg3_message')
        self.gui_log_sender = zmq.Socket(CTX, zmq.PUSH)
        self.gui_log_sender.connect('ipc:///tmp/log_message')
        self.gui_count_sender = zmq.Socket(CTX, zmq.PUSH)
        self.gui_count_sender.connect('ipc:///tmp/count_message')

        # clear parsed nffg
        copyfile('gui_server/empty.json', 'gui_server/parsed_nffg.json')
        # start gui web server at port 8888
        self.logger.info("started gui at port: {0}".format(host_port))
        self.gui_server = web_server(self.host_ip, host_port, guest_port)

        # start rest api to easily scale in/out
        self.rest_api_ = er_rest_api.rest_api.start_rest_server(self.monitorApp, self, port=self.rest_api_port)

        # monitor function to monitor and trigger scaling
        self.monitor_thread = hub.spawn(self._monitor)

        # parse nffg to start Ctrl app
        self.gui_log_sender.send_string('start ER')
        self.parse_nffg(self.nffg)


    def parse_nffg(self, nffg):
        # check nffg and parse the ovs instances deployed
        nffg_DPs = er_nffg.process_nffg(nffg)
        self.logger.info('parsed DPs: {0}'.format(nffg_DPs))
        if len(nffg_DPs) == 0:
            self.logger.info('no DPs found in nffg: {0}'.format(nffg))
        for DP_name in nffg_DPs:
            # if DP already exists in the ctrl app, leave it (otherwise port numbers and registered setting will be lost)
            if DP_name in self.DP_instances:
                continue
            else:
                new_DP = nffg_DPs[DP_name]
                self.DP_instances[DP_name] = new_DP

        # remove deleted switches in the nffg from the elastic router ctrl app
        DP_instance_copy = self.DP_instances.copy() # need copy because dict can be changed in the loop
        for DP_name in DP_instance_copy:
            if DP_name not in nffg_DPs:
                self.DPIDtoDP.pop(self.DP_instances[DP_name].datapath_id)
                self.DP_instances.pop(DP_name)
                self.logger.info('Removed DP: {0}'.format(DP_name))

        # set parsed nffg
        graph_dict = build_graph(self.DP_instances, 'gui_server/base_ER.json', output_file='gui_server/parsed_nffg.json')
        self.gui_log_sender.send_string('parsed nffg')

    # monitor stats and trigger scaling
    def _monitor(self):
        while True:
            # check if all switches are detected
            registered_DPs = filter(lambda x: self.DP_instances[x].registered is True, self.DP_instances)
            #self.logger.info('{0} switches detected'.format(len(registered_DPs)))
            for DP_name in registered_DPs:
                self.logger.info('{0} detected'.format(DP_name))

            unregistered_DPs = filter(lambda x: self.DP_instances[x].registered is False, self.DP_instances)
            for DP_name in unregistered_DPs:
                self.logger.info('{0} not detected'.format(DP_name))


            # ask port statistics
            self.monitorApp.init_measurement()

            # check if measurements are valid
            if not self.monitorApp.check_measurement():
                hub.sleep(1)
                continue

            # do measurements (cpu, ingress rate)
            self.monitorApp.do_measurements()


            # print some statistics
            DP_rate_string_html = ''
            for DP_name in self.DP_instances:
                DP = self.DP_instances.get(DP_name)
                DP_rate_string = "{0} total ingress rate: {1:.2f} pps".format(DP_name, self.monitorApp.DP_ingress_rate[DP_name])
                self.logger.info(DP_rate_string)
                DP_rate_string_html += DP_rate_string + '<br>'

            total_rate = "total ingress rate: {0:.2f} pps".format(self.monitorApp.complete_ingress_rate)
            total_rx_bytes = "total rx traffic:{0:.2f} MB".format(self.monitorApp.total_rx_bytes/(10**6))

            self.logger.info(total_rate + '\n' + total_rx_bytes)

            total_rate_html = DP_rate_string_html + '<br>' + total_rate + '<br>' + total_rx_bytes
            self.gui_count_sender.send_string(total_rate_html)


            # not needed for UNIFY version
            if self.auto_scale:
                scale_direction = None
                # check if scaling is needed
                scaling_ports = self.monitorApp.check_scaling_in()
                if len(scaling_ports) > 0:
                    scale_direction = 'in'
                else:
                    scaling_ports = self.monitorApp.check_scaling_out()
                    if len(scaling_ports) > 0:
                        scale_direction = 'out'

                if scale_direction:
                    #start_time = time.time()
                    self.VNFs_to_be_deleted = self.scale(scaling_ports,scale_direction)
                    #self.monitorApp.scaling_lock.acquire()
                    #self.monitorApp.scaling_lock.release()
                    #scaling_time = time.time() - start_time
                    #self.logger.info('scaling in finished ({0} seconds)'.format(round(scaling_time, 2)))

            hub.sleep(2)

    def scale(self, scaling_ports, direction):

        self.scale_dir = direction
        scale_log_string = "scale {0} started!".format(direction)
        self.logger.info(scale_log_string)
        self.gui_log_sender.send_string(scale_log_string)
        self.gui_nffg1_sender.send_string('parsed_nffg.json')

        # need to scale both nffg and internal objects
        # because we need the translation between old and new DPs
        # list of DPs to add
        new_DP_list = []

        # translate old port to new port in scaled topology
        scale_port_dict = {}

        # new DP for each scaled port
        id_add = 1 # constant to make the correct vnf id
        id = int(er_nffg.get_next_ovs_id(self.nffg, add=id_add))
        for port_list in scaling_ports:
            # we need to artificially pick the id, because UN does not allow
            # multiple instances of the same vnf type

            #id = len(self.DP_instances) + 1
            #id = int(er_nffg.get_next_vnf_id(self.nffg, add=id_add))-1  # control vnf id=1, so ovs id is one less...
            #id_nffg = er_nffg.get_next_vnf_id(self.nffg, add=id_add)
            ovs_id = id + id_add - 2
            DPname = 'ovs{0}'.format(ovs_id)


            # create new DP, ports will be added later
            new_DP = DP(DPname, id + id_add - 1, [])
            #new_DP = DP(DPname, ovs_id, [])
            self.DP_instances[new_DP.name] = new_DP

            # add external ports/links to new DP
            for i in range(0, len(port_list)):
                new_ifname = '{0}_eth{1}'.format(DPname,i)
                old_port = port_list[i]
				
                new_port = new_DP.add_port(new_ifname, port_type=DPPort.External, linked_port=old_port.linked_port)

                scale_port_dict[old_port] = new_port

            new_DP_list.append(new_DP)

            id_add = id_add + 1


        # add internal ports/links to new DP
        for new_DP in new_DP_list:

            # need all scaled out ports for translation of oftable
            new_DP.scale_port_dict = scale_port_dict

            other_DPs = [new_DP2 for new_DP2 in new_DP_list if new_DP.name != new_DP2.name]
            for linked_DP in other_DPs:
                # check if DPs are already connected
                if new_DP.check_connected(linked_DP):
                    continue
                # link 2 DPs with new ports
                index1 = len(new_DP.ports)
                ifname1 = '{0}_eth{1}'.format(new_DP.name,index1)
                index2 = len(linked_DP.ports)
                ifname2 = '{0}_eth{1}'.format(linked_DP.name,index2)
                new_DP.add_port(ifname1, port_type=DPPort.Internal)
                linked_port1 = new_DP.get_port(ifname1)
                linked_DP.add_port(ifname2, port_type=DPPort.Internal, linked_port=linked_port1)
                linked_port2 = linked_DP.get_port(ifname2)
                new_DP.get_port(ifname1).linked_port = linked_port2
                self.logger.info('linked {0} to {1}'.format(ifname1, ifname2))

        # When all internal/external ports are created
        # set the forwarding links to the external ports for each internal port on the same DP
        for new_DP in new_DP_list:
            port_list = new_DP.ports
            external_ports = [port for port in port_list if port.port_type == DPPort.External]
            internal_ports = [port for port in port_list if port.port_type == DPPort.Internal]
            other_DPs = [new_DP2 for new_DP2 in new_DP_list if new_DP.name != new_DP2.name]
            for ext_port in external_ports:
                for linked_DP in other_DPs:
                    for int_port in internal_ports:
                        # find first available free internal port to link to ext port
                        if int_port.linked_port.DP.name == linked_DP.name and int_port.forward_extport is None:
                            int_port.forward_extport = ext_port
                            break

        self.gui_log_sender.send_string('create intermediate nffg')
        # add new DP to intermediate nffg
        # clean all existing vnfs and flowentries

        #self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)
        nffg_intermediate = (copy.deepcopy(self.nffg))
        nffg_intermediate = er_nffg.remove_measure_to_ovs_vnfs(nffg_intermediate)

        for new_DP in new_DP_list:
            ovs_id = int(new_DP.id) - 1
            #nffg_id = new_DP.name
            nffg_id = int(new_DP.id) 
            #nffg_intermediate = er_nffg.add_ovs_vnf(nffg_intermediate, nffg_id, ovs_id, new_DP.name, vnftype='ovs',
            #                                        numports=len(new_DP.ports))
            nffg_intermediate = er_nffg.add_ovs_vnf(nffg_intermediate, nffg_id, ovs_id, new_DP.name, vnftype=new_DP.name, numports=len(new_DP.ports))


        '''
        intermediate_file = 'er_nffg_scale_{0}_intermediate_base.json'.format(direction)
        nffg_intermediate = open(intermediate_file).read()
        '''

        # only add internal links to nffg_intermediate base
        # add internal links
        for new_DP in new_DP_list:
            # add internal links
            internal_ports = [port for port in new_DP.ports if port.port_type == DPPort.Internal]
            for port in internal_ports:
                port_in = port
                port_out = port.linked_port

                nffg_intermediate = er_nffg.add_flowentry(nffg_intermediate, port_in, port_out)


            # add flow entries for external ports, but first with lower priority
            # at scale out finish, delete and add with correct priority
            external_ports = [port for port in new_DP.ports if port.port_type == DPPort.External]
            for port in external_ports:
                port_in = port
                port_SAP = port.linked_port

                # only need outgoing flows for the new DPs
                nffg_intermediate = er_nffg.add_flowentry_SAP(nffg_intermediate, port_in, port_SAP, priority=10)
                #nffg_intermediate = er_nffg.add_flowentry_SAP(nffg_intermediate, port_SAP, port_in, priority=9)



        #intermediate_file = 'ER_scale_{0}_intermediate.json'.format(direction)
        intermediate_file = 'ER_scale_{0}_intermediate.xml'.format(direction)
        file = open(intermediate_file, 'w')
        file.write(nffg_intermediate)
        file.close()

        self.gui_log_sender.send_string('send intermediate nffg')
        graph_dict = build_graph_list(new_DP_list, 'gui_server/parsed_nffg.json', y_offset=4)
        self.gui_nffg2_sender.send_string('parsed_nffg.json')

        #add the final measure data
        #nffg_intermediate = er_nffg.add_measure_to_ovs_vnfs(nffg_intermediate, self.scale_dir)

        # send via cf-or
        self.nffg = er_nffg.send_nffg(self.REST_Cf_Or, nffg_intermediate)


        self.gui_log_sender.send_string('scale intermediate')
        self.gui_log_sender.send_string('start routing table state transfer')

        VNFs_to_be_deleted = []
        for old_port in scale_port_dict:
            if old_port.DP.name not in VNFs_to_be_deleted:
                VNFs_to_be_deleted.append(old_port.DP.name)

        self.logger.info("VNFs to delete: {0}".format(VNFs_to_be_deleted))
        return VNFs_to_be_deleted


    def scale_finish(self):

        log_string = 'set new flows with higher priority'
        self.logger.info(log_string)
        self.gui_log_sender.send_string(log_string)

        # grey-out edges
        change_graph(base_file='gui_server/parsed_nffg.json', edges=self.VNFs_to_be_deleted, color='grey',
                     output_file='gui_server/intermediate_nffg.json')
        new_DPs = [DP_name for DP_name in self.DP_instances if DP_name not in self.VNFs_to_be_deleted]
        change_graph(base_file='gui_server/intermediate_nffg.json', edges=new_DPs, color='red')
        self.gui_nffg2_sender.send_string('intermediate_nffg.json')

        ###self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)

        # fix priorities of new flow entries to SAPs
        # only incoming flows matter
        for new_DP in new_DPs:
            external_ports = [port for port in self.DP_instances[new_DP].ports if port.port_type == DPPort.External]
            for port in external_ports:
                port_in = port
                port_SAP = port.linked_port
                self.nffg = er_nffg.add_flowentry(self.nffg, port_SAP, port_in, priority=11)
        #self.nffg = er_nffg.add_duplicate_flows_with_priority(self.nffg, old_priority=9, new_priority=11)

        #file = open('ER_scale_priorities1.xml', 'w')
        #file.write(self.nffg)
        #file.close()

        self.nffg = er_nffg.send_nffg(self.REST_Cf_Or, self.nffg)
        # need some time here to install flows, otherwise packet loss
        hub.sleep(5)

        log_string = 'delete old VNFs'
        self.logger.info(log_string)
        self.gui_log_sender.send_string(log_string)

        # delete the old intermediate VNFs
        # first delete external incoming flows for all old DPs (scaled out topo)
        ###self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)
        for del_VNF in self.VNFs_to_be_deleted:
            #VNF_id = self.DP_instances[del_VNF].name
            VNF_id = self.DP_instances[del_VNF].id
            self.nffg = er_nffg.delete_VNF_incoming_ext_flows(self.nffg, VNF_id)


        # TODO wait for state transfer to be completed (any state change after new DP were registered)

        # delete other flows and complete VNF
        # self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)
        for del_VNF in self.VNFs_to_be_deleted:
            #VNF_id = self.DP_instances[del_VNF].name
            VNF_id = int(self.DP_instances[del_VNF].id)
            self.nffg = er_nffg.delete_VNF(self.nffg, str(VNF_id))

        #file = open('ER_scale_priorities2.xml', 'w')
        #file.write(self.nffg)
        #file.close()

        self.nffg = er_nffg.send_nffg(self.REST_Cf_Or, self.nffg)
        # need some time here to delete all flows, otherwise packet loss
        #hub.sleep(5)

        graph_dict = change_graph(base_file='gui_server/intermediate_nffg.json', nodes=self.VNFs_to_be_deleted)
        self.gui_nffg2_sender.send_string('intermediate_nffg.json')

        scale_log_string = 'add new flows with priority 10'
        self.logger.info(scale_log_string)
        self.gui_log_sender.send_string(scale_log_string)

        self.VNFs_to_be_deleted = []
        #self.scaled_nffg = None
        #self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)
        self.parse_nffg(self.nffg)

        # fix priorities of new flow entries to SAPs
        #new_nffg = er_nffg.add_duplicate_flows_with_priority(self.nffg, old_priority=9, new_priority=10)
        new_nffg = er_nffg.add_duplicate_flows_with_priority(self.nffg, old_priority=11, new_priority=10)

        #file = open('ER_scale_priorities3.xml', 'w')
        #file.write(new_nffg)
        #file.close()

        new_nffg = er_nffg.send_nffg(self.REST_Cf_Or, new_nffg)
        # need some time here to install flows, otherwise packet loss
        hub.sleep(1)

        scale_log_string = 'delete old flows with priority 9-11'
        self.logger.info(scale_log_string)
        self.gui_log_sender.send_string(scale_log_string)

        #new_nffg = er_nffg.get_nffg(self.REST_Cf_Or)
        #need some time here to install flows, otherwise packet loss
        #hub.sleep(5)
        #new_nffg = er_nffg.delete_flows_by_priority(new_nffg, 9)
        new_nffg = er_nffg.delete_flows_by_priority(new_nffg, 11)

        #file = open('ER_scale_finish.xml', 'w')
        #file.write(new_nffg)
        #file.close()

        self.logger.info('restored priorities of flow entries to 10')
        # er_nffg.send_nffg(self.REST_Cf_Or ,er_nffg.remove_quotations_from_ports(new_nffg))

        #add the final measure data
        #new_nffg = er_nffg.add_measure_to_ovs_vnfs(new_nffg)
        new_nffg = er_nffg.add_measure_to_ovs_vnfs(new_nffg, self.scale_dir)

        file = open('ER_scale_finish-with-measure.xml', 'w')
        file.write(new_nffg)
        file.close()

        # send the final nffg
        self.nffg = er_nffg.send_nffg(self.REST_Cf_Or, new_nffg)

        #self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)

        self.logger.info('scaling finished!')
        self.gui_log_sender.send_string('scaling finished')
        self.gui_nffg3_sender.send_string('parsed_nffg.json')



    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        # install table-miss flow entry
        #
        # We specify NO BUFFER to max_len of the output action due to
        # OVS bug. At this moment, if we specify a lesser number, e.g.,
        # 128, OVS will send Packet-In with invalid buffer_id and
        # truncated packet data. In that case, we cannot output packets
        # correctly.  The bug has been fixed in OVS v2.1.0.
        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        # this flow entry is part of the default flow entry settings
        #self.add_flow(datapath, 0, match, actions)

    @set_ev_cls(EventSwitchLeave)
    def _ev_switch_leave_handler(self, ev):
        datapath = ev.switch.dp
        #this_DP = self.DPIDtoDP[datapath.id]
        #self.DP_instances.pop(this_DP.name)
        #self.logger.info('Removed DP: {0}'.format(this_DP.name))

    # new switch detected
    @set_ev_cls([EventSwitchEnter, EventSwitchReconnected])
    def _ev_switch_enter_handler(self, ev):
        datapath = ev.switch.dp
        self.logger.info('registered OF switch id: %s' % datapath.id)
        ofproto = datapath.ofproto
        self.logger.info('OF version: {0}'.format(ofproto))
        #print 'switch entered send port desc request'

        self.send_port_desc_stats_request(datapath)

    # query ports of new detected switch
    def send_port_desc_stats_request(self, datapath):
        ofp_parser = datapath.ofproto_parser
        req = ofp_parser.OFPPortDescStatsRequest(datapath, 0)
        datapath.send_msg(req)

    # register this switch and get assigned port numbers
    @set_ev_cls(ofp_event.EventOFPPortDescStatsReply, MAIN_DISPATCHER)
    def port_desc_stats_reply_handler(self, ev):
        ports = []
        this_DP = None
        # TODO check ovs instances by datapath id instead of name deduction from port name
        for p in ev.msg.body:
            # skip LOCAL port
            if p.port_no > 100 :
                continue
            ovs_name = DPPort.get_datapath_name(p.name)
            self.logger.info('found OVS name:{0} port: {1}'.format(ovs_name, p.name))
            this_DP = self.DP_instances.get(ovs_name)

            #wait until orchestrator reports this DP in the nffg
            while not this_DP:
                #if not this_DP:
                self.logger.info('found OVS name:{0} not in DP_instances'.format(ovs_name))
                time.sleep(1)
                # parse the nffg from the Cf-Or
                self.nffg = er_nffg.get_nffg(self.REST_Cf_Or)
                self.parse_nffg(self.nffg)
                this_DP = self.DP_instances.get(ovs_name)
                #continue

            # set port number assigned by orchestrator to this interface
            port = this_DP.get_port(p.name)
            port.number = p.port_no

            ports.append('port_no=%d hw_addr=%s name=%s config=0x%08x '
                         'state=0x%08x curr=0x%08x advertised=0x%08x '
                         'supported=0x%08x peer=0x%08x curr_speed=%d '
                         'max_speed=%d' %
                         (p.port_no, p.hw_addr,
                          p.name, p.config,
                          p.state, p.curr, p.advertised,
                          p.supported, p.peer, p.curr_speed,
                          p.max_speed))
        #self.logger.debug('OFPPortDescStatsReply received: %s', ports)

        if not this_DP:
            return

        this_DP.datapath = ev.msg.datapath
        this_DP.datapath_id = ev.msg.datapath.id
        this_DP.registered = True
        self.DPIDtoDP[this_DP.datapath_id] = this_DP
        self.logger.info('stored OF switch id: %s' % this_DP.datapath_id)
        self.gui_log_sender.send_string("registered new OVS: {0}".format(ovs_name))

        # fill switch flow entries
        self.send_oftable(this_DP)

    # query port statistics
    def port_stats_request(self, datapath):
        #print 'request stats'
        #self.logger.info('send stats request: %016x', datapath.id)
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        req = parser.OFPPortStatsRequest(datapath, 0, ofproto.OFPP_ANY)
        datapath.send_msg(req)

    @set_ev_cls(ofp_event.EventOFPPortStatsReply, MAIN_DISPATCHER)
    def _port_stats_reply_handler(self, ev):
        #print 'receive stats'
        body = ev.msg.body

        dpid = ev.msg.datapath.id
        this_DP = self.DPIDtoDP[dpid]


        for stat in sorted(body, key=attrgetter('port_no')):
            #skip control port
            if stat.port_no > 20 :
                continue
            #self.logger.debug('port of this DP: {0} port: {1}'.format(this_DP.name, stat.port_no))
            '''
            self.logger.info('%016x %8x %8d %8d %8d %8d %8d %8d',
                             ev.msg.datapath.id, stat.port_no,
                             stat.rx_packets, stat.rx_bytes, stat.rx_dropped,
                             stat.tx_packets, stat.tx_bytes, stat.tx_dropped)
            '''

            #calculate time delta since last measurement
            port_uptime  = stat.duration_sec + stat.duration_nsec * 10**(-9)
            this_DP.previous_monitor_time.setdefault(stat.port_no,0)
            previous_time = this_DP.previous_monitor_time[stat.port_no]
            if previous_time <= 0 or previous_time >= port_uptime:
               # first measurement
               #print 'ER: {0}-{1}'.format(ER.name, stat.port_no)
               #print 'first measurement'
               #print 'previous time: {0}'.format(previous_time)
               #print 'port_uptime: {0}'.format(port_uptime)
               this_DP.port_txstats_packets[stat.port_no] = stat.tx_packets
               this_DP.port_rxstats_packets[stat.port_no] = stat.rx_packets
               this_DP.port_txstats_bytes[stat.port_no] = stat.tx_bytes
               this_DP.port_rxstats_bytes[stat.port_no] = stat.rx_bytes
               this_DP.previous_monitor_time[stat.port_no] = port_uptime
               return
            else :
               time_delta = (port_uptime - previous_time)
               #print 'ER: {0}-{1}'.format(ER.name, stat.port_no)
               #self.logger.debug('time delta: {0}'.format(time_delta))
               #print 'previous time: {0}'.format(previous_time)
               #print 'port_uptime: {0}'.format(port_uptime)


            if this_DP and stat.port_no in this_DP.port_txstats_packets :
               this_DP.port_txrate_packets[stat.port_no] = (stat.tx_packets - this_DP.port_txstats_packets[stat.port_no])/float(time_delta)
               #print(this_DP.port_txrate[stat.port_no])
            if this_DP and stat.port_no in this_DP.port_rxstats_packets :
               this_DP.port_rxrate_packets[stat.port_no] = (stat.rx_packets - this_DP.port_rxstats_packets[stat.port_no])/float(time_delta)
               '''
               self.logger.debug('{1} {2} rx rate: {0}'.format(this_DP.port_rxrate[stat.port_no], this_DP.name, \
                                                               this_DP.get_port_by_number(stat.port_no).ifname))
               '''
            if this_DP and stat.port_no in this_DP.port_txstats_bytes:
                this_DP.port_txrate_bytes[stat.port_no] = (stat.tx_bytes - this_DP.port_txstats_bytes[stat.port_no]) / float(time_delta)
                # print(this_DP.port_txrate[stat.port_no])
            if this_DP and stat.port_no in this_DP.port_rxstats_bytes:
                this_DP.port_rxrate_bytes[stat.port_no] = (stat.rx_bytes - this_DP.port_rxstats_bytes[stat.port_no]) / float(time_delta)
                # keep couner of total received bytes
                self.monitorApp.total_rx_bytes += this_DP.port_rxrate_bytes[stat.port_no]

            this_DP.port_txstats_packets[stat.port_no] = stat.tx_packets
            this_DP.port_rxstats_packets[stat.port_no] = stat.rx_packets
            this_DP.port_txstats_bytes[stat.port_no] = stat.tx_bytes
            this_DP.port_rxstats_bytes[stat.port_no] = stat.rx_bytes
            this_DP.previous_monitor_time[stat.port_no] = port_uptime




    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        # If you hit this you might want to increase
        # the "miss_send_length" of your switch
        if ev.msg.msg_len < ev.msg.total_len:
            self.logger.debug("packet truncated: only %s of %s bytes",
                              ev.msg.msg_len, ev.msg.total_len)
        msg = ev.msg
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match['in_port']
        priority = 20

        pkt = packet.Packet(msg.data)
        '''
        for p in pkt.protocols:
            print p
        print ip_proto
        print pkt.get_protocol(ip_proto)
        '''
        eth = pkt.get_protocols(ethernet.ethernet)[0]
        mac_dst = eth.dst
        mac_src = eth.src
        dpid = datapath.id
        self.logger.info("packet in {0} {1} {2} {3} {4}".format(dpid, mac_src, mac_dst, in_port, eth.ethertype))

        dpid = datapath.id
        if dpid not in self.DPIDtoDP:
            self.logger.info( 'exception in packet_in handler: DPID: {0} not registered'.format(dpid))
            return
        source_DP = self.DPIDtoDP[dpid]
        # monitor packet_in rate per port
        #source_DP.packet_in_port_count.setdefault(in_port, 0)
        #source_DP.packet_in_port_count[in_port] += 1
        #self.logger.debug("packet in count port:{0} = {1}".format(in_port, source_DP.packet_in_port_count[in_port]))

        #source_ER.mac_to_port.setdefault(dpid, {})

        #learn a mac address to avoid FLOOD next time.
        self.logger.info("adding flows via packet_in is disabled to limit the number of flow entries")

        if mac_src not in source_DP.mac_to_port:
            #learn mac address and set tables

            source_DP.mac_to_port[mac_src] = in_port

            actions = [parser.OFPActionOutput(int(in_port))]
            match_dict = create_dictionary(eth_dst=mac_src)
            source_DP.oftable.append((match_dict, actions, priority))
            match = parser.OFPMatch(**match_dict)
            #self.add_flow(datapath, priority, match, actions)
            #self.logger.debug('added flow: DP: {2} mac_src:{0} out_port:{1}'.format(
            #        mac_src, in_port, source_DP.name))

            #learn in all connected DPs:
            internal_ports = [port for port in source_DP.ports if port.port_type == DPPort.Internal]
            for int_port in internal_ports:
                in_port2 = int_port.linked_port.number
                if in_port2 is None:
                    #exception, linked DP not registered yet
                    continue
                linked_DP = int_port.linked_port.DP
                linked_DP.mac_to_port[mac_src] = in_port2

                actions = [parser.OFPActionOutput(int(in_port2))]
                match_dict = create_dictionary(eth_dst=mac_src)
                linked_DP.oftable.append((match_dict, actions, priority))
                match = parser.OFPMatch(**match_dict)
                #self.add_flow(linked_DP.datapath, priority, match, actions)
                #self.logger.debug('added flow: DP: {2} mac_dst:{0} out_port:{1}'.format(
                #    mac_src, in_port2, linked_DP.name))

        # TODO add_flow for each mac src found in every DP

        #self.logger.info('{0} {1}'.format(source_DP.name, source_DP.mac_to_port))

        #no FLOOD -> loops in topology!

        if mac_dst in source_DP.mac_to_port:
            out_port = source_DP.mac_to_port[mac_dst]
        else:
            out_port = ofproto.OFPP_FLOOD
            #self.logger.info('flood packet')

        actions = [parser.OFPActionOutput(out_port)]
        # install a flow to avoid packet_in next time
        if out_port != ofproto.OFPP_FLOOD:

            #match_dict = create_dictionary(in_port=in_port, eth_dst=mac_dst)
            match_dict = create_dictionary(eth_dst=mac_dst)
            source_DP.oftable.append((match_dict, actions, priority))
            # self.logger.debug('added flow: DP: {3} in_port:{0} mac_dst:{1} out_port:{2}'.format(
            #    in_port, mac_dst, out_port, source_DP.name))
            self.logger.debug('added flow: DP: {2} mac_dst:{0} out_port:{1}'.format(
                mac_dst, out_port, source_DP.name))
            match = parser.OFPMatch(**match_dict)
            #match = parser.OFPMatch(in_port=in_port, eth_dst=mac_dst)

            # verify if we have a valid buffer_id, if yes avoid to send both
            # flow_mod & packet_out
            if msg.buffer_id != ofproto.OFP_NO_BUFFER:
                self.logger.info("adding flows via packet_in is disabled to limit the number of flow entries")
                #self.add_flow(datapath, priority, match, actions, msg.buffer_id)
                return
            else:
                self.logger.info("adding flows via packet_in is disabled to limit the number of flow entries")
                #self.add_flow(datapath, priority, match, actions)


        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

        out = parser.OFPPacketOut(datapath=datapath, buffer_id=msg.buffer_id,
                                  in_port=in_port, actions=actions, data=data)
        datapath.send_msg(out)

        return



        pkt_ethernet = pkt.get_protocol(ethernet.ethernet)
        if not pkt_ethernet:
            # which packet is this?
            self.logger.info('This is no ethernet packet')
            return
        pkt_arp = pkt.get_protocol(arp.arp)
        if pkt_arp:
            self.logger.info('received ARP packet')
            self._handle_arp(source_DP, in_port, pkt_ethernet, pkt_arp, msg)
            return

        pkt_ipv4 = pkt.get_protocol(ipv4.ipv4)
        if pkt_ipv4:
            self.logger.info('received ipv4 packet')
            dst = pkt_ipv4.dst
            '''
            dst_SAP = self.IP_to_SAP[dst]
            dst_port = source_DP.SAP_to_port[dst_SAP].number
            actions = [parser.OFPActionOutput(dst_port)]
            #actions[0].port = 10
            '''
            ip_proto = pkt_ipv4.proto
            #match = self.getFullMatch(msg)

            pkt_tcp = pkt.get_protocol(tcp.tcp)
            pkt_udp = pkt.get_protocol(udp.udp)
            pkt_icmp = pkt.get_protocol(icmp.icmp)
            if pkt_tcp:
                self.logger.info('received tcp packet')
                L4_pkt = pkt_tcp
                L4_src = L4_pkt.src_port
                L4_dst = L4_pkt.dst_port
                match_dict = create_dictionary(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                                    ip_proto=ip_proto, tcp_src=L4_src, tcp_dst=L4_dst)
                #match = parser.OFPMatch(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                #                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                #                    ip_proto=ip_proto, tcp_src=L4_src, tcp_dst=L4_dst)
                priority = 10
            elif pkt_udp:
                self.logger.info('received udp packet')
                L4_pkt = pkt_udp
                L4_src = L4_pkt.src_port
                L4_dst = L4_pkt.dst_port
                match_dict = create_dictionary(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                                    ip_proto=ip_proto, udp_src=L4_src, udp_dst=L4_dst)

                #match = parser.OFPMatch(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                #                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                #                    ip_proto=ip_proto, udp_src=L4_src, udp_dst=L4_dst)


                #match = getFullMatch(msg)
                priority = 10
            elif pkt_icmp:
                self.logger.info('received icmp packet')
                match_dict = create_dictionary(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                                    ip_proto=ip_proto)
                #match = parser.OFPMatch(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                #                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                #                    ip_proto=ip_proto)
            else:
                self.logger.info('received other packet')
                match_dict = create_dictionary(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                                    ip_proto=ip_proto)
                #match = parser.OFPMatch(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                #                    eth_type=ether.ETH_TYPE_IP,ipv4_dst=pkt_ipv4.dst, ipv4_src=pkt_ipv4.src,
                #                    ip_proto=ip_proto)
                #match = getFullMatch(msg)


            #add flow
            #source_DP.oftable.append((match_dict, actions, priority))
            match = parser.OFPMatch(**match_dict)
            if msg.buffer_id != ofproto.OFP_NO_BUFFER:
                self.add_flow(datapath, priority, match, actions, msg.buffer_id)
                return
            else:
                self.add_flow(datapath, priority, match, actions)

            #send packet out
            data = None
            if msg.buffer_id == ofproto.OFP_NO_BUFFER:
                data = msg.data
            out = parser.OFPPacketOut(datapath=datapath, buffer_id=msg.buffer_id,
                                      in_port=in_port, actions=actions, data=data)
            datapath.send_msg(out)
            return

        pkt_lldp = pkt.get_protocol(lldp.lldp)
        if pkt_lldp:
            self.logger.info('received lldp packet (unhandled)')
            match_dict = create_dictionary(in_port=in_port, eth_dst=mac_dst, eth_src=mac_src,
                                           eth_type=ether.ETH_TYPE_LLDP)
            return

        pkt_ipv6 = pkt.get_protocol(ipv6.ipv6)
        if pkt_ipv6:
            self.logger.info('unhandled ipv6 packet_in message on {0} port {1}'.format(self.DPIDtoER[datapath.id].name, in_port))
            for p in pkt.protocols:
                self.logger.info('protocol:{0}'.format(p))
        else:
            self.logger.info('unhandled packet_in message')
            self.logger.info("packet in %s %s %s %s %s", dpid, mac_src, mac_dst, in_port, eth.ethertype)
            for p in pkt.protocols:
                self.logger.info('protocol:{0}'.format(p))

    def _handle_arp(self, source_DP, in_port, pkt_ethernet, pkt_arp, msg):
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        dst = pkt_ethernet.dst
        out_port = []
        if dst in source_DP.mac_to_port:
            out_port.append(source_DP.mac_to_port[dst])
        else:
            # TODO no FLOOD -> loops in topology!
            #input in internal port = output only via external port and vice versa
            out_port.append(ofproto.OFPP_FLOOD)
            #print source_ER.portno_to_type
            '''
            if source_DP.portno_to_type[in_port] == ERPort.External:
                out_port.append(ofproto.OFPP_FLOOD)
            elif source_DP.portno_to_type[in_port] == ERPort.Internal:
                for port in source_DP.portno_to_type:
                    if source_DP.portno_to_type[port] == ERPort.External:
                        out_port.append(port)
            '''
        actions = []
        for port in out_port:
            actions.append(parser.OFPActionOutput(port))

        # install a flow to avoid packet_in next time
        priority = 1
        # TODO only install flow entry for ARP?
        #match_dict = create_dictionary(eth_type=ether.ETH_TYPE_ARP,in_port=in_port, eth_dst=dst)
        match_dict = create_dictionary(in_port=in_port, eth_dst=dst)
        #source_DP.oftable.append((match_dict, actions, priority))
        match = parser.OFPMatch(**match_dict)
        #match = parser.OFPMatch(eth_type=ether.ETH_TYPE_ARP,in_port=in_port, eth_dst=dst)
        #match = self.getFullMatch(msg)
        # verify if we have a valid buffer_id, if yes avoid to send both
        # flow_mod & packet_out
        if msg.buffer_id != ofproto.OFP_NO_BUFFER:
            self.add_flow(datapath, priority, match, actions, msg.buffer_id)
            return
        else:
            self.add_flow(datapath, priority, match, actions)

        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

        out = parser.OFPPacketOut(datapath=datapath, buffer_id=msg.buffer_id,
                                  in_port=in_port, actions=actions, data=data)
        datapath.send_msg(out)

    def _send_packet(self, datapath, port, pkt):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        pkt.serialize()
        #self.logger.info("packet-out %s" % (pkt,))
        data = pkt.data
        actions = [parser.OFPActionOutput(port=port)]
        out = parser.OFPPacketOut(datapath=datapath,
                                  buffer_id=ofproto.OFP_NO_BUFFER,
                                  in_port=ofproto.OFPP_CONTROLLER,
                                  actions=actions,
                                  data=data)
        datapath.send_msg(out)


    def add_flow(self, datapath, priority, match, actions, buffer_id=None, cookie=0):

        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,actions)]
        if buffer_id:
            mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id,
                                    priority=priority, match=match,
                                    instructions=inst, cookie=cookie)
        else:
            mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                    match=match, instructions=inst, cookie=cookie)

        datapath.send_msg(mod)


    def send_oftable(self, DP):

        DP.set_default_oftable_scale()
        DP.translate_oftable_scale()
        DP.set_routing_table(self.config['routing_table'])

        parser = DP.datapath.ofproto_parser
        self.logger.info('{0} adding {1} flows'.format(DP.name,len(DP.oftable)))

        # set cookie to help ATPG tool detect flows
        cookie = 1
        for match_dict, actions, priority  in DP.oftable:
            match = parser.OFPMatch(**match_dict)
            self.add_flow(DP.datapath, priority, match, actions, cookie=cookie)
            cookie += 1

        DP.translate_mactable_scale()


