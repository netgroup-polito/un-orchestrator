'''
Created on Oct 14, 2015

@author: fabiomignini
@author: stefanopetrangeli
'''
import sys, uuid, json, copy, collections
from .exception import InexistentLabelFound, WrongNumberOfPorts

class NF_FG(object):
    def __init__(self, _id = None, name = None,
                 vnfs = None, end_points = None,
                 flow_rules = None, description = None, 
                 domain = None, unify_monitoring = None):
        self.id = _id
        self.name = name
        self.description = description
        self.vnfs = vnfs or []
        self.end_points = end_points or []
        self.flow_rules = flow_rules or []
        self.domain = domain
        self.unify_monitoring = unify_monitoring
    
    def parseDict(self, nffg_dict):
        self.id = nffg_dict['forwarding-graph']['id']
        if 'domain' in nffg_dict['forwarding-graph']:
            self.domain = nffg_dict['forwarding-graph']['domain']
        if 'unify-monitoring' in nffg_dict['forwarding-graph']:
            self.unify_monitoring = nffg_dict['forwarding-graph']['unify-monitoring']            
        if 'name' in nffg_dict['forwarding-graph']:
            self.name = nffg_dict['forwarding-graph']['name']
        if 'description' in nffg_dict['forwarding-graph']:
            self.description = nffg_dict['forwarding-graph']['description']
        if 'VNFs' in nffg_dict['forwarding-graph']:
            for vnf_dict in nffg_dict['forwarding-graph']['VNFs']:
                vnf = VNF()
                vnf.parseDict(vnf_dict)
                self.vnfs.append(vnf)
        if 'end-points' in nffg_dict['forwarding-graph']:
            for end_point_dict in nffg_dict['forwarding-graph']['end-points']:
                endpoint = EndPoint()
                endpoint.parseDict(end_point_dict)
                self.end_points.append(endpoint)
        if 'big-switch' in nffg_dict['forwarding-graph']:
            if 'flow-rules' in nffg_dict['forwarding-graph']['big-switch']:
                for flow_rule_dict in nffg_dict['forwarding-graph']['big-switch']['flow-rules']:
                    flow_rule = FlowRule()
                    flow_rule.parseDict(flow_rule_dict)
                    self.flow_rules.append(flow_rule)
    
    def getDict(self, extended = False):
        nffg_dict = {}
        nffg_dict['forwarding-graph'] = {}
        if self.id is not None:
            nffg_dict['forwarding-graph']['id'] = self.id 
        if self.name is not None:
            nffg_dict['forwarding-graph']['name'] = self.name
        if self.description is not None:
            nffg_dict['forwarding-graph']['description'] = self.description
        if self.unify_monitoring is not None:
            nffg_dict['forwarding-graph']['unify-monitoring'] = self.unify_monitoring            
        vnfs_dict = []
        for vnf in self.vnfs:
            vnfs_dict.append(vnf.getDict(extended))
        if vnfs_dict:
            nffg_dict['forwarding-graph']['VNFs'] = vnfs_dict
        end_points_dict = []
        for end_point in self.end_points:
            end_points_dict.append(end_point.getDict(extended))
        if end_points_dict:
            nffg_dict['forwarding-graph']['end-points'] = end_points_dict
        flow_rules_dict = []
        for flow_rule in self.flow_rules:
            flow_rules_dict.append(flow_rule.getDict(extended))
        if flow_rules_dict:
            nffg_dict['forwarding-graph']['big-switch'] = {}
            nffg_dict['forwarding-graph']['big-switch']['flow-rules'] = flow_rules_dict
        if extended is True:
            if self.domain is not None:
                nffg_dict['forwarding-graph']['domain'] = self.domain
        return nffg_dict
    
    def getJSON(self, extended = False):
        return json.dumps(self.getDict(extended), indent=4)
    
    def getVNF(self, vnf_id):
        for vnf in self.vnfs:
            if vnf.id == vnf_id:
                return vnf
    
    def getVNFByDBID(self, vnf_db_id):
        for vnf in self.vnfs:
            if vnf.db_id == vnf_db_id:
                return vnf
    
    def getEndPoint(self, end_point_id):
        for end_point in self.end_points:
            if end_point.id == end_point_id:
                return end_point
            
    def getFlowRule(self, flow_rule_id):
        for flow_rule in self.flow_rules:
            if flow_rule.id == flow_rule_id:
                return flow_rule 
            
    def addVNF(self, vnf):
        if type(vnf) is VNF:
            self.vnfs.append(vnf)
        else:
            raise TypeError("Tried to add a vnf with a wrong type. Expected VNF, found "+type(vnf))
    
    def addEndPoint(self, end_point):
        if type(end_point) is EndPoint:
            self.end_points.append(end_point)
        else:
            raise TypeError("Tried to add a end-point with a wrong type. Expected EndPoint, found "+type(end_point))

    def addFlowRule(self, flow_rule):
        if type(flow_rule) is FlowRule:
            self.flow_rules.append(flow_rule)
        else:
            raise TypeError("Tried to add a flow-rules with a wrong type. Expected FlowRule, found "+type(flow_rule))
    
    def getEndPointsFromName(self, end_point_name):
        endpoints = []
        for end_point in self.end_points:
            if end_point.name == end_point_name:
                endpoints.append(end_point)
        return endpoints
    
    def getFlowRulesSendingTrafficToEndPoint(self, endpoint_id):
        return self.getFlowRuleSendingTrafficToNode("endpoint:"+endpoint_id)

    def getFlowRulesSendingTrafficFromEndPoint(self, endpoint_id):
        return self.getFlowRuleSendingTrafficFromNode("endpoint:"+endpoint_id)
    
    def getFlowRulesSendingTrafficFromPort(self, vnf_id, port_id):
        return self.getFlowRuleSendingTrafficFromNode("vnf:"+vnf_id+":"+port_id)
    
    def getFlowRulesSendingTrafficToPort(self, vnf_id, port_id):
        return self.getFlowRuleSendingTrafficToNode("vnf:"+vnf_id+":"+port_id)
    
    def getFlowRulesSendingTrafficToVNF(self, vnf):
        flow_rules = []
        for port in vnf.ports:
            flow_rules = flow_rules + self.getFlowRuleSendingTrafficToNode("vnf:"+vnf.id+":"+port.id)
        return flow_rules
    
    def getFlowRulesSendingTrafficFromVNF(self, vnf):
        flow_rules = []
        for port in vnf.ports:
            flow_rules = flow_rules + self.getFlowRuleSendingTrafficFromNode("vnf:"+vnf.id+":"+port.id)
        return flow_rules
    
    def getFlowRuleSendingTrafficToNode(self, node_id):
        flow_rules = []
        for flow_rule in self.flow_rules:
            for action in flow_rule.actions:
                if action.output == node_id:
                    flow_rules.append(flow_rule)
                    continue
        return flow_rules
    
    def getFlowRuleSendingTrafficFromNode(self, node_id):
        flow_rules = []
        for flow_rule in self.flow_rules:
            if flow_rule.match.port_in == node_id:
                flow_rules.append(flow_rule)
        return flow_rules
    
    def expandNode(self, old_vnf, internal_nffg):
        '''
        WARNING: Only 1to1 or 1toN connection among VNFs of internal graph and VNFs of external graph are supported.
        ''' 
        external_nffg = self
        
        # Add new VNFs in graph
        for internal_vnf in internal_nffg.vnfs:
            external_nffg.addVNF(internal_vnf)
        
        # List on ports of the old VNF. These ports represent the end-point of the internal graph (VNF expanded).
        for end_point_port in old_vnf.ports:
            # Add connections from internal graph (VNF expanded) to external graph
            internal_outgoing_flowrules = internal_nffg.getFlowRulesSendingTrafficToEndPoint(end_point_port.id.split(':')[0])
            external_outgoing_flowrules = external_nffg.getFlowRulesSendingTrafficFromPort(old_vnf.id, end_point_port.id)
            external_nffg.flow_rules = external_nffg.flow_rules + self.mergeFlowrules(internal_outgoing_flowrules, external_outgoing_flowrules)
            # Delete external_outgoing_flowrules from external_nffg.flow_rules
            for external_outgoing_flowrule in external_outgoing_flowrules:
                external_nffg.flow_rules.remove(external_outgoing_flowrule)
            for internal_outgoing_flowrule in internal_outgoing_flowrules:
                internal_nffg.flow_rules.remove(internal_outgoing_flowrule)
            
            # Add connections from external graph to internal graph 
            internal_ingoing_flowrules = internal_nffg.getFlowRulesSendingTrafficFromEndPoint(end_point_port.id.split(':')[0])
            external_ingoing_flowrules = external_nffg.getFlowRulesSendingTrafficToPort(old_vnf.id, end_point_port.id)
            external_nffg.flow_rules = external_nffg.flow_rules + self.mergeFlowrules(external_ingoing_flowrules, internal_ingoing_flowrules)
            # Delete external_ingoing_flowrules from external_nffg.flow_rules?
            for external_ingoing_flowrule in external_ingoing_flowrules:
                external_nffg.flow_rules.remove(external_ingoing_flowrule)
            for internal_ingoing_flowrule in internal_ingoing_flowrules:
                internal_nffg.flow_rules.remove(internal_ingoing_flowrule)     
        
        # Add Flow-rules
        for internal_flow_rule in internal_nffg.flow_rules:
            internal_flow_rule.id = uuid.uuid4().hex
            external_nffg.flow_rules.append(internal_flow_rule)
           
        # Delete old VNF
        self.vnfs.remove(old_vnf)
    
    def attachNF_FG(self, attaching_nffg, end_point_name):
        try:
            attaching_end_point = attaching_nffg.getEndPointsFromName(end_point_name)[0]
            end_point = self.getEndPointsFromName(end_point_name)[0]
        except IndexError:
            return
            
        
        if attaching_end_point is None or end_point is None:
            raise Exception("WARNING: Impossible to attach the graph: "+self.id+" with graph: "+attaching_nffg.id+" on an end-point called: "+end_point_name+", doesn't exit the common end_point.")
        
        # Add connections from internal graph (VNF expanded) to external graph
        attaching_outgoing_flowrules = attaching_nffg.getFlowRulesSendingTrafficToEndPoint(attaching_end_point.id)
        ingoing_flowrules = self.getFlowRulesSendingTrafficFromEndPoint(end_point.id)
        self.flow_rules = self.flow_rules + self.mergeFlowrules(attaching_outgoing_flowrules, ingoing_flowrules)
        # Delete external_outgoing_flowrules from external_nffg.flow_rules
        for ingoing_flowrule in ingoing_flowrules:
            self.flow_rules.remove(ingoing_flowrule)
        for attaching_outgoing_flowrule in attaching_outgoing_flowrules:
            attaching_nffg.flow_rules.remove(attaching_outgoing_flowrule)
        
        # Add connections from external graph to internal graph 
        attaching_ingoing_flowrules = attaching_nffg.getFlowRulesSendingTrafficFromEndPoint(attaching_end_point.id)
        outgoing_flowrules = self.getFlowRulesSendingTrafficToEndPoint(end_point.id)
        self.flow_rules = self.flow_rules + self.mergeFlowrules(outgoing_flowrules, attaching_ingoing_flowrules)
        # Delete external_ingoing_flowrules from external_nffg.flow_rules?
        for outgoing_flowrule in outgoing_flowrules:
            self.flow_rules.remove(outgoing_flowrule) 
        for attaching_ingoing_flowrule in attaching_ingoing_flowrules:
            attaching_nffg.flow_rules.remove(attaching_ingoing_flowrule)
            
        # Add the VNFs of the attaching graph
        for new_vnf in attaching_nffg.vnfs:
            self.vnfs.append(new_vnf)
        
        # Add the end-points of the attaching graph
        for new_end_point in attaching_nffg.end_points:
            self.end_points.append(new_end_point)
        
        # Add the end-points of the attaching graph
        for new_flow_rule in attaching_nffg.flow_rules:
            new_flow_rule.id = uuid.uuid4().hex
            self.flow_rules.append(new_flow_rule)
        
        # Delete the end-point of the attachment in the graph
        self.end_points.remove(end_point)
        self.end_points.remove(attaching_end_point)
        
    def mergeFlowrules(self, outgoing_flow_rules, ingoing_flow_rules, maintain_id=False):
        '''
        if maintaind_id is True the flowrule_id is preserved (except last two chars). This parameter is used when graphs are joined
        '''
        flowrules = []
        for outgoing_flow_rule in outgoing_flow_rules:
            for ingoing_flow_rule in ingoing_flow_rules:
                final_match = self.mergeMatches(outgoing_flow_rule.match, ingoing_flow_rule.match)
                if outgoing_flow_rule.match.port_in is not None:
                    final_match.port_in = outgoing_flow_rule.match.port_in
                if maintain_id:
                    flowrules.append(FlowRule(_id=ingoing_flow_rule.id[:-2], priority=ingoing_flow_rule.priority,
                                           actions=ingoing_flow_rule.actions, match = final_match))
                else:
                    flowrules.append(FlowRule(_id=uuid.uuid4().hex, priority=ingoing_flow_rule.priority,
                                           actions=ingoing_flow_rule.actions, match = final_match))
        return flowrules    
    
    def mergeMatches(self, first_match, second_match):
        match = Match()
        if first_match.ether_type is not None and second_match.ether_type is not None:
            if first_match.ether_type == second_match.ether_type:
                match.ether_type = second_match.ether_type
        elif first_match.ether_type is not None:
            match.ether_type = first_match.ether_type
        elif second_match.ether_type is not None:
            match.ether_type = second_match.ether_type
        if first_match.vlan_id is not None and second_match.vlan_id is not None:
            if first_match.vlan_id == second_match.vlan_id:
                match.vlan_id = second_match.vlan_id
        elif first_match.vlan_id is not None:
            match.vlan_id = first_match.vlan_id
        elif second_match.vlan_id is not None:
            match.vlan_id = second_match.vlan_id
        if first_match.vlan_priority is not None and second_match.vlan_priority is not None:
            if first_match.vlan_priority == second_match.vlan_priority:
                match.vlan_priority = second_match.vlan_priority
        elif first_match.vlan_priority is not None:
            match.vlan_priority = first_match.vlan_priority
        elif second_match.vlan_priority is not None:
            match.vlan_priority = second_match.vlan_priority
        if first_match.source_mac is not None and second_match.source_mac is not None:
            if first_match.source_mac == second_match.source_mac:
                match.source_mac = second_match.source_mac
        elif first_match.source_mac is not None:
            match.source_mac = first_match.source_mac
        elif second_match.source_mac is not None:
            match.source_mac = second_match.source_mac
        if first_match.dest_mac is not None and second_match.dest_mac is not None:
            if first_match.dest_mac == second_match.dest_mac:
                match.dest_mac = second_match.dest_mac
        elif first_match.dest_mac is not None:
            match.dest_mac = first_match.dest_mac
        elif second_match.dest_mac:
            match.dest_mac = second_match.dest_mac
        if first_match.source_ip is not None and second_match.source_ip is not None:
            if first_match.source_ip == second_match.source_ip:
                match.source_ip = second_match.source_ip
        elif first_match.source_ip is not None:
            match.source_ip = first_match.source_ip
        elif second_match.source_ip is not None:
            match.source_ip = second_match.source_ip
        if first_match.dest_ip is not None and second_match.dest_ip is not None:
            if first_match.dest_ip == second_match.dest_ip:
                match.dest_ip = second_match.dest_ip
        elif first_match.dest_ip is not None:
            match.dest_ip = first_match.dest_ip
        elif second_match.dest_ip is not None:
            match.dest_ip = second_match.dest_ip
        if first_match.tos_bits is not None and second_match.tos_bits is not None:
            if first_match.tos_bits == second_match.tos_bits:
                match.tos_bits = second_match.tos_bits
        elif first_match.tos_bits is not None:
            match.tos_bits = first_match.tos_bits
        elif second_match.tos_bits is not None:
            match.tos_bits = second_match.tos_bits
        if first_match.source_port is not None and second_match.source_port is not None:
            if first_match.source_port == second_match.source_port:
                match.source_port = second_match.source_port
        elif first_match.source_port is not None:
            match.source_port = first_match.source_port
        elif second_match.source_port is not None:
            match.source_port = second_match.source_port
        if first_match.dest_port is not None and second_match.dest_port is not None:
            if first_match.dest_port == second_match.dest_port:
                match.dest_port = second_match.dest_port
        elif first_match.dest_port is not None:
            match.dest_port = first_match.dest_port
        elif second_match.dest_port is not None:
            match.dest_port = second_match.dest_port
        if first_match.protocol is not None and second_match.protocol is not None:
            if first_match.protocol == second_match.protocol:
                match.protocol = second_match.protocol
        elif first_match.protocol is not None:
            match.protocol = first_match.protocol
        elif second_match.protocol is not None:
            match.protocol = second_match.protocol
        return match
    
    def diff (self, nffg_new):
        nffg = NF_FG()
        nffg.id = nffg_new.id
        nffg.name = nffg_new.name
        
        nffg.vnfs = nffg_new.vnfs
        nffg.end_points = nffg_new.end_points
        nffg.flow_rules = nffg_new.flow_rules 
        
        #VNFs
        for new_vnf in nffg_new.vnfs:
            new_vnf.status = 'new'   
        for old_vnf in self.vnfs:
            vnf_found = False
            for new_vnf in nffg_new.vnfs:
                if old_vnf.id == new_vnf.id and old_vnf.name == new_vnf.name and old_vnf.vnf_template_location == new_vnf.vnf_template_location:
                    new_vnf.status = 'already_deployed'
                    new_vnf.db_id = old_vnf.db_id
                    new_vnf.internal_id = old_vnf.internal_id
                    # check ports
                    
                    for new_port in new_vnf.ports:
                        new_port.status = 'new'
                    for old_port in old_vnf.ports:
                        port_found = False
                        for new_port in new_vnf.ports:
                            if old_port.id == new_port.id:
                                new_port.status = 'already_deployed'
                                new_port.db_id = old_port.db_id
                                new_port.internal_id = old_port.internal_id
                                
                                port_found = True
                                break
                        if port_found is False:
                            old_port.status = 'to_be_deleted'
                            new_vnf.ports.append(old_port)
                    vnf_found = True
                    break
            if vnf_found is False:
                old_vnf.status = 'to_be_deleted'
                nffg.vnfs.append(old_vnf)
        #Endpoints
        for new_endpoint in nffg_new.end_points:
            new_endpoint.status = 'new'
        for old_endpoint in self.end_points:
            endpoint_found = False
            for new_endpoint in nffg_new.end_points:
                if old_endpoint.id == new_endpoint.id and old_endpoint.type == new_endpoint.type\
                 and old_endpoint.vlan_id == new_endpoint.vlan_id and old_endpoint.remote_endpoint_id == new_endpoint.remote_endpoint_id\
                 and old_endpoint.node_id == new_endpoint.node_id and old_endpoint.switch_id == new_endpoint.switch_id\
                 and old_endpoint.interface == new_endpoint.interface and old_endpoint.remote_ip == new_endpoint.remote_ip\
                 and old_endpoint.local_ip == new_endpoint.local_ip and old_endpoint.ttl == new_endpoint.ttl\
                 and old_endpoint.local_ip == new_endpoint.local_ip and old_endpoint.ttl == new_endpoint.ttl:
                    if collections.Counter(old_endpoint.prepare_connection_to_remote_endpoint_ids) != collections.Counter(new_endpoint.prepare_connection_to_remote_endpoint_ids):
                        new_endpoint.status = 'to_be_updated'
                        new_endpoint.db_id = old_endpoint.db_id
                        endpoint_found = True
                        break
                    new_endpoint.status = 'already_deployed'
                    new_endpoint.db_id = old_endpoint.db_id
                    endpoint_found = True
                    break
            if endpoint_found is False:
                old_endpoint.status = 'to_be_deleted'
                nffg.end_points.append(old_endpoint)
        
        #Flowrules
        for new_flowrule in nffg.flow_rules:
            new_flowrule.status = 'new'
        for old_flowrule in self.flow_rules:
            flowrule_found = False
            for new_flowrule in nffg.flow_rules:
                if old_flowrule.getDict() == new_flowrule.getDict():
                    new_flowrule.status = 'already_deployed'
                    new_flowrule.db_id = old_flowrule.db_id
                    new_flowrule.internal_id = old_flowrule.internal_id
                    flowrule_found = True
                    break
            if flowrule_found is False:
                old_flowrule.status = 'to_be_deleted'
                nffg.flow_rules.append(old_flowrule)
        
        return nffg
    
    def split(self, left_list, right_list):
        '''
        Splits a nffg adding endpoints and returns both subgraphs (left and right)
        '''
        nffg_left = copy.deepcopy(self)
        left_list_original = copy.deepcopy(left_list)
        right_list_original = copy.deepcopy(right_list)
        
        #debug purpose only
        left_list_dict = []
        right_list_dict = []

        #Check if there are admissible elements in the lists
        total_list = left_list + right_list
        for element in total_list:
            if type(element) is VNF:
                if element not in self.vnfs:
                    raise Exception("VNF not present in nffg")
                if element in left_list:
                    left_list_dict.append("vnf:"+element.id)
                else:
                    right_list_dict.append("vnf:"+element.id)              
            elif type(element) is EndPoint:
                if element not in self.end_points:
                    raise Exception("EndPoint not present in nffg")
                if element in left_list:
                    left_list_dict.append("endpoint:"+element.id)
                else:
                    right_list_dict.append("endpoint:"+element.id)     
            else:
                raise TypeError()
            
        #print left_list_dict
        #print right_list_dict
        
        #Check links to break
        links = {}
        left_flows = []
        right_flows = []
        for l_element in left_list:
            #Endpoint
            if type(l_element) is EndPoint:
                flowrules = nffg_left.getFlowRulesSendingTrafficFromEndPoint(l_element.id)
                for flowrule in flowrules:
                    for action in flowrule.actions:
                        if action.output is not None:
                            if action.output.split(":")[0] == 'endpoint':
                                for r_element in right_list:
                                    if type(r_element) is EndPoint and r_element.id == action.output.split(":")[1]:
                                        if flowrule.match.port_in not in links:
                                            links[flowrule.match.port_in] = []
                                        links[flowrule.match.port_in].append(action.output)
                                        if flowrule.id not in left_flows:
                                            left_flows.append(flowrule.id)
                                        break
                            elif action.output.split(":")[0] == 'vnf':
                                for r_element in right_list:
                                    #action.output is vnf:vnf_id:port_id
                                    if type(r_element) is VNF and r_element.id == action.output.split(":")[1]:
                                        if flowrule.match.port_in not in links:
                                            links[flowrule.match.port_in] = []
                                        links[flowrule.match.port_in].append(action.output)
                                        if flowrule.id not in left_flows:
                                            left_flows.append(flowrule.id)
                                        break
                # Check flowrules sending traffic to this endpoint
                flowrules = nffg_left.getFlowRulesSendingTrafficToEndPoint(l_element.id)
                for flowrule in flowrules:
                    if flowrule.match is not None and flowrule.match.port_in is not None:
                        if flowrule.match.port_in.split(":")[0] == 'endpoint':
                            for r_element in right_list:
                                if type(r_element) is EndPoint and flowrule.match.port_in.split(":")[1] == r_element.id:
                                    if flowrule.match.port_in not in links:
                                        links[flowrule.match.port_in] = []
                                    links[flowrule.match.port_in].append("endpoint:"+l_element.id)
                                    if flowrule.id not in right_flows:
                                        right_flows.append(flowrule.id)
                                    break  
                        elif flowrule.match.port_in.split(":")[0] == 'vnf':
                            for r_element in right_list:
                                if type(r_element) is VNF and flowrule.match.port_in.split(":")[1] == r_element.id:
                                    if flowrule.match.port_in not in links:
                                        links[flowrule.match.port_in] = []
                                    links[flowrule.match.port_in].append("endpoint:"+l_element.id)
                                    if flowrule.id not in right_flows:
                                        right_flows.append(flowrule.id)
                                    break
            #VNF                        
            if type(l_element) is VNF:
                flowrules = nffg_left.getFlowRulesSendingTrafficFromVNF(l_element)
                for flowrule in flowrules:
                    for action in flowrule.actions:
                        if action.output is not None:
                            if action.output.split(":")[0] == 'endpoint':
                                for r_element in right_list:
                                    if type(r_element) is EndPoint and r_element.id == action.output.split(":")[1]:
                                        if flowrule.match.port_in not in links:
                                            links[flowrule.match.port_in] = []
                                        links[flowrule.match.port_in].append(action.output)
                                        if flowrule.id not in left_flows:
                                            left_flows.append(flowrule.id)
                                        break
                            elif action.output.split(":")[0] == 'vnf':
                                for r_element in right_list:
                                    #action.output is vnf:vnf_id:port_id
                                    if type(r_element) is VNF and r_element.id == action.output.split(":")[1]:
                                        if flowrule.match.port_in not in links:
                                            links[flowrule.match.port_in] = []
                                        links[flowrule.match.port_in].append(action.output)
                                        if flowrule.id not in left_flows:
                                            left_flows.append(flowrule.id)
                                        break
                # Check flowrules sending traffic to this vnf                       
                flowrules = nffg_left.getFlowRulesSendingTrafficToVNF(l_element)
                for flowrule in flowrules:
                    if flowrule.match is not None and flowrule.match.port_in is not None:
                        if flowrule.match.port_in.split(":")[0] == 'endpoint':
                            for r_element in right_list:
                                if type(r_element) is EndPoint and flowrule.match.port_in.split(":")[1] == r_element.id:
                                    if flowrule.match.port_in not in links:
                                        links[flowrule.match.port_in] = []
                                    for action in flowrule.actions:
                                        if action.output is not None and action.output.split(":")[1] == l_element.id:
                                            links[flowrule.match.port_in].append(action.output)
                                    if flowrule.id not in right_flows:
                                        right_flows.append(flowrule.id)
                                    break  
                        elif flowrule.match.port_in.split(":")[0] == 'vnf':
                            for r_element in right_list:
                                if type(r_element) is VNF and flowrule.match.port_in.split(":")[1] == r_element.id:
                                    if flowrule.match.port_in not in links:
                                        links[flowrule.match.port_in] = []
                                    for action in flowrule.actions:
                                        if action.output is not None and action.output.split(":")[1] == l_element.id:
                                            links[flowrule.match.port_in].append(action.output)
                                    if flowrule.id not in right_flows:
                                        right_flows.append(flowrule.id)
                                    break  
            
        #print links
        #print left_flows
        #print right_flows
        
        #Break links and create endpoints
        nffg_right = copy.deepcopy(nffg_left)
        #Link already split pointing to endpoint_id of generated endpoint
        link_split = {}
        
        for flow_id in left_flows:
            flowrule = nffg_left.getFlowRule(flow_id)
            for action in flowrule.actions:
                if action.output is not None and action.output in links[flowrule.match.port_in]:
                    #TODO: multiple output actions to be split not supported
                    endpoint_id = "autogenerated_split_"+flow_id
                    link_split[flowrule.match.port_in+"<->"+action.output] = endpoint_id
                    flowrule_left = flowrule
                    output = action.output
                    action.output = "endpoint:"+endpoint_id
                    flowrule_left.id = flow_id+"_1"
                    ep1 = EndPoint(_id=endpoint_id,_type="internal")
                    nffg_left.addEndPoint(ep1)
                    #Right graph
                    flowrule_2 = nffg_right.getFlowRule(flow_id)
                    flowrule_2.id = flow_id+"_2"
                    flowrule_2.match = Match(port_in="endpoint:"+endpoint_id)
                    flowrule_2.actions = []
                    flowrule_2.actions.append(Action(output=output))
                    ep2 = EndPoint(_id=endpoint_id,_type="internal")
                    nffg_right.addEndPoint(ep2)
                
        for flow_id in right_flows:
            flowrule = nffg_right.getFlowRule(flow_id)
            for action in flowrule.actions:
                if action.output is not None and action.output in links[flowrule.match.port_in]:
                    flowrule_right = flowrule
                    output = action.output
                    if action.output+"<->"+flowrule.match.port_in not in link_split:
                        link_split[action.output+"<->"+flowrule.match.port_in] = "autogenerated_split_"+flow_id
                    endpoint_id = link_split[action.output+"<->"+flowrule.match.port_in]
                    action.output = "endpoint:"+endpoint_id
                    flowrule_right.id = flow_id+"_1"
                    #Left graph
                    flowrule_left = nffg_left.getFlowRule(flow_id)
                    flowrule_left.id = flow_id+"_2"
                    flowrule_left.match = Match(port_in="endpoint:"+endpoint_id)
                    flowrule_left.actions = []
                    flowrule_left.actions.append(Action(output=output))    
                     
        #Delete obsolete parts from graphs
        marked_vnfs = None
        marked_endpoints=None
        for element in right_list_original:
            if type(element) is VNF:
                marked_vnfs, marked_endpoints = self.deleteVNFAndConnections(nffg_left, element, marked_vnfs, marked_endpoints)            
            elif type(element) is EndPoint:
                marked_vnfs, marked_endpoints = self.deleteEndpointAndConnections(nffg_left, element.id, marked_vnfs, marked_endpoints)
                
        marked_vnfs = None
        marked_endpoints=None       
        
        for element in left_list_original:
            if type(element) is VNF:
                marked_vnfs, marked_endpoints = self.deleteVNFAndConnections(nffg_right, element, marked_vnfs, marked_endpoints)        
            elif type(element) is EndPoint:
                marked_vnfs, marked_endpoints = self.deleteEndpointAndConnections(nffg_right, element.id, marked_vnfs, marked_endpoints)
        
        return nffg_left, nffg_right
        
    def deleteVNFAndConnections(self, nffg, vnf, marked_vnfs=None, marked_endpoints=None):
        '''
        Deletes the VNF and recursively connected VNFs and Endpoints.
        '''
        marked_vnfs = marked_vnfs or []        
        if vnf.id not in marked_vnfs:
            marked_vnfs.append(vnf.id)
            for port in vnf.ports:
                out_flows = nffg.deleteIncomingFlowrules("vnf:"+vnf.id+":"+port.id)
                for flow in out_flows:
                    for action in flow.actions:
                        if action.output is not None:
                            if action.output.split(":")[0] == "vnf":
                                marked_vnfs, marked_endpoints = self.deleteVNFAndConnections(nffg, nffg.getVNF(action.output.split(":")[1]), marked_vnfs, marked_endpoints)
                                break
                            elif action.output.split(":")[0] == "endpoint":
                                marked_vnfs, marked_endpoints = self.deleteEndpointAndConnections(nffg, action.output.split(":")[1], marked_vnfs, marked_endpoints)
                                break
                in_flows = nffg.deleteOutcomingFlowrules("vnf:"+vnf.id+":"+port.id)
                for flow in in_flows:
                    if flow.match is not None and flow.match.port_in is not None:
                        if flow.match.port_in.split(":")[0] == "vnf":
                            marked_vnfs, marked_endpoints = self.deleteVNFAndConnections(nffg, nffg.getVNF(flow.match.port_in.split(":")[1]), marked_vnfs, marked_endpoints)
                        elif flow.match.port_in.split(":")[0] == "endpoint":
                            marked_vnfs, marked_endpoints = self.deleteEndpointAndConnections(nffg, flow.match.port_in.split(":")[1], marked_vnfs, marked_endpoints)
            nffg.vnfs.remove(nffg.getVNF(vnf.id))
        return marked_vnfs, marked_endpoints

    def deleteEndpointAndConnections(self, nffg, endpoint_id, marked_vnfs=None, marked_endpoints=None):
        '''
        Deletes the Endpoint and recursively connected VNFs and Endpoints.
        '''     
        marked_endpoints = marked_endpoints or []
        if endpoint_id not in marked_endpoints:
            marked_endpoints.append(endpoint_id)
            out_flows = nffg.deleteIncomingFlowrules("endpoint:"+endpoint_id)
            for flow in out_flows:
                for action in flow.actions:
                    if action.output is not None:
                        if action.output.split(":")[0] == "vnf":
                            marked_vnfs, marked_endpoints = self.deleteVNFAndConnections(nffg, nffg.getVNF(action.output.split(":")[1]), marked_vnfs, marked_endpoints)
                            break
                        elif action.output.split(":")[0] == "endpoint":
                            marked_vnfs, marked_endpoints = self.deleteEndpointAndConnections(nffg, action.output.split(":")[1], marked_vnfs, marked_endpoints)
                            break
            in_flows = nffg.deleteOutcomingFlowrules("endpoint:"+endpoint_id)
            for flow in in_flows:
                if flow.match is not None and flow.match.port_in is not None:
                    if flow.match.port_in.split(":")[0] == "vnf":
                        marked_vnfs, marked_endpoints = self.deleteVNFAndConnections(nffg, nffg.getVNF(flow.match.port_in.split(":")[1]), marked_vnfs, marked_endpoints)
                    elif flow.match.port_in.split(":")[0] == "endpoint":
                        marked_vnfs, marked_endpoints = self.deleteEndpointAndConnections(nffg, flow.match.port_in.split(":")[1], marked_vnfs, marked_endpoints)
            nffg.end_points.remove(nffg.getEndPoint(endpoint_id))
        return marked_vnfs, marked_endpoints
    
    def getAutogeneratedEndpoints(self):
        endpoints = []
        for end_point in self.end_points:
            if end_point.id.startswith("autogenerated_split_"):
                endpoints.append(end_point)
        return endpoints
    
    def join(self, nffg_2):
        '''
        Merges two graphs that have been split previously. nffg_2 is left intact
        '''
        nffg_2 = copy.deepcopy(nffg_2)     
        removed_endpoints = []
        #Search for flowrules split and join them
        for flowrule in self.flow_rules[:]:
            if flowrule.id[-2] == '_':
                if flowrule.match is not None and flowrule.match.port_in is not None and flowrule.match.port_in.startswith("endpoint:autogenerated_split_"):
                    attaching_outgoing_flowrule = []
                    external_flowrule = nffg_2.getFlowRule(flowrule.id[:-1]+"1")
                    if external_flowrule is None:
                        continue
                    attaching_outgoing_flowrule.append(external_flowrule)
                    ingoing_flowrules = []
                    ingoing_flowrules.append(flowrule)
                    self.flow_rules =  self.flow_rules + self.mergeFlowrules(attaching_outgoing_flowrule, ingoing_flowrules, True)
                    self.flow_rules.remove(flowrule)
                    nffg_2.flow_rules.remove(external_flowrule)
                    if flowrule.match.port_in.split(":")[1] not in removed_endpoints:
                        removed_endpoints.append(flowrule.match.port_in.split(":")[1])
                elif flowrule.actions is not None:
                    for action in flowrule.actions:
                        if action.output is not None and action.output.startswith("endpoint:autogenerated_split_"):
                            attaching_ingoing_flowrule = []
                            external_flowrule = nffg_2.getFlowRule(flowrule.id[:-1]+"2")
                            if external_flowrule is None:
                                continue
                            attaching_ingoing_flowrule.append(external_flowrule)
                            outgoing_flowrules = []
                            outgoing_flowrules.append(flowrule)
                            self.flow_rules =  self.flow_rules + self.mergeFlowrules(outgoing_flowrules, attaching_ingoing_flowrule, True)
                            self.flow_rules.remove(flowrule)
                            nffg_2.flow_rules.remove(external_flowrule)
                            if action.output.split(":")[1] not in removed_endpoints:
                                removed_endpoints.append(action.output.split(":")[1])

                            
        self.flow_rules = self.flow_rules + nffg_2.flow_rules

        # Add the VNFs of the second graph
        for new_vnf in nffg_2.vnfs:
            self.vnfs.append(new_vnf)
        
        # Add the end-points of the second graph
        for new_end_point in nffg_2.end_points:
            self.end_points.append(new_end_point)
        for end_point in self.end_points[:]:
            if end_point.id in removed_endpoints:
                self.end_points.remove(end_point)
                        
    def deleteEndPointConnections(self, endpoint_id):
        return self.deleteConnections("endpoint:"+endpoint_id)
    
    def deleteConnectionsBetweenVNFs(self, vnf1_id, port_vnf1_id, vnf2_id, port_vnf2_id):
        deleted_flows=[]
        for flow_rule in self.flow_rules[:]:
            if flow_rule.match.port_in == 'vnf:'+vnf1_id+':'+port_vnf1_id:
                for action in flow_rule.actions:
                    if action.output == 'vnf:'+vnf2_id+':'+port_vnf2_id:
                        deleted_flows.append(copy.deepcopy(flow_rule))
                        self.flow_rules.remove(flow_rule)
    
            if flow_rule.match.port_in == 'vnf:'+vnf2_id+':'+port_vnf2_id:
                for action in flow_rule.actions:
                    if action.output == 'vnf:'+vnf1_id+':'+port_vnf1_id:
                        deleted_flows.append(copy.deepcopy(flow_rule))
                        self.flow_rules.remove(flow_rule)
        return deleted_flows
    
    def deleteVNFConnections(self, vnf_id):
        vnf = self.getVNF(vnf_id)
        deleted_flows=[]
        for port in vnf.ports:
            deleted_flows = deleted_flows + self.deleteConnections("vnf:"+vnf_id+":"+port.id)
        return deleted_flows
    
    def deleteConnections(self, node_id):
        deleted_flows = self.deleteIncomingFlowrules(node_id)
        deleted_flows = deleted_flows + self.deleteOutcomingFlowrules(node_id)
        return deleted_flows
    
    def deleteIncomingFlowrules(self, node_id):
        deleted_flows = []
        for flow_rule in self.flow_rules[:]:
            if flow_rule.match.port_in == node_id:
                deleted_flows.append(copy.deepcopy(flow_rule))
                self.flow_rules.remove(flow_rule)
                continue
        return deleted_flows
    
    def deleteOutcomingFlowrules(self, node_id):
        deleted_flows = []
        for flow_rule in self.flow_rules[:]:
            for action in flow_rule.actions:
                if action.output == node_id:
                    deleted_flows.append(copy.deepcopy(flow_rule))
                    self.flow_rules.remove(flow_rule)
        return deleted_flows
                    
