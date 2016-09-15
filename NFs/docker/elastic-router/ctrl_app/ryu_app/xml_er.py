
from virtualizer5.virtualizer import *
from er_utils import *

from operator import itemgetter
from itertools import groupby
import xml.etree.ElementTree as ET
import re

import logging
nffg_log = logging.getLogger(__name__)
nffg_log.setLevel(logging.DEBUG)

from random import randint

# fixed universal node id, always pick this one
NODE_ID = 'UUID11'

# scaled-in version of the NFFG
# nf_id 1 -> ctrl
# nf_id 2 -> ovs1
MEASURE_SCALEIN = (" measurements {"
    "m1 = cpu(vnf = 1);"
    "m2 = cpu(vnf = 2);"
    "m6 = overload.risk.rx(interface = virtual-sap1);"
    "m7 = overload.risk.rx(interface = virtual-sap2);"
    "m8 = overload.risk.rx(interface = virtual-sap3);"
    "m9 = overload.risk.rx(interface = virtual-sap4);"
    "}"
    "zones {"
    """z1 = (AVG(val = m1, max_age = "5 minute") < 0.5);"""
    """z2 = (AVG(val = m2, max_age = "5 minute") > 0.5);"""
    """z3 = (AVG(val = m6, max_age = "5 minute") < 0.5);"""
    """z4 = (AVG(val = m7, max_age = "5 minute") > 0.5);"""
    """z5 = (AVG(val = m8, max_age = "5 minute") < 0.5);"""
    """z6 = (AVG(val = m9, max_age = "5 minute") > 0.5);"""
    "}"
    "actions {"
    """z1->z2 = Publish(topic = "alarms", message = "z1 to z2"); Notify(target = "alarms", message = "z1 to z2");"""
    """z2->z1 = Publish(topic = "alarms", message = "z2 to z");"""
    """->z1 = Publish(topic = "alarms", message = "entered z1");"""
    """z1-> = Publish(topic = "alarms", message = "left z1");"""
    """z1 = Publish(topic = "alarms", message = "in z1");"""
    """z2 = Publish(topic = "alarms", message = "in z2");"""
    """->z3 = Publish(topic = "alarms", message = "entered z3");"""
    """->z4 = Publish(topic = "alarms", message = "entered z4");"""
    """->z5 = Publish(topic = "alarms", message = "entered z5");"""
    """->z6 = Publish(topic = "alarms", message = "entered z6");"""
    "}")

# scaled-out version of the NFFG
# nf_id 1 -> ctrl
# nf_id 3,4,5,6 -> ovs1,2,3,4
MEASURE_SCALEOUT = (" measurements {"
    "m1 = cpu(vnf = 1);"
    "m2 = cpu(vnf = 3);"
    "m3 = cpu(vnf = 4);"
    "m4 = cpu(vnf = 5);"
    "m5 = cpu(vnf = 6);"
    "m6 = overload.risk.rx(interface = virtual-sap1);"
    "m7 = overload.risk.rx(interface = virtual-sap2);"
    "m8 = overload.risk.rx(interface = virtual-sap3);"
    "m9 = overload.risk.rx(interface = virtual-sap4);"
    "}"
    "zones {"
    """z1 = (AVG(val = m1, max_age = "5 minute") < 0.5);"""
    """z2 = (AVG(val = m2, max_age = "5 minute") > 0.5);"""
    """z3 = (AVG(val = m3, max_age = "5 minute") < 0.5);"""
    """z4 = (AVG(val = m4, max_age = "5 minute") > 0.5);"""
    """z5 = (AVG(val = m5, max_age = "5 minute") < 0.5);"""
    """z6 = (AVG(val = m6, max_age = "5 minute") > 0.5);"""
    """z7 = (AVG(val = m7, max_age = "5 minute") > 0.5);"""
    """z8 = (AVG(val = m8, max_age = "5 minute") > 0.5);"""
    """z9 = (AVG(val = m9, max_age = "5 minute") > 0.5);"""
    "}"
    "actions {"
    """z1->z2 = Publish(topic = "alarms", message = "z1 to z2"); Notify(target = "alarms", message = "z1 to z2");"""
    """z2->z1 = Publish(topic = "alarms", message = "z2 to z");"""
    """->z1 = Publish(topic = "alarms", message = "entered z1");"""
    """z1-> = Publish(topic = "alarms", message = "left z1");"""
    """z1 = Publish(topic = "alarms", message = "in z1");"""
    """z2 = Publish(topic = "alarms", message = "in z2");"""
    """->z3 = Publish(topic = "alarms", message = "entered z3");"""
    """->z4 = Publish(topic = "alarms", message = "entered z4");"""
    """->z5 = Publish(topic = "alarms", message = "entered z5");"""
    """->z6 = Publish(topic = "alarms", message = "entered z6");"""
    """->z7 = Publish(topic = "alarms", message = "entered z7");"""
    """->z8 = Publish(topic = "alarms", message = "entered z8");"""
    """->z9 = Publish(topic = "alarms", message = "entered z9");"""
    "}")

