
from nffg_library.nffg import *
from er_utils import *
import json
from operator import itemgetter
from itertools import *
from ryu.lib import hub

import logging
import xml.etree.ElementTree as ET
import threading
import urllib2

#Set the logger
logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

def process_nffg(nffg_json):

    logging.debug("Reading dict '%s'...", nffg_json)

    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    ovs_instances = {}
    for vnf in nffg.vnfs:
        vnf_name = vnf.name
        logging.debug("found NF: %s", vnf_name)
        if 'ovs' in vnf_name:
            ovsName = vnf_name
            ovsId = vnf.id
            new_DP = DP(ovsName, ovsId)
            ovs_instances[ovsName] = new_DP
            logging.debug("found ovs NF: %s", ovsName)
            #ovs_instances.setdefault(ovsName, {'id': 0, 'ports': []})
            #ovs_instances[ovsName]['id'] = ovsId
            for port in vnf.ports:
                # do not add control port of the DP
                if 'control' in port.name: continue

                portName = port.name
                portId = port.id
                new_port = DPPort(portName, portId, DP_parent=new_DP)
                #ovs_instances[ovsName]['ports'].append(new_port)
                new_DP.ports.append(new_port)

                logging.debug("found ovs port: %s with ovs id: %s", portName, ovsId)

    # first make all the ovs instances with all the ports,
    # then fill the linked ports
    # this function is only used to parse the first nffg, external ports only
    for ovs_name in ovs_instances:
        #ovsId = ovs_instances[ovs_name]['id']
        ovsId = ovs_instances[ovs_name].id
        #for port in ovs_instances[ovs_name]['ports']:
        for port in ovs_instances[ovs_name].ports:
            portId = port.id
            flowrules = nffg.getFlowRulesSendingTrafficFromPort(ovsId, portId)
            for flowrule in flowrules:
                # assume only one action
                port_in = flowrule.actions[0].output
                tokens = port_in.split(':')
                port_in_type = tokens[0]
                port_in_element_id = tokens[1]
                port_in_id = ':'.join(tokens[2:])
                #check if external port
                if port_in_type == 'endpoint':
                    SAP_name =  nffg.getEndPoint(port_in_element_id).name
                    port.port_type = DPPort.External
                    linked_port = DPPort(SAP_name, port_in_id, port_type=DPPort.SAP)
                #check if internal port
                elif port_in_type == 'vnf':
                    vnf =  nffg.getVNF(port_in_element_id)
                    VNF_name = vnf.name
                    if not 'ovs' in VNF_name:
                        continue
                    port_in_name = vnf.getPort(port_in_id).name
                    port.port_type = DPPort.Internal
                    #for port2 in ovs_instances[VNF_name]['ports']:
                    for port2 in ovs_instances[VNF_name].ports:
                        if port2.ifname == port_in_name:
                            linked_port = port2
                    #linked_port = DPPort(port_in_name, port_in_id, port_type=DPPort.Internal)

                port.linked_port = linked_port
                linked_port.linked_port = port

    # TODO after all internal/external ports are known in the NFFG,
    # set the forward_extport for all internal ports
    # choose a free one, when multiple  ext ports are on the same DP
    for ovs_name in ovs_instances:
        port_list = ovs_instances[ovs_name].ports
        external_ports = [port for port in port_list if port.port_type == DPPort.External]
        internal_ports = [port for port in port_list if port.port_type == DPPort.Internal]

        other_DPs = [ovs_name2 for ovs_name2 in ovs_instances if ovs_name != ovs_name2]
        for ext_port in external_ports:
            for ovs_name2 in other_DPs:
                for int_port in internal_ports:
                    # find first available free internal port to link to ext port
                    if int_port.linked_port.DP.name == ovs_name2 and int_port.forward_extport is None:
                        int_port.forward_extport = ext_port
                        break


    return ovs_instances

def add_vnf(nffg_json, id, name, vnftype, numports):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    #new_ovs = VNF()

    return nffg.getJSON()