class VNF(object):
    def __init__(self, _id = None, name = None,
                vnf_template_location = None, ports = None,
                groups = None, template = None, status = None,
                db_id = None, internal_id = None, availabilty_zone = None, 
                domain = None, unify_control = None, unify_env_variables = None):
        self.id = _id
        self.name = name
        self.vnf_template_location = vnf_template_location
        self.template = template
        self.ports = ports or []
        self.groups = groups or []
        self.status = status
        self.db_id = db_id
        self.internal_id = internal_id
        self.availabilty_zone = availabilty_zone
        self.domain = domain
        self.unify_control = unify_control or []
        self.unify_env_variables = unify_env_variables or []
    
    def parseDict(self, vnf_dict):
        self.id = vnf_dict['id']
        if 'name' in vnf_dict:
            self.name = vnf_dict['name']
        if 'vnf_template' in vnf_dict:
            self.vnf_template_location = vnf_dict['vnf_template']
        for port_dict in vnf_dict['ports']:
            port = Port()
            port.parseDict(port_dict)
            self.ports.append(port)
        if 'unify-control' in vnf_dict:
            for control_dict in vnf_dict['unify-control']:
                control = UnifyControl()
                control.parseDict(control_dict)
                self.unify_control.append(control)     
        if 'unify-env-variables' in vnf_dict:
            for env_variable_dict in vnf_dict['unify-env-variables']:
                env_variable = env_variable_dict['variable']
                self.unify_env_variables.append(env_variable)                         
        if 'groups' in vnf_dict:
            self.groups = vnf_dict['groups']
        if 'domain' in vnf_dict:
            self.domain = vnf_dict['domain']            
        
    def getDict(self, extended = False):
        vnf_dict = {}
        if self.id is not None:
            vnf_dict['id'] = self.id 
        if self.name is not None:
            vnf_dict['name'] = self.name
        if self.vnf_template_location is not None:
            vnf_dict['vnf_template'] = self.vnf_template_location
        ports_dict = []
        for port in self.ports:
            ports_dict.append(port.getDict(extended))
        if ports_dict:
            vnf_dict['ports'] = ports_dict
        control_dict = []
        for control in self.unify_control:
            control_dict.append(control.getDict(extended))
        if control_dict:
            vnf_dict['unify-control'] = control_dict   
        unify_env_variables_dict = []
        for variable in self.unify_env_variables:
            variable_dict={}
            variable_dict['variable'] = variable
            unify_env_variables_dict.append(variable_dict)            
        if unify_env_variables_dict:
            vnf_dict['unify-env-variables'] = unify_env_variables_dict       
        if self.groups:
            vnf_dict['groups'] = self.groups
        if extended is True:
            if self.status is not None:
                vnf_dict['status'] = self.status 
            if self.db_id is not None:
                vnf_dict['db_id'] = self.db_id
            if self.internal_id is not None:
                vnf_dict['internal_id'] = self.internal_id
            if self.domain is not None:
                vnf_dict['domain'] = self.domain                
        return vnf_dict
    
    def getPort(self, port_id):
        for port in self.ports:
            if port.id == port_id:
                return port
    
    def addPort(self, port):
        if type(port) is Port:
            self.ports.append(port)
        else:
            raise TypeError("Tried to add a port with a wrong type. Expected Port, found "+type(port))    
        return port
    
    def addTemplate(self, template):
        self.template = template
        
    def checkPortsAgainstTemplate(self):
        labels = {}
        for port in self.ports:
            if port.id.split(':')[0] in labels:
                labels[port.id.split(':')[0]] += 1
            else:
                labels[port.id.split(':')[0]] = 1
        
        
        for (label, num_ports_per_label) in labels.items():
            found = False
            for template_port in self.template.ports:
                if label == template_port.label:
                    if template_port.position.split('-')[1] == 'N' or template_port.position.split('-')[1] == 'n':
                        maximum_number_of_ports_per_label = sys.maxsize
                    else:
                        maximum_number_of_ports_per_label = int(template_port.position.split('-')[1]) - int(template_port.position.split('-')[0]) + 1
                    minimum_number_of_ports_per_label = int(template_port.min)
                    found = True
                    
            # Check if the label of the vnf's ports exist in the template
            if found is False:
                raise InexistentLabelFound("Inexistent label found: "+str(label))
            
            # Check maximum number of port per label
            if num_ports_per_label > maximum_number_of_ports_per_label:
                raise WrongNumberOfPorts("The maximum number of port for: "+str(label)+" is: "+str(maximum_number_of_ports_per_label)+" found: "+str(num_ports_per_label))
            
            # Check minimum number of port per label
            if num_ports_per_label < minimum_number_of_ports_per_label:
                raise WrongNumberOfPorts("The minimum number of port for: "+str(label)+" is: "+str(minimum_number_of_ports_per_label)+" found: "+str(num_ports_per_label))
        
        for port in self.ports:
            # TODO: Check that, the relative index of the port doesn't exceed the maximum number of ports for that label
            pass
    
    def getHigherReletiveIDForPortLabel(self, label):
        max_relative_id = -1
        for port in self.ports:
            if port.id.split(":")[0] == label:
                if port.id.split(":")[1] > max_relative_id:
                    max_relative_id = port.id.split(":")[1]
        return max_relative_id
        