def get_virtualizer_nffg(nffg_xml):
    try:
        tree = ET.fromstring(nffg_xml)
    except ET.ParseError as e:
        nffg_log.debug('ParseError: {0}'.format(e.message))
        return 0

    nffg = Virtualizer.parse(root=tree)

    return nffg

def get_UN_node(nffg):
    universal_nodes = nffg.nodes
    # take first node per default
    # alternatively, we can look for UUID11 (as done in UN virtualizer.py)
    for un in universal_nodes:
        un_id = un.id.get_value()
        if NODE_ID in un_id:
            return un

    nffg_log.error('Universal node: {0} not found'.format(NODE_ID))

def get_mapped_port(nffg_xml, vnf_name, int_port):
    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)
    instances = un.NF_instances

    for instance in instances:
        if instance.name.get_value() == vnf_name:
            # only port 0 can have l4 address
            port = instance.ports[str(0)]
            l4_addresses = port.addresses.l4.get_value()
            l4_addresses_list = re.findall("'[a-z]*\/(\d*)'\s*:\s*\('[0-9.]*', (\d*)\)", l4_addresses)
            for vnf_port, host_port in l4_addresses_list:
                if vnf_port == str(int_port):
                    return host_port
        nffg_log.error('port mapping not found for {0} port: {1}'.format(vnf_name, int_port))

def process_nffg(nffg_xml):

    #nffg_log.debug("Reading dict: {0}".format(nffg_xml))

    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)
    ovs_switches = find_ovs(un)


    return ovs_switches

def find_ovs(un):
    """
    find ER data paths (ovs containers) in the topology
    :param un: Universal Node object from the NFFG
    :return:
    """
    ovs_instances = {}
    nf_instances = un.NF_instances
    for nf in nf_instances:
        nf_name = nf.name.get_value()
        nf_type = nf.type.get_value()
        logging.debug("found NF: {0}".format(nf.name.get_value()))
        if 'ovs' in nf_type:
            ovsName = nf.name.get_value()
            ovsId = nf.id.get_value()
            new_DP = DP(ovsName, ovsId)
            ovs_instances[ovsName] = new_DP
            nffg_log.debug("found ovs NF: {0}".format(ovsName))

            for port in nf.ports:
                portName = port.name.get_value()

                # do not add control or public port of the DP
                if 'control' in portName or 'public' in portName: continue

                portId = port.id.get_value()
                new_port = DPPort(portName, portId, DP_parent=new_DP)
                new_DP.ports.append(new_port)

                logging.debug("found ovs port: {0} with ovs id: {1}".format(portName, ovsId))

    # first make all the ovs instances with all the ports,
    # then fill the linked ports
    # this function is only used to parse the first nffg, external ports only
    for ovs_name in ovs_instances:
        ovsId = ovs_instances[ovs_name].id
        for port in ovs_instances[ovs_name].ports:
            portId = port.id
            flowrules = getFlowRulesSendingTrafficFromPort(un, ovsId, portId)

            for flowrule in flowrules:
                # assume only one action
                port_linked = flowrule.out.get_target()
                portPath = flowrule.out.get_target().get_path()
                tokens = portPath.split('/')
                port_linked_type = tokens[4]
                port_linked_id = (port_linked.id.get_value())

                # check if external port
                if port_linked_type == 'ports':
                    SAP_name = port_linked.sap.get_value()
                    port.port_type = DPPort.External
                    linked_port = DPPort(SAP_name, port_linked_id, port_type=DPPort.SAP)
                # check if internal port
                elif port_linked_type == 'NF_instances':
                    vnf = port_linked.get_parent().get_parent()
                    vnf_name = vnf.name.get_value()
                    if not 'ovs' in vnf_name:
                        continue
                    port_linked_name = port_linked.name.get_value()
                    port.port_type = DPPort.Internal
                    for port2 in ovs_instances[vnf_name].ports:
                        if port2.ifname == port_linked_name:
                            linked_port = port2

                port.linked_port = linked_port
                linked_port.linked_port = port

    # after all internal/external ports are known in the NFFG,
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