def get_next_flowrule_id(nffg_json, add=0):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    flow_id_list = []
    for flowrule in nffg.flow_rules:
        id = int(flowrule.id)
        flow_id_list.append(id)

    # http://stackoverflow.com/questions/3149440/python-splitting-list-based-on-missing-numbers-in-a-sequence
    # group the sorted list unitl a value is missing (the deleted vnf id)
    sorted_vnf_id_list = sorted(flow_id_list)
    list = []
    for k, g in groupby(enumerate(sorted_vnf_id_list), lambda (i, x): i - x):
        list.append(map(itemgetter(1), g))

    max_id = max(list[0])

    if max_id == 999999999:
        max_id = 0
    next_id_str = str(max_id+1+add).zfill(9)

    return next_id_str

def get_next_vnf_id(nffg_json, add=0):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    vnf_id_list = []
    for vnf in nffg.vnfs:
        id = int(vnf.id)
        vnf_id_list.append(id)

    # http://stackoverflow.com/questions/3149440/python-splitting-list-based-on-missing-numbers-in-a-sequence
    # group the sorted list unitl a value is missing (the deleted vnf id)
    sorted_vnf_id_list = sorted(vnf_id_list)
    list = []
    for k, g in groupby(enumerate(sorted_vnf_id_list), lambda (i,x):i-x):
        list.append(map(itemgetter(1), g))

    max_id = max(list[0])
    if max_id == 99999999:
        max_id = 0
    next_id_str = str(max_id+1+add).zfill(8)

    return next_id_str


def add_flowentry_SAP(nffg_json, port_in, port_out, priority=10):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    flow_id = get_next_flowrule_id(nffg_json)

    if port_in.port_type == DPPort.SAP and port_out.port_type == DPPort.External:
        match_endpoints = nffg.getEndPointsFromName(port_in.ifname)
        match_entry = 'endpoint:' + match_endpoints[0].id
        new_match = Match(port_in=match_entry)

        action_vnf = nffg.getVNF(port_out.DP.id)
        action_port_id = ''
        for port in action_vnf.ports:
            if port_out.ifname in port.name:
                action_port_id = port.id
                break
        action_entry = 'vnf:' + action_vnf.id + ':' + action_port_id
        new_action = Action(output=action_entry)

    elif port_in.port_type == DPPort.External and port_out.port_type == DPPort.SAP:
        match_vnf = nffg.getVNF(port_in.DP.id)
        match_port_id = ''
        for port in match_vnf.ports:
            if port_in.ifname in port.name:
                match_port_id = port.id
                break
        match_entry = 'vnf:' + match_vnf.id + ':' + match_port_id
        new_match = Match(port_in=match_entry)

        action_endpoints = nffg.getEndPointsFromName(port_out.ifname)
        action_entry = 'endpoint:' + action_endpoints[0].id
        new_action = Action(output=action_entry)

    if 'new_match' in locals() and 'new_action' in locals():
        new_flow_rule = FlowRule(_id=flow_id, priority=priority, match=new_match, actions=[new_action])
        nffg.addFlowRule(new_flow_rule)

    return nffg.getJSON()

#can only be used for internal flow_rule, between ovs's
def add_flowentry(nffg_json, port_in, port_out, priority=10):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    flow_id = get_next_flowrule_id(nffg_json)

    match_vnf = nffg.getVNF(port_in.DP.id)
    #logging.info('id of DP port_in: {0}'.format(port_in.DP.id))
    match_port_id = ''
    for port in match_vnf.ports:
        if port_in.ifname in port.name:
            match_port_id = port.id
            break
    match_entry = 'vnf:' + match_vnf.id + ':' + match_port_id
    new_match = Match(port_in=match_entry)

    action_vnf = nffg.getVNF(port_out.DP.id)
    action_port_id = ''
    for port in action_vnf.ports:
        if port_out.ifname in port.name:
            action_port_id = port.id
            break
    action_entry = 'vnf:' + action_vnf.id + ':' + action_port_id
    new_action = Action(output=action_entry)

    new_flow_rule = FlowRule(_id=flow_id, priority=priority, match=new_match, actions=[new_action])

    nffg.addFlowRule(new_flow_rule)

    return nffg.getJSON()