class Port(object):
    def __init__(self, _id = None, name = None, _type = None, status = None, db_id = None, mac = None, unify_ip = None, internal_id = None):
        self.id = _id
        self.name = name
        self.mac = mac
        self.unify_ip = unify_ip        
        self.type = _type
        self.status = status
        self.db_id = db_id
        self.internal_id = internal_id
        
    def parseDict(self, port_dict):
        self.id = port_dict['id']
        if 'name' in port_dict:
            self.name = port_dict['name']
        if 'mac' in port_dict:
            self.mac = port_dict['mac']
        if 'unify-ip' in port_dict:
            self.unify_ip = port_dict['unify-ip']                        
        
    def getDict(self, extended = False):
        port_dict = {}
        if self.id is not None:
            port_dict['id'] = self.id
        if self.name is not None:
            port_dict['name'] = self.name
        if self.mac is not None:
            port_dict['mac'] = self.mac
        if self.unify_ip is not None:
            port_dict['unify-ip'] = self.unify_ip            
        if extended is True:
            if self.type is not None:
                port_dict['type'] = self.type
            if self.status is not None:
                port_dict['status'] = self.status 
            if self.db_id is not None:
                port_dict['db_id'] = self.db_id
            if self.internal_id is not None:
                port_dict['internal_id'] = self.internal_id            
        return port_dict
    