def get_next_ovs_id(nffg_xml, add=0):

    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    vnf_id_list = []
    vnfs = un.NF_instances
    for vnf in vnfs:
        #id = str(vnf.id.get_value())
        id = str(vnf.name.get_value())
        if 'ovs' not in id: continue
        id_int = int(id.split('ovs')[-1])
        vnf_id_list.append(id_int)

    # http://stackoverflow.com/questions/3149440/python-splitting-list-based-on-missing-numbers-in-a-sequence
    # group the sorted list until a value is missing (the deleted vnf id)
    # so we can re-use the id of the vnf that has been deleted before
    sorted_vnf_id_list = sorted(vnf_id_list)
    list = []
    for k, g in groupby(enumerate(sorted_vnf_id_list), lambda (i,x):i-x):
        list.append(map(itemgetter(1), g))

    if min(list[0]) > 1:
        max_id = 0
    else:
        max_id = max(list[0])

    if max_id == 99999999:
        max_id = 0
    next_id_str = str(max_id+1+add)

    return next_id_str


def add_duplicate_flows_with_priority(nffg_xml, old_priority, new_priority):
    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    # get the flowrules to be replaced, with old_priority
    flowrules_to_be_replaced = [flowrule for flowrule in un.flowtable if str(flowrule.priority.get_value()) == str(old_priority)]

    # clean all existing flowrules from the NFFG
    # nffg.flow_rules = []

    # create the new flowrules, with new_priority
    for flowrule in flowrules_to_be_replaced:
        new_flowrule = copy.deepcopy(flowrule)
        new_flowrule.priority.set_value(str(new_priority))
        new_flowrule.id.set_value(get_next_flowrule_id(nffg_xml))
        new_flowrule.set_operation(operation="create", recursive=False)
        un.flowtable.add(new_flowrule)
        nffg_xml = nffg.xml()

    return nffg.xml()


def delete_VNF(nffg_xml, vnf_id):
    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    flows_in = getFlowRulesSendingTrafficToVNF(un, vnf_id)
    flows_out = getFlowRulesSendingTrafficFromVNF(un, vnf_id)

    flow_list = flows_in + flows_out

    flow_list_id = []
    for flow in flow_list:
        flow.set_operation(operation="delete", recursive=False)
        id = flow.id.get_value()
        flow_list_id.append(id)
    logging.info("flows to delete: {0}".format(flow_list_id))

    vnf = un.NF_instances.node.__getitem__(vnf_id)
    vnf.set_operation(operation="delete", recursive=False)

    return nffg.xml()


def delete_flows_by_priority(nffg_xml, priority):
    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    # get the flowrules to be replaced, with old_priority
    flowrules_to_be_deleted = [flowrule for flowrule in un.flowtable if
                                str(flowrule.priority.get_value()) == str(priority)]

    for flowrule in flowrules_to_be_deleted:
        flowrule.set_operation(operation="delete", recursive=False)
        logging.info("deleted flow id: {0} with priority {1} ".format(flowrule.id.get_value(), priority))

    return nffg.xml()


