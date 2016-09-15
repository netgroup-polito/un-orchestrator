__author__ = 'Steven Van Rossem'

#NF-FG library
#import sys
#sys.path.append('virtualizer4')
#import os, imp
#imp.load_source("virtualizer4", os.path.join(os.path.dirname(__file__), "virtualizer4/virtualizer4.py"))

from virtualizer4.virtualizer4 import *
from er_utils import *
#import virtualizer3

import logging
import xml.etree.ElementTree as ET
import threading

#Set the logger
logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

def process_nffg(nffg_xml):

    logging.debug("Reading tmp file '%s'...", nffg_xml)

    try:
        tree = ET.fromstring(nffg_xml)
    except ET.ParseError as e:
        print('ParseError: %s' % e.message)
        return 0
    logging.debug("File correctly read")

    nffg = Virtualizer.parse(root=tree)
    universal_nodes = nffg.nodes
    for un in universal_nodes:
        un_id = un.id.get_value()
        logging.debug("found UN: %s", un.id.get_value())
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
        logging.debug("found NF: %s", nf.name.get_value())
        if 'ovs' in nf_type:
            ovsName = nf.name.get_value()
            ovsId = nf.id.get_value()
            logging.debug("found ovs NF: %s", ovsName)
            ovs_instances.setdefault(ovsName, {'id': 0, 'ports': []})
            # get ports of ovs (eth0 is control port by convention, only need switch ports)
            # TODO meta data in nffg to define control port, switch port, port name in docker, switch name in docker
            for port in nf.ports:
                portName = port.name.get_value()
                portId = port.id.get_value()
                new_port = DPPort(portName, portId)
                ovs_instances[ovsName]['ports'].append(new_port)
                ovs_instances[ovsName]['id'] = ovsId
                logging.debug("found ovs port: %s with ovs id: %s", portName, ovsId)

    # check if the ports are internal nffg links or external to SAPs
    #ovs_instances_with_link_types = {}
    port_types, port_linked_ports = extract_links(un)
    #port_linked_ports = extract_links(un)[1]
    for ovs in ovs_instances:
        for port in ovs_instances[ovs]['ports']:
            portName = port.ifname
            try:
                port.port_type = port_types[ovs][portName]
                linked_port = port_linked_ports[ovs][portName]
                port.linked_port = linked_port
                port.linked_port.linked_port = port
            except:
                logging.info("{0} port: {1} not linked to SAP or ovs".format(ovs, portName))

    return ovs_instances