class UnifyControl(object):
    def __init__(self, host_tcp_port = None, vnf_tcp_port = None):
        self.host_tcp_port = host_tcp_port
        self.vnf_tcp_port = vnf_tcp_port
        
    def parseDict(self, control_dict):
        self.host_tcp_port = control_dict['host-tcp-port']
        self.vnf_tcp_port = control_dict['vnf-tcp-port']
        
    def getDict(self, extended = False):
        control_dict = {}
        if self.host_tcp_port is not None:
            control_dict['host-tcp-port'] = self.host_tcp_port
        if self.vnf_tcp_port is not None:
            control_dict['vnf-tcp-port'] = self.vnf_tcp_port             
        return control_dict          

class EndPoint(object):
    def __init__(self, _id = None, name = None, _type = None, 
                 remote_endpoint_id = None, node_id = None, switch_id = None,
                 interface = None, remote_ip = None, local_ip = None, gre_key = None, ttl = None, secure_gre = None, 
                 status = None, db_id = None, internal_id = None, vlan_id = None, 
                 interface_internal_id = None, 
                 prepare_connection_to_remote_endpoint_id = None,
                 prepare_connection_to_remote_endpoint_ids = None, domain = None):
        '''
        Parameters
        ----------
        _id : string
           Id of the end-point.
        name : string
           Name of the end-point.
        _type : string
           Type of the end-point, it could be 'internal', 'interface', 'interface_out', 'gre_tunnel', 'vlan'.
        remote_endpoint_id : string
           Identify the remote end-point where this end-point will be connected.
           The layout of the string will be: 'id-remote-graph:id-remote-endpoint'.
           It is an optional field.
        prepare_connection_to_remote_endpoint_ids : string
            Identify the remote end-points where this end-point will be connected.
           The layout of the string will be: 'id-remote-graph:id-remote-endpoint'.
           It is an optional field. Unlike the field  remote_endpoint_id, the field
           prepare_connection_to_remote_endpoint_ids does not trigger an attach operation
           but only prepare the graph for a forthcoming connection.
        prepare_connection_to_remote_endpoint_id : string
            Is a field for the auto-generate 1 to 1 end-point (in the end-point switch model), and identifies
            the remote end-points where this end-point will be connected.
        node_id : string
           Optional field. Its meaning depends on the value of field _type
        switch_id : string
           Optional field. Its meaning depends on the value of field _type
        interface : string
           Optional field. Its meaning depends on the value of field _type
        remote_ip : string
           Optional field. Its meaning depends on the value of field _type
        local_ip : string
           Optional field. Its meaning depends on the value of field _type
        gre_key : string
           Optional field. Its meaning depends on the value of field _type            
        ttl : string
           Optional field. Its meaning depends on the value of field _type          
        '''
        self.id = _id
        self.name = name
        self.type = _type
        self.remote_endpoint_id = remote_endpoint_id
        self.node_id = node_id
        self.switch_id = switch_id
        self.interface = interface
        self.remote_ip = remote_ip
        self.local_ip = local_ip
        self.gre_key = gre_key
        self.ttl = ttl
        self.secure_gre = secure_gre
        self.vlan_id = vlan_id
        self.status = status
        self.db_id = db_id
        self.internal_id = internal_id
        self.interface_internal_id = interface_internal_id
        self.prepare_connection_to_remote_endpoint_id = prepare_connection_to_remote_endpoint_id
        self.prepare_connection_to_remote_endpoint_ids = prepare_connection_to_remote_endpoint_ids or []
        self.domain = domain        
        
    def parseDict(self, end_point_dict):
        self.id = end_point_dict['id']
        if 'name' in end_point_dict:
            self.name = end_point_dict['name']
        if 'domain' in end_point_dict:
            self.domain = end_point_dict['domain']            
        if 'remote_endpoint_id' in end_point_dict:
            self.remote_endpoint_id = end_point_dict['remote_endpoint_id']
        if 'prepare_connection_to_remote_endpoint_ids' in end_point_dict:
            self.prepare_connection_to_remote_endpoint_ids = end_point_dict['prepare_connection_to_remote_endpoint_ids']
        if 'type' in end_point_dict:
            self.type = end_point_dict['type']
            if self.type == 'interface' or self.type == 'interface-out':
                self.interface = end_point_dict[self.type]['interface']
                if 'node-id' in end_point_dict[self.type]:
                    self.node_id = end_point_dict[self.type]['node-id']
                if 'switch-id' in end_point_dict[self.type]:
                    self.switch_id = end_point_dict[self.type]['switch-id']
            elif self.type == 'gre-tunnel':
                self.remote_ip = end_point_dict[self.type]['remote-ip']
                self.local_ip = end_point_dict[self.type]['local-ip']
                self.interface = end_point_dict[self.type]['interface']
                self.gre_key = end_point_dict[self.type]['gre-key']                
                if 'ttl' in end_point_dict[self.type]:
                    self.ttl = end_point_dict[self.type]['ttl'] 
                if 'secure' in end_point_dict[self.type]:
                    self.secure_gre = end_point_dict[self.type]['secure']                       
            elif self.type == 'vlan':
                self.interface = end_point_dict[self.type]['interface']
                self.vlan_id = end_point_dict[self.type]['vlan-id']
                if 'node-id' in end_point_dict[self.type]:
                    self.node_id = end_point_dict[self.type]['node-id']
                if 'switch-id' in end_point_dict[self.type]:
                    self.switch_id = end_point_dict[self.type]['switch-id']
         
    def getDict(self, extended = False):
        end_point_dict = {}
        if self.id is not None:
            end_point_dict['id'] = self.id
        if self.name is not None:
            end_point_dict['name'] = self.name
        if self.remote_endpoint_id is not None:
            end_point_dict['remote_endpoint_id'] = self.remote_endpoint_id  
        if self.prepare_connection_to_remote_endpoint_ids:
            end_point_dict['prepare_connection_to_remote_endpoint_ids'] = self.prepare_connection_to_remote_endpoint_ids 
        if self.prepare_connection_to_remote_endpoint_id is not None:
            end_point_dict['prepare_connection_to_remote_endpoint_id'] = self.prepare_connection_to_remote_endpoint_id 
        if self.type is not None:
            end_point_dict['type'] = self.type
            if self.type != 'internal' and self.type != 'shadow':
                end_point_dict[self.type] = {}
                if self.node_id is not None:
                    end_point_dict[self.type]['node-id'] = self.node_id
                if self.switch_id is not None:
                    end_point_dict[self.type]['switch-id'] = self.switch_id     
                if self.interface is not None:
                    end_point_dict[self.type]['interface'] = self.interface
                if self.remote_ip is not None:
                    end_point_dict[self.type]['remote-ip'] = self.remote_ip  
                if self.local_ip is not None:
                    end_point_dict[self.type]['local-ip'] = self.local_ip     
                if self.gre_key is not None:
                    end_point_dict[self.type]['gre-key'] = self.gre_key                   
                if self.ttl is not None:
                    end_point_dict[self.type]['ttl'] = self.ttl
                if self.secure_gre is not None:
                    end_point_dict[self.type]['secure'] = self.secure_gre                    
                if self.vlan_id is not None:
                    end_point_dict[self.type]['vlan-id'] = self.vlan_id             
        if extended is True:
            if self.status is not None:
                end_point_dict['status'] = self.status 
            if self.db_id is not None:
                end_point_dict['db_id'] = self.db_id
            if self.internal_id is not None:
                end_point_dict['internal_id'] = self.internal_id
            if self.interface_internal_id is not None:
                end_point_dict['interface_internal_id'] = self.interface_internal_id
            if self.domain is not None:
                end_point_dict['domain'] = self.domain                 
        return end_point_dict   