def delete_VNF_incoming_ext_flows(nffg_xml, vnf_id):
    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    flows_in = getFlowRulesSendingTrafficToVNF(un, vnf_id)
    for flowentry in flows_in:
        portPath = flowentry.out.get_target().get_path()
        tokens = portPath.split('/')
        if tokens[4] == 'ports':
            # This is a port of the universal node
            flowentry.set_operation(operation="delete", recursive=False)
            nffg_log.debug("deleted external flow with id: {0}".format(flowentry.id.get_value()))

    return nffg.xml()


def getFlowRulesSendingTrafficToVNF(un, vnfId):
    flow_rules = []

    for flowentry in un.flowtable:
        portPath = flowentry.out.get_target().get_path()
        port = flowentry.out.get_target()
        tokens = portPath.split('/')
        if tokens[4] == 'ports':
            # This is a port of the universal node, skip
            pass

        elif tokens[4] == 'NF_instances':
            # This is a port of the NF. I have to extract the port ID and the type of the NF.
            vnf = port.get_parent().get_parent()
            vnf_id = vnf.id.get_value()
            port_id = (port.id.get_value())
            if vnf_id == vnfId :
                flow_rules.append(flowentry)

        else:
            nffg_log.error("Invalid port {0} defined in a flowentry".format(port))

        continue

    return flow_rules

def getFlowRulesSendingTrafficFromVNF(un, vnfId):
    flow_rules = []

    for flowentry in un.flowtable:
        portPath = flowentry.port.get_target().get_path()
        port = flowentry.port.get_target()
        tokens = portPath.split('/')
        if tokens[4] == 'ports':
            # This is a port of the universal node, skip
            pass

        elif tokens[4] == 'NF_instances':
            # This is a port of the NF. I have to extract the port ID and the type of the NF.
            vnf = port.get_parent().get_parent()
            vnf_id = vnf.id.get_value()
            port_id = (port.id.get_value())
            if vnf_id == vnfId :
                flow_rules.append(flowentry)

        else:
            nffg_log.error("Invalid port {0} defined in a flowentry".format(port))

        continue

    return flow_rules


def getFlowRulesSendingTrafficFromPort(un, vnfId, portId):
    flow_rules = []

    for flowentry in un.flowtable:
        portPath = flowentry.port.get_target().get_path()
        port = flowentry.port.get_target()
        tokens = portPath.split('/')
        if tokens[4] == 'ports':
            # This is a port of the universal node, skip
            pass

        elif tokens[4] == 'NF_instances':
            # This is a port of the NF. I have to extract the port ID and the type of the NF.
            vnf = port.get_parent().get_parent()
            vnf_id = vnf.id.get_value()
            port_id = (port.id.get_value())
            if vnf_id == vnfId and port_id == portId:
                flow_rules.append(flowentry)

        else:
            nffg_log.error("Invalid port {0} defined in a flowentry".format(port))

        continue

    return flow_rules

# remove all measure strings of all ovs vnfs in the nffg
def remove_measure_to_ovs_vnfs(nffg_xml):

    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    nf_instances = un.NF_instances
    for nf in nf_instances:
        nf_name = nf.name.get_value()
        nf_type = nf.type.get_value()
        nf_id = nf.id.get_value()

        logging.debug("found NF: {0}".format(nf.name.get_value()))
        if 'ovs' in nf_type:
            measurestring = ""
            nf.metadata.add(MetadataMetadata(key='measure', value=measurestring))

    return nffg.xml()

# add the measure string to all ovs vnfs in the nffg
def add_measure_to_ovs_vnfs(nffg_xml, direction):

    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    nf_instances = un.NF_instances
    for nf in nf_instances:
        nf_name = nf.name.get_value()
        nf_type = nf.type.get_value()
        nf_id = nf.id.get_value()

        logging.debug("found NF: {0}".format(nf.name.get_value()))
        if 'ovs' in nf_type and 'in' in direction:
            #measurestring = MEASURE_STRING2.format(nf_id)
            measurestring = MEASURE_SCALEIN
            nf.metadata.add(MetadataMetadata(key='measure', value=measurestring))
            # only add to one of the VNFs
            break
        elif 'ovs' in nf_type and 'out' in direction:
            measurestring = MEASURE_SCALEOUT
            nf.metadata.add(MetadataMetadata(key='measure', value=measurestring))
             # only add to one of the VNFs
            break

    return nffg.xml()


