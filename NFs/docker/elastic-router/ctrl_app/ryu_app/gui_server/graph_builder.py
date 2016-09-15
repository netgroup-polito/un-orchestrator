import json
from math import cos, sin, pi
from er_utils import DPPort
import logging

defaultNodeColor = '#ec5148'


def build_graph(DP_dict, base_file='base_ER.json', y_offset=0, output_file=None):
    json_file = open(base_file).read()
    graph_dict = json.loads(json_file)

    sap_list = []
    x = 0
    y = 0
    R = 1
    num_DP = len(DP_dict)
    if num_DP == 0:
        return graph_dict

    theta = (2 * pi) / num_DP
    i = 1
    j = 0
    for ovs_name in DP_dict:
        logging.info('graph ovs:{0}'.format(ovs_name))
        # extra rotation to avoid ovs node is just below ctrl node
        add = 0
        if num_DP % 4 == 0:
            add = theta / 2

        if num_DP > 1:
            x = R * cos((theta * i) + add)
            y = R * sin((theta * i) + add) + y_offset
            i += 1

        ovs_dict={'id':ovs_name, 'label': ovs_name, 'x': x, 'y': y, 'size': 4}
        graph_dict['nodes'].append(ovs_dict)


        for port in DP_dict[ovs_name].ports:
            id = 'e{0}'.format(j)

            source = port.DP.name
            if port.port_type == DPPort.Internal:
                target = port.linked_port.DP.name
            elif port.port_type == DPPort.External:
                sap_name = port.linked_port.ifname
                if sap_name not in sap_list:
                    sap_list.append(sap_name)
                target = sap_name

            edge_dict = {'id': id, 'source': source, 'target': target}
            graph_dict['edges'].append(edge_dict)
            j += 1

    x = 0
    y = 0
    R = 3
    num_saps = len(sap_list)
    theta = (2 * pi) / num_saps
    i = 1
    for sap_name in sap_list:

        # extra rotation to avoid ovs node is just below ctrl node
        add = 0
        if num_saps % 4 == 0:
            add = theta / 2

        if num_saps > 1:
            x = R * cos((theta * i) + add)
            y = R * sin((theta * i) + add)
            i += 1

        sap_dict = {'id': sap_name, 'label': sap_name, 'x': x, 'y': y, 'size': 2, 'color': 'green'}
        graph_dict['nodes'].append(sap_dict)

    #logging.info('graph send:{0}'.format(json.dumps(graph_dict, indent=4)))

    if output_file is None:
        output_file = base_file
    with open(output_file, 'w') as outfile:
        json.dump(graph_dict, outfile)

    return graph_dict


def build_graph_list(DP_list, base_file='base_ER.json', y_offset=0, output_file=None):
    json_file = open(base_file).read()
    graph_dict = json.loads(json_file)

    sap_list = []
    x = 0
    y = 0 + y_offset
    R = 1
    num_DP = len(DP_list)
    if num_DP == 0:
        return graph_dict
    theta = (2 * pi) / num_DP
    i = 1

    #edge id must be unique
    edge_names = [edge['id'] for edge in graph_dict['edges']]
    j = len(edge_names)

    for DP in DP_list:
        ovs_name = DP.name
        logging.info('graph ovs:{0}'.format(ovs_name))
        # extra rotation to avoid ovs node is just below ctrl node
        add = 0
        if num_DP % 4 == 0:
            add = theta / 2

        if num_DP > 1:
            x = R * cos((theta * i) + add)
            y = R * sin((theta * i) + add) + y_offset
            i += 1

        ovs_dict={'id':ovs_name, 'label': ovs_name, 'x': x, 'y': y, 'size': 4}
        graph_dict['nodes'].append(ovs_dict)


        for port in DP.ports:
            id = 'e{0}'.format(j)
            color = defaultNodeColor
            source = port.DP.name
            if port.port_type == DPPort.Internal:
                target = port.linked_port.DP.name
            elif port.port_type == DPPort.External:
                sap_name = port.linked_port.ifname
                if sap_name not in sap_list:
                    sap_list.append(sap_name)
                target = sap_name
                color = 'grey'

            edge_dict = {'id': id, 'source': source, 'target': target, 'color': color}
            graph_dict['edges'].append(edge_dict)
            j += 1

    x = 0
    y = 0
    R = 3
    num_saps = len(sap_list)
    theta = (2 * pi) / num_saps
    i = 1
    for sap_name in sap_list:

        node_names = [node['id'] for node in graph_dict['nodes']]
        if sap_name in node_names:
            #sap name allready exists, cannot have 2 nodes with the same id
            continue

        # extra rotation to avoid ovs node is just below ctrl node
        add = 0
        if num_saps % 4 == 0:
            add = theta / 2

        if num_saps > 1:
            x = R * cos((theta * i) + add)
            y = R * sin((theta * i) + add)
            i += 1

        sap_dict = {'id': sap_name, 'label': sap_name, 'x': x, 'y': y, 'size': 2, 'color': 'green'}
        graph_dict['nodes'].append(sap_dict)

    #logging.info('graph send:{0}'.format(json.dumps(graph_dict, indent=4)))

    if output_file is None:
        output_file = base_file
    with open(output_file, 'w') as outfile:
        json.dump(graph_dict, outfile)

    return graph_dict


def change_graph(base_file='parsed_nffg.json', nodes=[], edges=[], color='grey', output_file=None):
    json_file = open(base_file).read()
    graph_dict = json.loads(json_file)

    for node_id in nodes:
        node_dict = find_node(graph_dict, node_id)
        node_dict['color'] = color

    for edge_source in edges:
        edge_target = 'sap'
        edge_list = find_edges(graph_dict, edge_target, edge_source)
        for edge_dict in edge_list:
            edge_dict['color'] = color

    if output_file is None:
        output_file = base_file
    with open(output_file, 'w') as outfile:
        json.dump(graph_dict, outfile)

    return graph_dict


def find_node(graph_dict, node_id):
    for node in graph_dict['nodes']:
        if node_id == node['id']:
            return node
    return None


def find_edges(graph_dict, edge_target, edge_source):
    found_edges = []
    for edge in graph_dict['edges']:
        if edge_target in edge['target'] and edge_source in edge['source']:
            found_edges.append(edge)
    return found_edges