class FlowRule(object):
    def __init__(self, _id = None, priority = None,
                 match = None, actions = None, status = None,
                 db_id = None, internal_id = None, _type = None,
                 node_id = None, description = None):
        self.id = _id
        self.description = description
        self.priority = priority
        self.match = match
        self.actions = actions or []
        self.status = status
        self.db_id = db_id
        self.internal_id = internal_id
        self.type = _type
        self.node_id = node_id
    
    def parseDict(self, flow_rule_dict):
        self.id = flow_rule_dict['id']
        if 'description' in flow_rule_dict:
            self.description = flow_rule_dict['description']
        self.priority = flow_rule_dict['priority']
        match = Match()
        match.parseDict(flow_rule_dict['match'])
        self.match = match
        for action_dict in flow_rule_dict['actions']:
            action = Action()
            action.parseDict(action_dict)
            self.actions.append(action)
        
    def getDict(self, extended = False):
        flow_rule_dict = {}
        if self.id is not None:
            flow_rule_dict['id'] = self.id
        if self.description is not None:
            flow_rule_dict['description'] = self.description
        if self.priority is not None:
            flow_rule_dict['priority'] = self.priority
        if self.match is not None:
            flow_rule_dict['match'] = self.match.getDict(extended)
        actions = []
        for action in self.actions:
            actions.append(action.getDict(extended))
        flow_rule_dict['actions'] = actions
        if extended is True:
            if self.status is not None:
                flow_rule_dict['status'] = self.status 
            if self.db_id is not None:
                flow_rule_dict['db_id'] = self.db_id
            if self.internal_id is not None:
                flow_rule_dict['internal_id'] = self.internal_id           
            if self.type is not None:
                flow_rule_dict['type'] = self.type    
            if self.node_id is not None:
                flow_rule_dict['node_id'] = self.node_id    
        return flow_rule_dict
          
    def changePortOfFlowRule(self, old_port_id, new_port_id):
        if self.match.port_in == old_port_id:
            self.match.port_in = new_port_id
        for action in self.actions:
            if action.output == old_port_id:
                action.output = new_port_id
    