def add_ovs_vnf(nffg_xml, nffg_id, ovs_id, name, numports, vnftype='ovs', add_measure=False):

    mac = str(hex(int(ovs_id))[2:]).zfill(2)
    ovs_mac = '00:00:00:00:00:{0}'.format(mac)
    ovs_ip = '10.0.10.{0}/24'.format(ovs_id)
    ovs_ip2 = '10.0.10.{0}'.format(ovs_id)

    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)
    vnf = Node(id=str(nffg_id), name=name, type=vnftype)

    # add external mgmt port
    port = Port(id=str(0), name='public-port', port_type='port-sap', sap='INTERNET',
                addresses=PortAddresses(l4="{'tcp/22'}"))
    vnf.ports.add(port)

    # add control port
    l3 = L3_address(id='addr-ctrl', configure='True', requested=ovs_ip.format(ovs_id))
    mac = str(hex(int(ovs_id))[2:]).zfill(2)
    l3_address = PortAddresses(l2=ovs_mac)
    l3_address.add(l3)
    control_port = Port(id=str(1), name='control-port', port_type='port-abstract',
                addresses=l3_address)
    vnf.ports.add(control_port)

    i = 2
    for x in range(0, numports):
        port = Port(id=str(i), name=name + '_eth' + str(i-2), port_type='port-abstract')
        vnf.ports.add(port)
        i = i+1

    vnf.metadata.add(MetadataMetadata(key='variable:VNF_NAME', value=name))
    vnf.metadata.add(MetadataMetadata(key='variable:OVS_DPID', value='99{0}'.format(str(ovs_id).zfill(14))))
    vnf.metadata.add(MetadataMetadata(key='variable:CONTROLLER', value='tcp:10.0.10.100:6633'))

    # create very long measure string without newlines included, without escaped chars, fit for printing into xml...
    #measurestring = MEASURE_STRING2.format(nffg_id)

    if add_measure:
        # create very long measure string without newlines included, without escaped chars, fit for printing into xml...
        measurestring = MEASURE_SCALEIN

        vnf.metadata.add(MetadataMetadata(key='measure', value=measurestring))

    vnf.set_operation(operation="create",  recursive=False)
    un.NF_instances.add(vnf)

    # add control link
    #ctrl_id = 'ctrl' #1
    ctrl_id = 1
    controller = un.NF_instances.node.__getitem__(str(ctrl_id)) # controller has always id=1 or ctrl
    controller_port = controller.ports.port.__getitem__(str(1)) # control port has always id=1
    ovs_port = vnf.ports.port.__getitem__(str(1))  # control port has always id=1

    # controller -> ovs (ip)
    flowentry_id = get_next_flowrule_id(nffg.xml())
    match='ether_type=0x800,dest_ip={0}'.format(ovs_ip2)
    new_flowentry = Flowentry(id=flowentry_id, priority=10, port=controller_port, out=ovs_port, match=match)
    new_flowentry.set_operation(operation="create", recursive=False)
    un.flowtable.add(new_flowentry)

    # controller -> ovs (arp)
    flowentry_id = get_next_flowrule_id(nffg.xml())
    match = 'ether_type=0x806,dest_mac={0}'.format(ovs_mac)
    new_flowentry = Flowentry(id=flowentry_id, priority=10, port=controller_port, out=ovs_port, match=match)
    new_flowentry.set_operation(operation="create", recursive=False)
    un.flowtable.add(new_flowentry)

    # ovs -> controller
    flowentry_id = get_next_flowrule_id(nffg.xml())
    new_flowentry = Flowentry(id=flowentry_id, priority=10, port=ovs_port, out=controller_port)
    new_flowentry.set_operation(operation="create", recursive=False)
    un.flowtable.add(new_flowentry)
    return nffg.xml()