def delete_VNF_incoming_ext_flows(nffg_json, vnf_id, RESTaddress):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    del_vnf = nffg.getVNF(vnf_id)

    flows_in = nffg.getFlowRulesSendingTrafficToVNF(del_vnf)
    for flow in flows_in:
        if 'endpoint' in flow.match.port_in:
            delete_flowrule(flow.id, RESTaddress)
            logging.info("deleted external flow with port_in: {0}".format(flow.match.port_in))


def delete_VNF(nffg_json, vnf_id, RESTaddress):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    del_vnf = nffg.getVNF(vnf_id)

    flows_in = nffg.getFlowRulesSendingTrafficToVNF(del_vnf)
    flows_out = nffg.getFlowRulesSendingTrafficFromVNF(del_vnf)

    flow_list = flows_in + flows_out

    flow_list_id = []
    for flow in flow_list:
        flow_list_id.append(flow.id)
    logging.info("flows to delete: {0}".format(flow_list_id))

    for flow in list(set(flow_list)):
        delete_flowrule(flow.id, RESTaddress)
        #hub.sleep(0.4)
        logging.info("deleted flow id: {0}".format(flow.id))


def delete_flowrule(rule_id, RESTaddress):
    url = RESTaddress + '/NF-FG/NF-FG/' + rule_id
    req = urllib2.Request(url)
    req.get_method = lambda: 'DELETE'
    response = urllib2.urlopen(req)
    result = response.read()
    #logging.info(result)

def import_json_file(file_name):
    json_file = open(file_name).read()
    json_dict = json.loads(json_file)

    return json_dict


def add_duplicate_flows_with_priority(nffg_json, old_priority, new_priority):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    # get the flowrules to be replaced, with old_priority
    flowrules_to_be_replaced = [flowrule for flowrule in nffg.flow_rules if flowrule.priority == old_priority]

    # clean all existing flowrules from the NFFG
    nffg.flow_rules = []

    # create the new flowrules, with new_priority
    add = 0
    for flowrule in flowrules_to_be_replaced:
        new_flowrule = copy.deepcopy(flowrule)
        new_flowrule.priority = new_priority
        new_flowrule.id = get_next_flowrule_id(nffg_json, add=add)
        nffg.addFlowRule(new_flowrule)
        add = add + 1

    return nffg.getJSON()

def delete_flows_by_priority(nffg_json, priority, RESTaddress):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    # get the flowrules to be replaced, with old_priority
    flowrules_to_be_deleted = [flowrule for flowrule in nffg.flow_rules if flowrule.priority == priority]

    for flowrule in flowrules_to_be_deleted:
        delete_flowrule(flowrule.id, RESTaddress)
        logging.info("deleted flow id: {0} with priority {1} ".format(flowrule.id, priority))

def remove_quotations_from_ports(nffg_json):
    json_dict = json.loads(nffg_json)
    nffg = NF_FG()
    nffg.parseDict(json_dict)

    for vnf in nffg.vnfs:
        for control in vnf.unify_control:
            host_port = control.host_tcp_port
            control.host_tcp_port = int(host_port)
            vnf_port = control.vnf_tcp_port
            control.vnf_tcp_port = int(vnf_port)
            #print host_port

    return nffg.getJSON()

if __name__ == "__main__":
    #json_file = open('er_nffg.json').read()
    json_file = open('ER_scale_finish.json').read()
    #json_file = open('ER_scale_priorities.json').read()

    delete_VNF_incoming_ext_flows(json_file, '00000003', '')

    new_json = remove_quotations_from_ports(json_file)
    delete_flows_by_priority(json_file,9,'')

    DP_switches = process_nffg(json_file)

    vnf_id = get_next_vnf_id(json_file)

    flow_id = get_next_flowrule_id(json_file)
    print 'next flow id: {0}'.format(flow_id)

    #new_nffg = add_flowrule(json_file)

    rib = import_json_file("elastic_router_config.json")
    print rib

    routing_table = rib['routing_table']
    for entry in routing_table:
        for net in entry['net']:
            print net

    print 'end'