class Match(object):
    def __init__(self, port_in=None, 
                 ether_type = None, vlan_id = None, 
                 vlan_priority = None, source_mac = None,
                 dest_mac = None, source_ip = None,
                 dest_ip = None, tos_bits = None,
                 source_port = None, dest_port = None,
                 protocol = None, db_id = None,
                 arp_spa = None, arp_tpa = None):
        '''
        It contains all the openflow 1.0 match types.
        
        uint32_t wildcards; /* Wildcard fields. */
        uint16_t in_port; /* Input switch port. */
        uint8_t dl_src[OFP_ETH_ALEN]; /* Ethernet source address. */
        uint8_t dl_dst[OFP_ETH_ALEN]; /* Ethernet destination address. */
        uint16_t dl_vlan; /* Input VLAN id. */
        uint8_t dl_vlan_pcp; /* Input VLAN priority. */
        uint8_t pad1[1]; /* Align to 64-bits */
        uint16_t dl_type; /* Ethernet frame type. */
        uint8_t nw_tos; /* IP ToS (actually DSCP field, 6 bits). */
        uint8_t nw_proto; /* IP protocol or lower 8 bits of
        * ARP opcode. */
        uint8_t pad2[2]; /* Align to 64-bits */
        uint32_t nw_src; /* IP source address. */
        uint32_t nw_dst; /* IP destination address. */
        uint16_t tp_src; /* TCP/UDP source port. */
        uint16_t tp_dst; /* TCP/UDP destination port. */
        '''
        self.port_in = port_in
        self.ether_type = ether_type
        self.vlan_id = vlan_id
        self.vlan_priority = vlan_priority
        self.source_mac = source_mac
        self.dest_mac = dest_mac
        self.source_ip = source_ip
        self.dest_ip = dest_ip
        self.tos_bits = tos_bits
        self.source_port = source_port
        self.dest_port = dest_port
        self.protocol = protocol
        self.arp_spa = arp_spa
        self.arp_tpa = arp_tpa
        self.db_id = db_id
    
    def parseDict(self, match_dict):
        if 'port_in' in match_dict:
            self.port_in = match_dict['port_in']
        if 'ether_type' in match_dict:
            self.ether_type = match_dict['ether_type']
        if 'vlan_id' in match_dict:
            self.vlan_id = match_dict['vlan_id']
        if 'vlan_priority' in match_dict:
            self.vlan_priority = match_dict['vlan_priority']       
        if 'source_mac' in match_dict:
            self.source_mac = match_dict['source_mac']
        if 'dest_mac' in match_dict:
            self.dest_mac = match_dict['dest_mac']
        if 'source_ip' in match_dict:
            self.source_ip = match_dict['source_ip']
        if 'dest_ip' in match_dict:
            self.dest_ip = match_dict['dest_ip']  
        if 'tos_bits' in match_dict:
            self.tos_bits = match_dict['tos_bits']
        if 'source_port' in match_dict:
            self.source_port = match_dict['source_port']
        if 'dest_port' in match_dict:
            self.dest_port = match_dict['dest_port']
        if 'protocol' in match_dict:
            self.protocol = match_dict['protocol']     
        if 'arp_spa' in match_dict:
            self.arp_spa = match_dict['arp_spa']                                                
        if 'arp_tpa' in match_dict:
            self.arp_tpa = match_dict['arp_tpa']  
        
    def getDict(self, extended = False):
        match_dict = {} 
        if self.port_in is not None:
            match_dict['port_in'] = self.port_in  
        if self.ether_type is not None:
            match_dict['ether_type'] = self.ether_type  
        if self.vlan_id is not None:
            match_dict['vlan_id'] = self.vlan_id  
        if self.vlan_priority is not None:
            match_dict['vlan_priority'] = self.vlan_priority  
        if self.source_mac is not None:
            match_dict['source_mac'] = self.source_mac  
        if self.dest_mac is not None:
            match_dict['dest_mac'] = self.dest_mac  
        if self.source_ip is not None:
            match_dict['source_ip'] = self.source_ip  
        if self.dest_ip is not None:
            match_dict['dest_ip'] = self.dest_ip
        if self.tos_bits is not None:
            match_dict['tos_bits'] = self.tos_bits  
        if self.source_port is not None:
            match_dict['source_port'] = self.source_port  
        if self.dest_port is not None:
            match_dict['dest_port'] = self.dest_port
        if self.protocol is not None:
            match_dict['protocol'] = self.protocol
        if self.arp_spa is not None:
            match_dict['arp_spa'] = self.arp_spa  
        if self.arp_tpa is not None:
            match_dict['arp_tpa'] = self.arp_tpa              
        if extended is True:
            if self.db_id is not None:
                match_dict['db_id'] = self.db_id
        return match_dict