def get_next_flowrule_id(nffg_xml, add=0):

    next_id = randint(10000, 99999)
    return next_id

    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    flow_id_list = []
    for flowrule in un.flowtable:
        id = int(flowrule.id.get_value())
        flow_id_list.append(id)

    # http://stackoverflow.com/questions/3149440/python-splitting-list-based-on-missing-numbers-in-a-sequence
    # group the sorted list unitl a value is missing (the deleted vnf id)
    sorted_vnf_id_list = sorted(flow_id_list)
    list = []
    for k, g in groupby(enumerate(sorted_vnf_id_list), lambda (i, x): i - x):
        list.append(map(itemgetter(1), g))
        break

    if min(list[0]) > 1:
        max_id = 0
    else:
        max_id = max(list[0])


    if max_id == 999999999:
        max_id = 0
    next_id_str = str(max_id+1+add)

    return next_id_str


def add_flowentry(nffg_xml, port_in, port_out, match=None, priority=10):
    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)

    # for SAP ports, need to look in nodes tree
    if port_in.port_type == DPPort.SAP:
        DP_in = un
    else:
        DP_in = un.NF_instances.node.__getitem__(str(port_in.DP.id))
        #DP_in = un.NF_instances.node.__getitem__(str(port_in.DP.name))
    DP_port_in = DP_in.ports.port.__getitem__(str(port_in.id))

    if port_out.port_type == DPPort.SAP:
        DP_out = un
    else:
        #DP_out = un.NF_instances.node.__getitem__(str(port_out.DP.name))
        DP_out = un.NF_instances.node.__getitem__(str(port_out.DP.id))
    DP_port_out = DP_out.ports.port.__getitem__(str(port_out.id))

    flowentry_id = get_next_flowrule_id(nffg_xml)
    new_flowentry = Flowentry(id=flowentry_id, priority=priority, port=DP_port_in, out=DP_port_out, match=match)
    new_flowentry.set_operation(operation="create", recursive=False)
    un.flowtable.add(new_flowentry)

    nffg_log.debug('add flow to nffg: id:{2}-priority:{3} - {0} -> {1}'.format(port_in.ifname, port_out.ifname, flowentry_id, priority))

    return nffg.xml()


# for compatibility with json nffg
add_flowentry_SAP = add_flowentry

def clean_nffg(nffg_xml):
    # clean all existing vnfs without operation attribute
    new_nffg_xml = copy.deepcopy(nffg_xml)
    new_nffg = get_virtualizer_nffg(new_nffg_xml)
    new_un = get_UN_node(new_nffg)
    for vnf in new_un.NF_instances:
        if not vnf.has_operation("create", recursive=False):
            new_un.NF_instances.node.remove(vnf.id.get_value())
    # clean all existing flowentries without operation attribute
    for flowrule in new_un.flowtable:
        if not flowrule.has_operation("create", recursive=False):
            new_un.flowtable.flowentry.remove(flowrule.id.get_value())

    return new_nffg.xml()

def check_vnf_in_nffg(nffg_xml, vnf_name):
    found = False
    nffg = get_virtualizer_nffg(nffg_xml)
    un = get_UN_node(nffg)
    nf_instances = un.NF_instances
    for nf in nf_instances:
        nf_name = nf.name.get_value()
        nf_type = nf.type.get_value()
        logging.debug("found NF: {0}".format(nf.name.get_value()))
        if vnf_name == nf_name:
            found = True
            break
    return found

if __name__ == "__main__":
    xml = open('test.xml').read()
    ovs_instances = process_nffg(xml)

    nffg = get_virtualizer_nffg(xml)
    un = get_UN_node(nffg)


    new_xml = nffg.xml()

    new_xml = add_ovs_vnf(new_xml, 3, 2, 'ovs2', 'ovs', 4)

    new_DP = DP('ovs2', 3)
    new_port1 = DPPort('eth_test10', 5, DP_parent=new_DP)
    new_DP.ports.append(new_port1)
    new_port2 = DPPort('eth_test11', 5, DP_parent=new_DP)
    new_DP.ports.append(new_port2)

    new_xml = add_flowentry(new_xml, new_port1, new_port2)

    new_xml = clean_nffg(new_xml)
    nffg2 = get_virtualizer_nffg(new_xml)


    print nffg2.xml()
    pass