def extract_links(un):
    """
    Find the links of the ER topology in the NFFG, to know which ports are connected to SAPs and which are not.
    This information is needed for the routing in the ER ctrl_app.
    code from virtualizer.py in un-orchestrator
    :param un: Universal Node object from the NFFG
    :return:
    """
    port_types = {}
    port_linked_ports = {}
    flowtable_ = un.flowtable
    for flowentry_ in flowtable_:
        portPath_ = flowentry_.port.get_target().get_path()
        port = flowentry_.port.get_target()
        tokens = portPath_.split('/')

        if len(tokens) is not 6 and len(tokens) is not 8:
            logging.error("Invalid port '%s' defined in a flowentry (len(tokens) returned %d)",portPath_,len(tokens))
            error = True
            return

        if tokens[4] == 'ports':
            #This is a port of the universal node. We have to extract the virtualized port name
            src_node = ('SAP', port)
        elif tokens[4] == 'NF_instances':
            #This is a port of the NF. I have to extract the port ID and the type of the NF.
            #XXX I'm using the port ID as name of the port
            src_node= ('VNF', port)
        else:
            logging.error("Invalid port '%s' defined in a flowentry",port)
            error = True
            return

        portPath_ = flowentry_.out.get_target().get_path()
        port = flowentry_.out.get_target()
        tokens = portPath_.split('/')
        if len(tokens) is not 6 and len(tokens) is not 8:
            logging.error("Invalid port '%s' defined in a flowentry",portPath_)
            error = True
            return

        if tokens[4] == 'ports':
            #This is a port of the universal node. We have to extract the ID
            #Then, I have to retrieve the virtualized port name, and from there
            #the real name of the port on the universal node
            dst_node= ('SAP', port)
        elif tokens[4] == 'NF_instances':
            #This is a port of the NF. I have to extract the port ID and the type of the NF.
            #XXX I'm using the port ID as name of the port
            dst_node = ('VNF', port)
        else:
            logging.error("Invalid port '%s' defined in a flowentry",port)
            error = True
            return

        if src_node[0] == 'VNF' and dst_node[0] == 'SAP' and 'ovs' in src_node[1].name.get_value():
            port = src_node[1]
            vnf = port.get_parent().get_parent()
            vnfName = vnf.name.get_value()
            portName = port.name.get_value()
            port_types.setdefault(vnfName, {})
            port_types[vnfName][portName] = DPPort.External
            linked_port = dst_node[1]
            port_linked_ports.setdefault(vnfName, {})
            linked_port2 = DPPort(linked_port.name.get_value(), linked_port.id.get_value(), \
                                  port_type=DPPort.SAP, DP_parent=DP(un.name.get_value(),un.id.get_value(),[]))
            port_linked_ports[vnfName][portName] = linked_port2

            # port = dst_node[1]
            # vnf = port.get_parent().get_parent()
            # vnfName = vnf.name.get_value()
            # portName = port.name.get_value()
            # port_types.setdefault(vnfName, {})
            # port_types[vnfName][portName] = DPPort.Internal
            # linked_port = src_node[1]
            # port_linked_ports.setdefault(vnfName, {})
            # port_linked_ports[vnfName][portName] = linked_port
        elif src_node[0] == 'SAP' and dst_node[0] == 'VNF' and 'ovs' in dst_node[1].name.get_value():
            port = src_node[1]
            vnf = port.get_parent().get_parent()
            vnfName = vnf.name.get_value()
            portName = port.name.get_value()
            port_types.setdefault(vnfName, {})
            port_types[vnfName][portName] = DPPort.SAP
            linked_port = dst_node[1]
            port_linked_ports.setdefault(vnfName, {})
            linked_port2 = DPPort(linked_port.name.get_value(), linked_port.id.get_value(), \
                                  port_type=DPPort.External)
            port_linked_ports[vnfName][portName] = linked_port2

            # port = src_node[1]
            # vnf = port.get_parent().get_parent()
            # vnfName = vnf.name.get_value()
            # portName = port.name.get_value()
            # port_types.setdefault(vnfName, {})
            # port_types[vnfName][portName] = DPPort.Internal
            # linked_port = dst_node[1]
            # port_linked_ports.setdefault(vnfName, {})
            # port_linked_ports[vnfName][portName] = linked_port
        elif src_node[0] == 'VNF' and dst_node[0] == 'VNF' \
            and 'ovs' in src_node[1].name.get_value() and 'ovs' in dst_node[1].name.get_value():
            port = src_node[1]
            vnf = port.get_parent().get_parent()
            vnfName = vnf.name.get_value()
            portName = port.name.get_value()
            port_types.setdefault(vnfName, {})
            port_types[vnfName][portName] = DPPort.Internal
            linked_port = dst_node[1]
            linked_port2 = DPPort(linked_port.name.get_value(), linked_port.id.get_value(), \
                                  port_type=DPPort.Internal)
            port_linked_ports.setdefault(vnfName, {})
            port_linked_ports[vnfName][portName] = linked_port2

            # port = src_node[1]
            # vnfName = vnf.name.get_value()
            # portName = port.name.get_value()
            # port_types.setdefault(vnfName, {})
            # port_types[vnfName][portName] = DPPort.Internal
            # linked_port = dst_node[1]
            # port_linked_ports.setdefault(vnfName, {})
            # port_linked_ports[vnfName][portName] = linked_port

    return (port_types, port_linked_ports)

def convert_to_virtualizer(nffg_xml):
    try:
        tree = ET.fromstring(nffg_xml)
    except ET.ParseError as e:
        print('ParseError: %s' % e.message)
        return 0
    logging.debug("File correctly read")

    nffg = Virtualizer.parse(root=tree)
    return nffg

def add_vnf(nffg_xml, id, name, vnftype, numports):
    nffg = convert_to_virtualizer(nffg_xml)
    # take first universal node by default
    un = nffg.nodes.__iter__().next()
    nf_instances = un.NF_instances

    # get number of ovs VNFs in nffg
    #ovs_instances = [vnf.name.get_value() for vnf in nf_instances if 'ovs' in vnf.type.get_value()]
    #print ovs_instances

    vnf = Node(id=str(id), name=name, type=vnftype)
    i = 1
    for x in range(0, numports):
        port = Port(id=str(i), name=name + '_eth' + str(i-1), port_type='port-abstract')
        vnf.ports.add(port)
        i = i+1

    un.NF_instances.add(vnf)

    return nffg.xml()

def remove_vnf(nffg_xml, vnf_id):
    nffg = convert_to_virtualizer(nffg_xml)
    # take first universal node by default
    un = nffg.nodes.__iter__().next()
    un.NF_instances.remove(str(vnf_id))
    return nffg.xml()