class Action(object):
    def __init__(self, output = None, controller = False, drop=False, set_vlan_id = None,
                 set_vlan_priority = None, push_vlan = None, pop_vlan = False,
                 set_ethernet_src_address = None, set_ethernet_dst_address= None,
                 set_ip_src_address = None, set_ip_dst_address= None,
                 set_ip_tos = None, set_l4_src_port=None,
                 set_l4_dst_port = None, output_to_queue= None, db_id = None):
        '''
        It contains all the openflow 1.0 action types.
        
        OFPAT_OUTPUT, /* Output to switch port. */
        OFPAT_SET_VLAN_VID, /* Set the 802.1q VLAN id. */
        OFPAT_SET_VLAN_PCP, /* Set the 802.1q priority. */
        OFPAT_STRIP_VLAN, /* Strip the 802.1q header. */
        OFPAT_SET_DL_SRC, /* Ethernet source address. */
        OFPAT_SET_DL_DST, /* Ethernet destination address. */
        OFPAT_SET_NW_SRC, /* IP source address. */
        OFPAT_SET_NW_DST, /* IP destination address. */
        OFPAT_SET_NW_TOS, /* IP ToS (DSCP field, 6 bits). */
        OFPAT_SET_TP_SRC, /* TCP/UDP source port. */
        OFPAT_SET_TP_DST, /* TCP/UDP destination port. */
        OFPAT_ENQUEUE, /* Output to queue. */
        '''
        self.output = output
        self.controller = controller
        self.drop = drop
        self.set_vlan_id = set_vlan_id
        self.set_vlan_priority = set_vlan_priority
        self.push_vlan = push_vlan
        self.pop_vlan = pop_vlan
        self.set_ethernet_src_address = set_ethernet_src_address
        self.set_ethernet_dst_address = set_ethernet_dst_address
        self.set_ip_src_address = set_ip_src_address
        self.set_ip_dst_address = set_ip_dst_address
        self.set_ip_tos = set_ip_tos
        self.set_l4_src_port = set_l4_src_port
        self.set_l4_dst_port = set_l4_dst_port
        self.output_to_queue = output_to_queue
        self.db_id = db_id
        
    def parseDict(self, action_dict):
        if 'output_to_port' in action_dict:
            self.output = action_dict['output_to_port']
        if 'output_to_controller' in action_dict:
            self.controller = action_dict['output_to_controller']
        if 'drop' in action_dict:
            self.drop = action_dict['drop']
        if 'set_vlan_id' in action_dict:
            self.set_vlan_id = action_dict['set_vlan_id']
        if 'set_vlan_priority' in action_dict:
            self.set_vlan_priority = action_dict['set_vlan_priority']
        if 'push_vlan' in action_dict:
            self.push_vlan = action_dict['push_vlan']            
        if 'pop_vlan' in action_dict:
            self.pop_vlan = action_dict['pop_vlan']         
        if 'set_ethernet_src_address' in action_dict:
            self.set_ethernet_src_address = action_dict['set_ethernet_src_address']
        if 'set_ethernet_dst_address' in action_dict:
            self.set_ethernet_dst_address = action_dict['set_ethernet_dst_address']
        if 'set_ip_src_address' in action_dict:
            self.set_ip_src_address = action_dict['set_ip_src_address']
        if 'set_ip_dst_address' in action_dict:
            self.set_ip_dst_address = action_dict['set_ip_dst_address']  
        if 'set_ip_tos' in action_dict:
            self.set_ip_tos = action_dict['set_ip_tos']
        if 'set_l4_src_port' in action_dict:
            self.set_l4_src_port = action_dict['set_l4_src_port']
        if 'set_l4_dst_port' in action_dict:
            self.set_l4_dst_port = action_dict['set_l4_dst_port']
        if 'output_to_queue' in action_dict:
            self.output_to_queue = action_dict['output_to_queue']                              
    
    def getDict(self, extended = False):
        action_dict = {}      
        if self.output is not None:
            action_dict['output_to_port'] = self.output  
        if self.controller is not False:
            action_dict['output_to_controller'] = self.controller  
        if self.drop is not False:
            action_dict['drop'] = self.drop  
        if self.set_vlan_id is not None:
            action_dict['set_vlan_id'] = self.set_vlan_id
        if self.set_vlan_priority is not None:
            action_dict['set_vlan_priority'] = self.set_vlan_priority  
        if self.push_vlan is not None:
            action_dict['push_vlan'] = self.push_vlan              
        if self.pop_vlan is not False:
            action_dict['pop_vlan'] = self.pop_vlan  
        if self.set_ethernet_src_address is not None:
            action_dict['set_ethernet_src_address'] = self.set_ethernet_src_address  
        if self.set_ethernet_dst_address is not None:
            action_dict['set_ethernet_dst_address'] = self.set_ethernet_dst_address  
        if self.set_ip_src_address is not None:
            action_dict['set_ip_src_address'] = self.set_ip_src_address  
        if self.set_ip_dst_address is not None:
            action_dict['set_ip_dst_address'] = self.set_ip_dst_address  
        if self.set_ip_tos is not None:
            action_dict['set_ip_tos'] = self.set_ip_tos
        if self.set_l4_src_port is not None:
            action_dict['set_l4_src_port'] = self.set_l4_src_port  
        if self.set_l4_dst_port is not None:
            action_dict['set_l4_dst_port'] = self.set_l4_dst_port  
        if self.output_to_queue is not None:
            action_dict['output_to_queue'] = self.output_to_queue
        if extended is True:
            if self.db_id is not None:
                action_dict['db_id'] = self.db_id            
        return action_dict
        