def delete_vnf(nffg_xml, vnf_id):
    nffg = convert_to_virtualizer(nffg_xml)
    # take first universal node by default
    un = nffg.nodes.__iter__().next()
    vnf = un.NF_instances.__getitem__(str(vnf_id))
    vnf.set_operation(operation="delete")

    # also delete all flowentries for this vnf
    nffg_xml = delete_flowentries(nffg.xml(), vnf_id)

    return nffg_xml

def delete_flowentries(nffg_xml, vnf_id):
    nffg = convert_to_virtualizer(nffg_xml)
    # take first universal node by default
    un = nffg.nodes.__iter__().next()
    flowtable = un.flowtable

    for flowentry in flowtable:

        portPath = flowentry.port.get_target().get_path()
        port = flowentry.port.get_target()
        tokens = portPath.split('/')

        if len(tokens) is not 6 and len(tokens) is not 8:
            logging.error("Invalid port '%s' defined in a flowentry (len(tokens) returned %d)",portPath,len(tokens))
            error = True
            return

        if tokens[4] == 'NF_instances':
            # this is a port of a vnf
            vnf = port.get_parent().get_parent()
            vnfId = vnf.id.get_value()
            if vnfId == vnf_id:
                # delete this flowentry
                #flowtable.remove(flowentry)
                flowentry.set_operation(operation="delete",recursive=False)
                continue

        # test for out path in flowentry
        outPath = flowentry.out.get_target().get_path()
        out = flowentry.out.get_target()
        tokens = outPath.split('/');

        if len(tokens) is not 6 and len(tokens) is not 8:
            logging.error("Invalid port '%s' defined in a flowentry (len(tokens) returned %d)",portPath,len(tokens))
            error = True
            return

        if tokens[4] == 'NF_instances':
            # this is a port of a vnf
            vnf = out.get_parent().get_parent()
            vnfId = vnf.id.get_value()
            if vnfId == vnf_id:
                # delete this flowentry
                #flowtable.remove(flowentry)
                flowentry.set_operation(operation="delete", recursive=False)

    return nffg.xml()

def add_flowentry(nffg_xml, port_in, port_out, match=None, priority='100'):
    nffg = convert_to_virtualizer(nffg_xml)
    # take first universal node by default
    un = nffg.nodes.__iter__().next()
    flowtable = un.flowtable
    # for SAP ports, need to look in nodes tree
    if port_in.port_type == DPPort.SAP:
        DP_in = nffg.nodes.__getitem__(str(port_in.DP.id))
    else:
        DP_in = un.NF_instances.__getitem__(str(port_in.DP.id))
    DP_port_in = DP_in.ports.__getitem__(str(port_in.id))

    if port_out.port_type == DPPort.SAP:
        DP_out = nffg.nodes.__getitem__(str(port_out.DP.id))
    else:
        DP_out = un.NF_instances.__getitem__(str(port_out.DP.id))
    DP_port_out = DP_out.ports.__getitem__(str(port_out.id))

    flowentry_id = str(len(flowtable.flowentry.items())+1)
    new_flowentry = Flowentry(id=flowentry_id, priority=priority, port=DP_port_in, out=DP_port_out, match=match)
    flowtable.add(new_flowentry)
    return nffg.xml()


if __name__ == "__main__":
    xml = open('er_nffg.xml').read()
    DP_switches = process_nffg(xml)
    for DP_name in DP_switches:
        id = DP_switches[DP_name]['id']
        ports = DP_switches[DP_name]['ports']
        new_DP = DP(DP_name, id, ports)
        for port in new_DP.ports:
            print(port.ifname)
            print(port.port_type)


    new_nffg = add_vnf(xml, '2', 'ovs2', 'ovs2', 4)
    print new_nffg
    #new_nffg = remove_vnf(new_nffg, '1')
    #print new_nffg
    DP_instances = {}
    DP_switches = process_nffg(new_nffg)
    for DP_name in DP_switches:
        id = DP_switches[DP_name]['id']
        ports = DP_switches[DP_name]['ports']
        new_DP = DP(DP_name, id, ports)
        DP_instances[DP_name] = new_DP
        for port in new_DP.ports:
            print(port.ifname)
            print(port.port_type)

    test_lock = threading.Lock()
    test_lock.acquire()

    # add flowentry
    new_nffg = add_flowentry(new_nffg, DP_instances['ovs1'].ports[0], DP_instances['ovs2'].ports[0])
    new_nffg = add_flowentry(new_nffg, DP_instances['ovs2'].ports[0], DP_instances['ovs1'].ports[0], match = 'ipv4_dst=10.0.10.2')
    print new_nffg

    # remove vnf
    vnf_id = DP_instances['ovs1'].id
    new_nffg = delete_vnf(new_nffg, vnf_id)
    #new_nffg = delete_flowentries(new_nffg, vnf_id)
    print new_nffg



