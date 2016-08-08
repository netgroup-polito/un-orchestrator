#!/usr/bin/env python
from exception import ClientError, ServerError
from collections import OrderedDict
from requests.exceptions import HTTPError
__author__ = 'Ivano Cerrato, Stefano Petrangeli'

import falcon
import json
import logging
import requests
import ConfigParser
import re

import constants
from virtualizer_library.virtualizer import ET, Virtualizer,  Software_resource, Infra_node, Port as Virt_Port
from un_native_nffg_library.nffg import NF_FG, VNF, Match, Action, EndPoint, FlowRule, Port, UnifyControl

class DoPing:
	'''
	This class manages the ping
	'''

	def on_get(self,req,resp):
		resp.body = "OK"
		resp.status = falcon.HTTP_200
		
	def on_post(self,req,resp):
		resp.body = "OK"
		resp.status = falcon.HTTP_200

class DoUsage:
	'''
	This class shows how to interact with the virtualizer
	'''
	
	def __init__(self):
		a = 'usage:\n'
		b = '\tget http://hostip:tcpport - this help message\n'
		c = '\tget http://hostip:tcpport/ping - test webserver aliveness\n'
		d = '\tpost http://hostip:tcpport/get-config - query NF-FG\n'
		e = '\tpost http://hostip:tcpport/edit-config - send NF-FG request in the post body\n'
		f = '\n'
		g = 'limitations (of the universal node orchestrator):\n'
		h = '\tthe flowrule ID must be unique on the node.\n'
		i = '\ttype cannot be repeated in multiple NF instances.\n'
		j = '\tcapabilities are not supported.\n'
		k = '\tit is not possible to deploy a VNF that does not have any flow referring its ports.\n'	
		l = '\ta VNF is removed (undeployed) only when no flows still remain that refer to its ports.\n'
		m = '\tthe number of ports actually attached to a VNF depends on the number of ports used in the flows'
		N = '\n\n'
		self._answer = a + b + c + d + e + f + g + h + i + j + k + l + m + N

	def on_get(self,req,resp):
		resp.body = self._answer
		resp.status = falcon.HTTP_200
		
	def on_post(self,req,resp):
		resp.body = self._answer
		resp.status = falcon.HTTP_200

class DoGetConfig:

	def on_post(self,req,resp):
		'''
		Return the current configuration of the node.
		'''
		LOG.info("Executing the 'get-config' command")
		LOG.debug("Reading file: %s",constants.GRAPH_XML_FILE)

		try:
			tree = ET.parse(constants.GRAPH_XML_FILE)
		except ET.ParseError as e:
			print('ParseError: %s' % e.message)
			resp.status = falcon.HTTP_500
			return
			
		LOG.debug("File correctly read")
	
		infrastructure = Virtualizer.parse(root=tree.getroot())
	
		LOG.debug("%s",infrastructure.xml())

		resp.content_type = "text/xml"
		resp.body = infrastructure.xml()
		resp.status = falcon.HTTP_200

		LOG.info("'get-config' command properly handled")

class DoEditConfig:
	
	def on_post(self, req, resp):
		'''
		Edit the configuration of the node
		'''
		global unify_monitoring
		try:
			LOG.info("Executing the 'edit-config' command")
			content = req.stream.read()
			#content = req
			
			LOG.debug("Body of the request:")
			LOG.debug("%s",content)
			
			
			checkCorrectness(content)
			
			if operation_type == "netconf-like":
				nffg = readGraphFile()
			else:
				# in full-content mode the current state of the UN is discarded
				nffg = NF_FG()
				with open(constants.GRAPH_XML_FILE, 'w') as f:
					f.write(base_xml)
			
			#
			#	Extract the needed information from the message received from the network
			#
			
			processVNFs(nffg, content)
			
			processRules(nffg, content)
			
			#
			# Interact with the universal node orchestrator in order to implement the required commands
			#
			
			sendToUniversalNode(nffg)
		
			# 
			# The required modifications have been implemented in the universal node, then we can update the
			# configuration saved in the proper files
			#

			if operation_type == "netconf-like":
				writeGraphFile(nffg)
			
			un_config = updateUniversalNodeConfig(content) #Updates the file containing the current configuration of the universal node, by editing the #<flowtable> and the <NF_instances> and returning the xml
			
			resp.content_type = "text/xml"
			resp.body = un_config
			resp.status = falcon.HTTP_200

			unify_monitoring = ""

			LOG.info("'edit-config' command properly handled")
			
		except ClientError as ex:
			raise falcon.HTTPBadRequest("Client Error", ex.message)

		except ServerError as ex:
			LOG.error("Please, press 'ctrl+c' and restart the virtualizer.")
			LOG.error("Please, also restart the universal node orchestrator.")
			raise falcon.HTTPInternalServerError('Server Error', ex.message)

		except Exception as ex:
			LOG.exception(ex)
			raise falcon.HTTPInternalServerError('Internal Server Error', str(ex))

def checkCorrectness(newContent):
	'''
	Check if the new configuration of the node (in particular, the flowtable) is correct:
	*	the ports are part of the universal node
	*	the VNFs referenced in the flows are instantiated
	'''
	
	LOG.debug("Checking the correctness of the new configuration...")

	LOG.debug("Reading file '%s', which contains the current configuration of the universal node...",constants.GRAPH_XML_FILE)
	try:
		oldTree = ET.parse(constants.GRAPH_XML_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ServerError("ParseError: %s" % e.message)
	LOG.debug("File correctly read")
		
	infrastructure = Virtualizer.parse(root=oldTree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	nfInstances = universal_node.NF_instances
	
	#tmpInfra = copy.deepcopy(infrastructure)
	
	LOG.debug("Getting the new flowrules to be installed on the universal node")
	try:
		newTree = ET.ElementTree(ET.fromstring(newContent))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ClientError("ParseError: %s" % e.message)
							
	newInfrastructure = Virtualizer.parse(root=newTree.getroot())
	newFlowtable = newInfrastructure.nodes.node[constants.NODE_ID].flowtable
	newNfInstances = newInfrastructure.nodes.node[constants.NODE_ID].NF_instances
							
	#Update the NF instances with the new NFs
	try:
		for instance in newNfInstances:
			if instance.get_operation() == 'delete':
				nfInstances[instance.id.get_value()].delete()
			else:
				nfInstances.add(instance)
	except KeyError:
		raise ClientError("Trying to delete a VNF that does not exist! ID: " + instance.id.get_value())
			
	#Update the flowtable with the new flowentries
	try:
		for flowentry in newFlowtable:
			if flowentry.get_operation() == 'delete':
				flowtable[flowentry.id.get_value()].delete()
			else:
				flowtable.add(flowentry) 
	except KeyError:
		LOG.error("Trying to delete a flowrule that does not exist! ID:%s", flowentry.id.get_value())
		raise ClientError("Trying to delete a flowrule that does not exist! ID: "+ flowentry.id.get_value())

	#Here, infrastructure contains the new configuration of the node
	#Then, we execute the checks on it!
	
	LOG.debug("The new configuration of the universal node is correct!")
			
def processVNFs(nffg, content):
	'''
	Parses the message and extracts the type of the deployed network functions.
	
	As far as I understand, the 'type' in a NF is the linker between <NF_instances>
	and <capabilities><supported_NFs>. Then, this function also checks that the type
	of the NF to be instantiated is among those to be supported by the universal node
	'''
	
	global tcp_port, unify_port_mapping, unify_monitoring
	
	try:
		tree = ET.parse(constants.GRAPH_XML_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ServerError("ParseError: %s" % e.message)
	
	currentInfrastructure = Virtualizer.parse(root=tree.getroot())
	supportedNFs = currentInfrastructure.nodes.node[constants.NODE_ID].capabilities.supported_NFs
	supportedTypes = []
	#lowerPortId = {}
	for nf in supportedNFs:
		nfType = nf.type.get_value()
		supportedTypes.append(nfType)
		#lowerPortId[nfType] = getLowerPortId(nf)
		
	LOG.debug("Extracting the network functions (to be) deployed on the universal node")
	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ClientError("ParseError: %s" % e.message)
	
	infrastructure = Virtualizer.parse(root=tree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	instances = universal_node.NF_instances
	
	LOG.debug("Considering instances:")
	LOG.debug("'%s'",infrastructure.xml())
	
	for instance in instances:
		if operation_type == "netconf-like":
			if instance.get_operation() is None:
				LOG.warning("VNF {0} has no operation set and will be ignored".format(instance.id.get_value()))
				continue
			
			elif instance.get_operation() == 'delete':
				#This network function has to be removed from the universal node
				vnf_to_be_removed = nffg.getVNF(instance.id.get_value())
				if vnf_to_be_removed is None:
					LOG.error("Trying to delete a VNF that is not currently deployed. Vnf id: " + instance.id.get_value())
					raise ClientError("Trying to delete a VNF that is not currently deployed. Vnf id: " + instance.id.get_value())
				LOG.debug("Network function '%s' has to be removed",vnf_to_be_removed.id)
				nffg.vnfs.remove(vnf_to_be_removed)
				continue
	
			elif instance.get_operation() != 'create':
				LOG.error("Unsupported operation for vnf: " + instance.id.get_value())
				raise ClientError("Unsupported operation for vnf: "+instance.id.get_value())
			
		vnfType = instance.type.get_value()
		if vnfType not in supportedTypes:
			LOG.error("VNF of type '%s' is not supported by the UN!",vnfType)
			raise ClientError("VNF of type "+ vnfType +" is not supported by the UN!")
		
		port_list = []
		unify_control = []
		unify_env_variables = []
		for port_id in instance.ports.port:
			port = instance.ports[port_id]
			l4_addresses = port.addresses.l4.get_value()
			# only process l4 address for new VNFs to be created
			#if l4_addresses is not None and instance.get_operation() == 'create':
			if l4_addresses is not None:
				if int(port.id.get_value()) != 0:
					LOG.error("L4 configuration is supported only to the port with id = 0 on VNF of type '%s'", vnfType)
					raise ClientError("L4 configuration is supported only to the port with id = 0 on VNF of type " + vnfType)
				# find all the l4_addresses with regular expression and reformat to request notation "{protocol/port,}"
				# l4_address format can be "protocol/port: (ip, port)" when sending back a existing vnf
				l4_addresses_list = re.findall("('[a-z]*\/\d*')", l4_addresses)
				s= ","
				l4_addresses = s.join(l4_addresses_list)
				# Removing not needed chars
				for ch in ['{','}',' ',"'"]:
					if ch in l4_addresses:
						l4_addresses=l4_addresses.replace(ch,"")
				LOG.debug("l4 adresses: %s", l4_addresses)
				for l4_address in l4_addresses.split(","):
					tmp = l4_address.split("/")
					if tmp[0] != "tcp":
						LOG.error("Only tcp ports are supported on L4 configuration of VNF of type '%s'", vnfType)
						raise ClientError("Only tcp ports are supported on L4 configuration of VNF of type "+ vnfType)
					l4_port = tmp[1]
					uc = UnifyControl(vnf_tcp_port=int(l4_port), host_tcp_port=tcp_port)
					unify_port_mapping[instance.id.get_value() + ":" + port_id + "/" + l4_address] = (unOrchestratorIP, tcp_port)
					unify_control.append(uc)
					tcp_port = tcp_port + 1
				'''
			# just copy the existing l4 addresses
			elif l4_addresses is not None and instance.get_operation() is None:
				l4_addresses_list = re.findall("'[a-z]*\/(\d*)'\s*:\s*\('[0-9.]*', (\d*)\)", l4_addresses)
				for vnf_port, host_port in l4_addresses_list:
					uc = UnifyControl(vnf_tcp_port=int(int(vnf_port)), host_tcp_port=int(host_port))
					unify_control.append(uc)
				'''
			else:
				if int(port.id.get_value()) == 0:
					LOG.error("Port with id = 0 should be present only if it has a L4 configuration on VNF of type '%s'", vnfType)
					raise ClientError("Port with id = 0 should be present only if it has a L4 configuration on VNF of type " + vnfType)
				unify_ip = None
				if port.addresses.l3.length() != 0:
					if port.addresses.l3.length() > 1:
						LOG.error("Only one l3 address is supported on a port on VNF of type '%s'", vnfType)
						raise ClientError("Only one l3 address is supported on a port on VNF of type " + vnfType)
					for l3_address_id in port.addresses.l3:
						l3_address = port.addresses.l3[l3_address_id]
						"""
						if l3_address.configure.get_value() == "False" or l3_address.configure.get_value() == "false":
							LOG.error("Configure must be set to True on l3 address of VNF of type '%s'", vnfType)
							raise ClientError("Configure must be set to True on l3 address of VNF of type " + vnfType)
						"""
						unify_ip = l3_address.requested.get_as_text()

				mac = port.addresses.l2.get_value()
				port_id_nffg = int(port_id)-1
				port_list.append(Port(_id="port:"+str(port_id_nffg), unify_ip=unify_ip, mac=mac))
			if port.control.orchestrator.get_as_text() is not None:
				unify_env_variables.append("CFOR="+port.control.orchestrator.get_as_text())
			if port.metadata.length() > 0:
				LOG.error("Metadata are not supported inside a port element. Those should specified per node")
		if instance.metadata.length() > 0:
			for metadata_id in instance.metadata:
				metadata = instance.metadata[metadata_id]
				key = metadata.key.get_as_text()
				value = metadata.value.get_as_text()
				if key.startswith("variable:"):
					tmp = key.split(":",1)
					unify_env_variables.append(tmp[1]+"="+value)
				elif key.startswith("measure"):
					unify_monitoring = unify_monitoring + " " + value
				else:
					LOG.error("Unsupported metadata " + key)
					raise ClientError("Unsupported metadata " + key)
		if instance.resources.cpu.data is not None or instance.resources.mem.data is not None or instance.resources.storage.data is not None:
			LOG.warning("Resources are not supported inside a node element! Node: "+ instance.id.get_value())


		vnf = VNF(_id = instance.id.get_value(), name = vnfType, ports = port_list, unify_control = unify_control, unify_env_variables = unify_env_variables)
		if nffg.getVNF(vnf.id) is not None:
			LOG.warning("A VNF with id " + vnf.id + " is already deployed.")
			nffg.vnfs.remove(nffg.getVNF(vnf.id))
		nffg.addVNF(vnf)
		LOG.debug("Required VNF: '%s'",vnfType)
		
def processRules(nffg, content):
	'''
	Parses the message and translates the flowrules in the internal JSON representation
	'''
		
	LOG.debug("Extracting the flowrules to be installed in the universal node")

	try:
		tree = ET.parse(constants.GRAPH_XML_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ServerError("ParseError: %s" % e.message)
	
	currentInfrastructure = Virtualizer.parse(root=tree.getroot())
	
	try:
		tree = ET.ElementTree(ET.fromstring(content))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ClientError("ParseError: %s" % e.message)

	infrastructure = Virtualizer.parse(root=tree.getroot())
	try:
		# Added to bind refs to currently deployed VNFs and flowrules
		infrastructure.bind(reference=currentInfrastructure)
	except KeyError as ex:
		LOG.exception(ex)
		LOG.error("Bind operation failed due to inconsistent key. Please check that all IDs in your paths are coherent")
		raise ClientError("Bind operation failed due to inconsistent key. Please check that all IDs in your paths are coherent")

	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable

	endpoint_id = 1

	flowrules = []
	for flowentry in flowtable:
		if operation_type == "netconf-like":
			if flowentry.get_operation() is None:
				LOG.warning("Flowrule {0} has no operation set and will be ignored".format(flowentry.id.get_value()))
				continue
			
			elif flowentry.get_operation() == 'delete':
				#This rule has to be removed from the universal node
				flow_to_be_removed = nffg.getFlowRule(flowentry.id.get_value())
				if flow_to_be_removed is None:
					LOG.error("Trying to delete a Flowrule that is not currently deployed. Flow id: " + flowentry.id.get_value())
					raise ClientError("Trying to delete a Flowrule that is not currently deployed. Flow id: " + flowentry.id.get_value())
				LOG.debug("Flowrule '%s' has to be removed", flow_to_be_removed.id)
				nffg.flow_rules.remove(flow_to_be_removed)				
				continue
	
			elif flowentry.get_operation() != 'create':
				LOG.error("Unsupported operation for flowentry: " + flowentry.id.get_value())
				raise ClientError("Unsupported operation for flowentry: " + flowentry.id.get_value())

	
		flowrule = FlowRule()
		
		f_id = flowentry.id.get_value()
		priority = flowentry.priority.get_value()
		
		#Iterate on the match in order to translate it into the json syntax
		#supported internally by the universal node
		#match = {}
		match = Match() 
		if flowentry.match is not None:
			if type(flowentry.match.get_value()) is str:
				#The tag <match> contains a sequence of matches separated by " " or "," or ";"
				matches = re.split(',| |;', flowentry.match.data)
				for m in matches:
					tokens = m.split("=")
					elements = len(tokens)
					if elements != 2:
						LOG.error("Incorrect match "+flowentry.match.data)
						raise ClientError("Incorrect match")
					#The match is in the form "name=value"
					if not supportedMatch(tokens[0]):
						raise ClientError("Not supported match")

					#We have to convert the virtualizer match into the UN equivalent match
					
					setattr(match, equivalentMatch(tokens[0]), tokens[1])

			#We ignore the element in case it's not a string. It is possible that it is simply empty
					
		#The content of <port> must be added to the match
		#XXX: the following code is quite dirty, but it is a consequence of the nffg library

		portPath = flowentry.port.get_target().get_path()
		port = flowentry.port.get_target()	
		tokens = portPath.split('/');
						
		if len(tokens) is not 6 and len(tokens) is not 8:
			LOG.error("Invalid port '%s' defined in a flowentry (len(tokens) returned %d)",portPath,len(tokens))
			raise ClientError("Invalid port defined in a flowentry")
						
		if tokens[4] == 'ports':
			#This is a port of the universal node. We have to extract the virtualized port name
			if port.name.get_value() not in physicalPortsVirtualization:
				LOG.error("Physical port "+ port.name.get_value()+" is not present in the UN")
				raise ClientError("Physical port "+ port.name.get_value()+" is not present in the UN")
			port_name = physicalPortsVirtualization[port.name.get_value()]
			# Check if we need to create an endpoint or it has been already created
			endpoint = None
			for endp in nffg.end_points:
				if endp.interface == port_name:
					endpoint = endp
					break
			if endpoint is None:
				while nffg.getEndPoint(str(endpoint_id)) is not None:
					endpoint_id += 1
				endpoint = EndPoint(_id = str(endpoint_id) ,_type = "interface", interface = port_name, name = port.name.get_value())
				nffg.addEndPoint(endpoint)				
				endpoint_id += 1
			match.port_in = "endpoint:" + endpoint.id
		elif tokens[4] == 'NF_instances':
			#This is a port of the NF. I have to extract the port ID and the type of the NF.
			#XXX I'm using the port ID as name of the port
			vnf = port.get_parent().get_parent()
			vnf_id = vnf.id.get_value()
			port_id = int(port.id.get_value()) - 1
			match.port_in = "vnf:"+ vnf_id + ":port:" + str(port_id)
			# Check if this VNF port has L4 configuration. In this case rules cannot involve such port 
			if universal_node.NF_instances[vnf_id].ports[port.id.get_value()].addresses.l4.get_value() is not None:
				LOG.error("It is not possibile to install flows related to a VNF port that has L4 configuration. Flowrule id: "+f_id)
				raise ClientError("It is not possibile to install flows related to a VNF port that has L4 configuration")
		else:
			LOG.error("Invalid port '%s' defined in a flowentry",port)
			raise ClientError("Invalid port "+port+" defined in a flowentry")
	
		if flowentry.action is not None:
			if type(flowentry.action.data) is str:
				#The tag <action> contains a sequence of actions separated by " " or "," or ";"
				actions = re.split(',| |;', flowentry.action.data)

				for a in actions:
					action = Action()
					tokens = a.split(":")
					elements = len(tokens)
					if not supportedAction(tokens[0],elements-1):
						raise ClientError("action not supported")
					if elements == 1:
						setattr(action, equivalentAction(tokens[0]), True)
					else:
						setattr(action, equivalentAction(tokens[0]), tokens[1])
					flowrule.actions.append(action)

			# We ignore the element in case it's not a string. It could be simply empty.
							
		#The content of <out> must be added to the action
		#XXX: the following code is quite dirty, but it is a consequence of the nffg library

		portPath = flowentry.out.get_target().get_path()
		port = flowentry.out.get_target()	
		tokens = portPath.split('/');
		if len(tokens) is not 6 and len(tokens) is not 8:
			LOG.error("Invalid port '%s' defined in a flowentry",portPath)
			raise ClientError("Invalid port "+portPath+" defined in a flowentry")
		
		if tokens[4] == 'ports':
			#This is a port of the universal node. We have to extract the ID
			#Then, I have to retrieve the virtualized port name, and from there
			#the real name of the port on the universal node
			port_name = physicalPortsVirtualization[port.name.get_value()]
			# Check if we need to create an endpoint or it has been already created			
			endpoint = None
			for endp in nffg.end_points:
				if endp.interface == port_name:
					endpoint = endp
					break
			if endpoint is None:
				while nffg.getEndPoint(str(endpoint_id)) is not None:
					endpoint_id += 1
				endpoint = EndPoint(_id = str(endpoint_id) ,_type = "interface", interface = port_name, name = port.name.get_value())
				nffg.addEndPoint(endpoint)
				endpoint_id += 1
			flowrule.actions.append(Action(output = "endpoint:" + endpoint.id))
		elif tokens[4] == 'NF_instances':
			#This is a port of the NF. I have to extract the port ID and the type of the NF.
			#XXX I'm using the port ID as name of the port
			vnf = port.get_parent().get_parent()
			vnf_id = vnf.id.get_value()
			port_id = int(port.id.get_value()) - 1
			flowrule.actions.append(Action(output = "vnf:" + vnf_id + ":port:" + str(port_id)))
			
			# Check if this VNF port has L4 configuration. In this case rules cannot involve such port 
			if universal_node.NF_instances[vnf_id].ports[port.id.get_value()].addresses.l4.get_value() is not None:
				LOG.error("It is not possibile to install flows related to a VNF port that has L4 configuration")
				raise ClientError("It is not possibile to install flows related to a VNF port that has L4 configuration")
		else:
			LOG.error("Invalid port '%s' defined in a flowentry",port)
			raise ClientError("Invalid port "+port+" defined in a flowentry")

		#Prepare the rule
		flowrule.id = f_id
		if priority is None:
			LOG.error("Flowrule '%s' must have a priority set", f_id)
			raise ClientError("Flowrule "+f_id+" must have a priority set")
		flowrule.priority = int(priority)
		flowrule.match = match
				
		flowrules.append(flowrule)
		if nffg.getFlowRule(flowrule.id) is not None:
			LOG.warning("A Flowrule with id " + flowrule.id + " is already deployed.")
			nffg.flow_rules.remove(nffg.getFlowRule(flowrule.id))
		nffg.addFlowRule(flowrule)
			
	LOG.debug("Rules extracted:")
	for rule in flowrules:
		LOG.debug(rule.getDict())
	
"""
def getLowerPortId(nf):
	'''
	Scans the ports of a NF retrieving the port with lower id
	'''
	lower_id = maxint
	for port in nf.ports.port:
		port_id = int(port)
		if port_id < lower_id:
			lower_id = port_id
	return lower_id
"""	
		
def supportedMatch(tag):
	'''
	Given an element within match, this function checks whether such an element is supported or node
	'''
	if tag in constants.supported_matches:
		LOG.debug("'%s' is supported!",tag)
		return True
	else:
		LOG.error("'%s' is not a supported match!",tag)
		return False
		
def equivalentMatch(tag):
	'''
	Given an element within match, this function return the element with equivalent meaning in native orchestrator NF-FG
	'''
	return constants.supported_matches[tag]
	
def supportedAction(tag,elements):
	'''
	Given an element within an action, this function checks whether such an element is supported or not
	'''
	if tag in constants.supported_actions:
		LOG.debug("'%s' is supported with %d elements!",tag,constants.supported_actions[tag])
		if constants.supported_actions[tag] == elements:
			return True
		else:
			LOG.debug("The action specifies has a wrong number of elements: %d",elements)
			return False
	else:
		LOG.error("'%s' is not a supported action!",tag)
		return False
		
def equivalentAction(tag):
	'''
	Given an element within action, this function return the element with equivalent meaning in native orchestrator NF-FG
	'''
	return constants.equivalent_actions[tag]

def readGraphFile():
	'''
	Read the graph currently deployed and it returns the nffg object. It is stored in a tmp file, in a json format.
	'''

	try:
		LOG.debug("Reading file: %s",constants.GRAPH_FILE)
		tmpFile = open(constants.GRAPH_FILE,"r")
		json_file = tmpFile.read()
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		raise ServerError("I/O error")
	
	nffg_dict = json.loads(json_file)
	nffg = NF_FG()
	nffg.parseDict(nffg_dict)
	return nffg
	
def writeGraphFile(nffg):
	'''
	Write the whole nffg to the graph file. It is stored in a tmp file, in a json format.
	'''
	
	LOG.debug("Updating the json representation of the graph deployed")
	
	try:
		tmpFile = open(constants.GRAPH_FILE, "w")
		tmpFile.write(json.dumps(nffg.getDict(), indent=4, separators=(',', ': ')))
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		raise ServerError("I/O error")

def updateUniversalNodeConfig(newContent):
	'''
	Read the configuration of the universal node, and applies the required modifications to
	the NF instances and to the flowtable
	'''
	
	LOG.debug("Updating the file containing the configuration of the node...")
	
	LOG.debug("Reading file '%s', which contains the current configuration of the universal node...",constants.GRAPH_XML_FILE)
	try:
		oldTree = ET.parse(constants.GRAPH_XML_FILE)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ServerError("ParseError: %s" % e.message)
	LOG.debug("File correctly read")
		
	infrastructure = Virtualizer.parse(root=oldTree.getroot())
	universal_node = infrastructure.nodes.node[constants.NODE_ID]
	flowtable = universal_node.flowtable
	nfInstances = universal_node.NF_instances
	
	
	#LOG.debug("Getting the new flowrules to be installed on the universal node")
	try:
		newTree = ET.ElementTree(ET.fromstring(newContent))
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		raise ServerError("ParseError: %s" % e.message)
			
	newInfrastructure = Virtualizer.parse(root=newTree.getroot())
	newFlowtable = newInfrastructure.nodes.node[constants.NODE_ID].flowtable
	newNfInstances = newInfrastructure.nodes.node[constants.NODE_ID].NF_instances
			
	#Update the NF instances with the new NFs
	for instance in newNfInstances:
		if instance.get_operation() == 'delete':
			nfInstances[instance.id.get_value()].delete()
		else:
			for port_id in instance.ports.port:
				port = instance.ports[port_id]	
				# Check if there is a request of a l3 address. If this is the case, then provide the response		
				if port.addresses.l3.length() != 0:
					for l3_address_id in port.addresses.l3:
						l3_address = port.addresses.l3[l3_address_id]
						l3_address.provided.set_value(l3_address.requested.get_as_text())
						
				# Check if there is a request of l4 configuration. If this is the case, then provide the response		
				l4_addresses = port.addresses.l4.get_value()
				if l4_addresses is not None:
					l4_response = {}
					# unify_port_mapping is in the form NF_id:port_id/protocol/port
					for k,v in unify_port_mapping.iteritems():
						tmp1 = k.split("/", 1)
						tmp2 = tmp1[0].split(":")
						if instance.id.get_value() == tmp2[0] and port_id == tmp2[1]:
							l4_response[tmp1[1]] = v
					port.addresses.l4.set_value(l4_response)
			instance.set_operation(None)
			nfInstances.add(instance)
	
	#Update the flowtable with the new flowentries
	for flowentry in newFlowtable:
		if flowentry.get_operation() == 'delete':
			flowtable[flowentry.id.get_value()].delete()
		else:
			flowentry.set_operation(None)
			flowtable.add(flowentry)
	#It is not necessary to remove conflicts, since they are already handled by the library,
	#i.e., it does not insert two identical rules
	
	try:
		tmpFile = open(constants.GRAPH_XML_FILE, "w")
		tmpFile.write(infrastructure.xml())
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		raise ServerError("I/O error")
		
	return infrastructure.xml()

'''
	Methods used to interact with the universal node orchestrator
'''
def sendToUniversalNode(nffg):
	'''
	Deploys rules and VNFs on the universal node
	'''
	LOG.info("Sending the new configuration to the universal node orchestrator (%s)",unOrchestratorURL)

	nffg.id = graph_id
	nffg.name = graph_name
	if unify_monitoring == "":
		nffg.unify_monitoring = None
	else:
		nffg.unify_monitoring = unify_monitoring
	
	#Delete endpoints that are not involved in any flowrule
	for endpoint in nffg.end_points[:]:
		if not nffg.getFlowRulesSendingTrafficToEndPoint(endpoint.id) and not nffg.getFlowRulesSendingTrafficFromEndPoint(endpoint.id):
			nffg.end_points.remove(endpoint)
			
	graph_url = unOrchestratorURL + "/NF-FG/%s"
	
	try:
		if len(nffg.flow_rules) + len(nffg.vnfs) + len(nffg.end_points) == 0:
			LOG.debug("No elements have to be sent to the universal node orchestrator...sending a delete request")
			LOG.debug("DELETE url: "+ graph_url % (nffg.id))
			if debug_mode is False:
				if authentication is True and token is None:
					getToken()
				responseFromUN = requests.delete(graph_url % (nffg.id), headers=headers)
				LOG.debug("Status code received from the universal node orchestrator: %s",responseFromUN.status_code)
				# TODO: check the correct code
				if responseFromUN.status_code == 201: 
					LOG.info("Graph successfully deleted")
				elif responseFromUN.status_code == 401:
					LOG.debug("Token expired, getting a new one...")
					getToken()
					newresponseFromUN = requests.delete(graph_url % (nffg.id), headers=headers)
					LOG.debug("Status code received from the universal node orchestrator: %s",newresponseFromUN.status_code)
					if newresponseFromUN.status_code == 201: 
						LOG.info("Graph successfully deleted")
					else:
						LOG.error("Something went wrong while deleting the graph on the universal node")	
						raise ServerError("Something went wrong while deleting the graph on the universal node")						
				else:
					LOG.error("Something went wrong while deleting the graph on the universal node")	
					raise ServerError("Something went wrong while deleting the graph on the universal node")
		else:
			LOG.debug("Graph that is going to be sent to the universal node orchestrator:")
			LOG.debug("%s",nffg.getJSON())
			LOG.debug("PUT url: "+ graph_url % (nffg.id))
			
			if debug_mode is False:
				if authentication is True and token is None:
					getToken()	
				responseFromUN = requests.put(graph_url % (nffg.id), data=nffg.getJSON(), headers=headers)
				LOG.debug("Status code received from the universal node orchestrator: %s",responseFromUN.status_code)
			
				if responseFromUN.status_code == 201:
					LOG.info("New VNFs and flows properly deployed on the universal node")
				elif responseFromUN.status_code == 401:
					LOG.debug("Token expired, getting a new one...")
					getToken()
					newresponseFromUN = requests.put(graph_url % (nffg.id), data=nffg.getJSON(), headers=headers)
					LOG.debug("Status code received from the universal node orchestrator: %s",newresponseFromUN.status_code)
					if newresponseFromUN.status_code == 201: 
						LOG.info("New VNFs and flows properly deployed on the universal node")
					else:
						LOG.error("Something went wrong while deploying the new VNFs and flows on the universal node")	
						raise ServerError("Something went wrong while deploying the new VNFs and flows on the universal node")
				else:
					LOG.error("Something went wrong while deploying the new VNFs and flows on the universal node")	
					raise ServerError("Something went wrong while deploying the new VNFs and flows on the universal node")
				
	except (requests.ConnectionError):
		LOG.error("Cannot contact the universal node orchestrator at '%s'",graph_url % (nffg.id))
		raise ServerError("Cannot contact the universal node orchestrator at "+graph_url)

def getToken():
	'''
	If the authentication is enabled in the configuration file, this function interacts with the UN orchestrator in order to get a valid token.
	'''
	global token, headers
	headers = {'Accept': 'application/json', 'Content-Type': 'application/json'}
	authenticationData = {'username': username, 'password': password}
	authentication_url = unOrchestratorURL + "/login"	
	resp = requests.post(authentication_url, data=json.dumps(authenticationData), headers=headers)
	try:
		resp.raise_for_status()
		LOG.debug("Authentication successfully performed")
		token = resp.text
		headers = {'Content-Type': 'application/json', 'X-Auth-Token': token}
	except HTTPError as err:
		LOG.error(err)
		raise ServerError("login failed: " + str(err))
	
'''
	Methods used in the initialization phase of the virtualizer
'''

def virtualizerInit():
	'''
	The virtualizer maintains the state of the node in a tmp file.
	This function initializes such a file.
	'''
		
	LOG.info("Initializing the virtualizer...")
	
	if not readConfigurationFile():
		return False

	v = Virtualizer(id=constants.INFRASTRUCTURE_ID, name=constants.INFRASTRUCTURE_NAME)				
	v.nodes.add(
		Infra_node(
			id=constants.NODE_ID,
			name=constants.NODE_NAME,
			type=constants.NODE_TYPE,
			resources=Software_resource(
				cpu=cpu,
				mem=memory,
				storage=storage
			)
		)
	)
	
	#Read information related to the infrastructure and add it to the
	#virtualizer representation
	LOG.debug("Reading file '%s'...",infrastructureFile)
	try:
		tree = ET.parse(infrastructureFile)
	except ET.ParseError as e:
		print('ParseError: %s' % e.message)
		return False
	root = tree.getroot()
	
	universal_node = v.nodes.node[constants.NODE_ID]
	
	#Read information related to the physical ports and add it to the
	#virtualizer representation
	
	#global physicalPortsVirtualization

	ports = root.find('ports')
	portID = 1
	for port in ports:
		virtualized = port.find('virtualized')
		port_description = virtualized.attrib
		LOG.debug("physicl name: %s - virtualized name: %s - type: %s - sap: %s", port.attrib['name'], port_description['as'],port_description['port-type'],port_description['sap'])
		physicalPortsVirtualization[port_description['as']] =  port.attrib['name']

		portObject = Virt_Port(id=str(portID), name=port_description['as'], port_type=port_description['port-type'], sap=port_description['sap'])
		universal_node.ports.add(portObject)	
		portID = portID + 1
	
	#Save the virtualizer representation on a file
	try:
		tmpFile = open(constants.GRAPH_XML_FILE, "w")
		tmpFile.write(v.xml())
		tmpFile.close()
	except IOError as e:
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return False
	
	if not contactNameResolver():
		return False

	if operation_type == "netconf-like":
		#Initizialize the file describing the deployed graph as a json with an empty graph 
		writeGraphFile(NF_FG(_id=graph_id))

	LOG.info("The virtualizer has been initialized")
	return True

def readConfigurationFile():
	'''
	Read the configuration file of the virtualizer
	'''
	
	global nameResolverURL
	global unOrchestratorIP
	global unOrchestratorURL
	global infrastructureFile
	global cpu, memory, storage
	global authentication, username, password
	global operation_type
	
	LOG.info("Reading configuration file: '%s'",constants.CONFIGURATION_FILE)
	config = ConfigParser.ConfigParser()
	config.read(constants.CONFIGURATION_FILE)
	sections = config.sections()
	
	if 'connections' not in sections:
		LOG.error("Wrong file '%s'. It does not include the section 'connections' :(",constants.CONFIGURATION_FILE)
		return False
	try:
		nameResolverURL = nameResolverURL + config.get("connections","NameResolverAddress") + ":" + config.get("connections","NameResolverPort")
	except:
		LOG.error("Option 'NameResolverAddress' or option 'NameResolverPort' not found in section 'connections' of file '%s'",constants.CONFIGURATION_FILE)
		return False
	try:
		unOrchestratorIP = config.get("connections","UNOrchestratorAddress")
		unOrchestratorURL = unOrchestratorURL + unOrchestratorIP + ":" + config.get("connections","UNOrchestratorPort")
	except:
		LOG.error("Option 'UNOrchestratorAddress' or option 'UNOrchestratorPort' not found in section 'connections' of file '%s'",constants.CONFIGURATION_FILE)
		return False
	
	if 'un-orchestrator authentication' not in sections:
		LOG.error("Wrong file '%s'. It does not include the section 'un-orchestrator authentication' :(",constants.CONFIGURATION_FILE)
		return False
	try:
		authentication = config.getboolean("un-orchestrator authentication","authentication")
	except:
		LOG.error("Option 'authentication' not found in section 'un-orchestrator authentication' of file '%s'",constants.CONFIGURATION_FILE)
		return False
	if authentication is True:
		try:
			username = config.get("un-orchestrator authentication","username")
			password = config.get("un-orchestrator authentication","password")	
		except:
			LOG.error("Option 'username' or 'password' not found in section 'un-orchestrator authentication' of file '%s'",constants.CONFIGURATION_FILE)
			return False
	
	if 'resources' not in sections:
		LOG.error("Wrong file '%s'. It does not include the section 'resources' :(",constants.CONFIGURATION_FILE)
		return False
	try:
		cpu = config.get("resources","cpu")
		memory = config.get("resources","memory")
		storage = config.get("resources","storage")
	except:
		LOG.error("Option 'cpu' or 'memory' or 'storage' not found in section 'resources' of file '%s'",constants.CONFIGURATION_FILE)
		return False
	
	if 'configuration' not in sections:
		LOG.error("Wrong file '%s'. It does not include the section 'configuration' :(",constants.CONFIGURATION_FILE)
		return False
	try:
		infrastructureFile = config.get("configuration","PortFile")
	except:
		LOG.error("Option 'PortFile' not found in section 'configuration' of file '%s'",constants.CONFIGURATION_FILE)
		return False
	try:
		operation_type = config.get("configuration","operation-type")
	except:
		LOG.error("Option 'operation-type' not found in section 'configuration' of file '%s'",constants.CONFIGURATION_FILE)
		return False
	if operation_type != "full-content" and operation_type != "netconf-like":
		LOG.error("Option 'operation-type' must be set to 'full-content' or to 'netconf-like'. Current value is '%s'",operation_type)
		return False
	try:
		LogLevel = config.get("configuration","LogLevel")	
		if LogLevel == 'debug':
			LOG.setLevel(logging.DEBUG)
			LOG.addHandler(sh)
			LOG.debug("Log level set to 'debug'")
		if LogLevel == 'info':
			LOG.setLevel(logging.INFO)
			LOG.info("Log level set to 'info'")
		if LogLevel == 'warning':
			LOG.setLevel(logging.WARNING)
			LOG.warning("Log level set to 'warning'")
		if LogLevel == 'error':
			LOG.setLevel(logging.ERROR)
			LOG.error("Log level set to 'error'")
		if LogLevel == 'critical':
			LOG.setLevel(logging.CRITICAL)
			LOG.critical("Log level set to 'critical'")
	except:
		LOG.warning("Option 'LogLevel' not found in section 'configuration' of file '%s'",constants.CONFIGURATION_FILE)
		LOG.warning("Log level is set on 'INFO'")
		
	LOG.debug("CPU: %s", cpu)
	LOG.debug("memory: %s", memory)
	LOG.debug("storage: %s", storage)
	
	LOG.info("Url used to contact the name-resolver: %s",nameResolverURL)
	LOG.info("Url used to contact the universal node orchestrator: %s",unOrchestratorURL)
	LOG.info("The virtualizer is operating in %s mode", operation_type)
	
	return True

def contactNameResolver():
	'''
	Contact the name resolver is order to know the VNFs available
	'''
	global base_xml
	
	LOG.info("Starting interaction with the name-resolver (%s)",nameResolverURL)
	
	url = nameResolverURL + "/nfs/digest"
	try:
		response = requests.get(url)
	except (requests.ConnectionError) as e:
		LOG.error("Cannot contact the name-resolver at %s",url)
		return False

	LOG.debug("Answer from the name resolver, in plain text %s",response.text)

	data = response.json()
	
	LOG.debug("Data received from the name-resolver")
	LOG.debug("%s",json.dumps(data, indent = 4))
	
	json_object = data
	
	if 'network-functions' not in json_object.keys():
		LOG.error("Wrong response received from the 'name-resolver'")
		return False
	
	sequence_number = 1
	for vnf_name in json_object['network-functions']:
		if 'name' not in vnf_name:
			LOG.error("Wrong response received from the 'name-resolver'")
			return False
		LOG.debug("Considering VNF: '%s'",vnf_name['name'])
		
		url = nameResolverURL + '/nfs/' + vnf_name['name']
		try:
			response = requests.get(url)
		except (requests.ConnectionError) as e:
			LOG.error("Cannot contact the name-resolver at %s",url)
			return False
	
		vnf_description = response.json()
	
		LOG.debug("Data received from the name-resolver")
		LOG.debug("%s",json.dumps(vnf_description, indent = 4))
		
		if 'name' not in vnf_description:
			LOG.error("Wrong response received from the 'name-resolver'")
			return False

		if 'nports' not in vnf_description:
			LOG.error("Wrong response received from the 'name-resolver'")
			return False
		
		if 'summary' not in vnf_description:
			LOG.error("Wrong response received from the 'name-resolver'")
			return False
		
		ID = 'NF'+str(sequence_number)
		name = vnf_description['summary']
		vnftype = vnf_description['name']
		numports = vnf_description['nports']
		
		try:
			tree = ET.parse(constants.GRAPH_XML_FILE)
		except ET.ParseError as e:
			print('ParseError: %s' % e.message)
			return False
	
		LOG.debug("Inserting VNF %s, ID %s, type %s, num ports %d...",ID,name,vnftype,numports)
	
		infrastructure = Virtualizer.parse(root=tree.getroot())
		universal_node = infrastructure.nodes.node[constants.NODE_ID]
		capabilities = universal_node.capabilities
		supportedNF = capabilities.supported_NFs
	
		vnf = Infra_node(id=ID,name=name,type=vnftype)
	
		i = 1
		for x in range(0, numports):
			port = Virt_Port(id=str(i), name='VNF port ' + str(i), port_type='port-abstract')
			vnf.ports.add(port)
			i = i+1
	
		supportedNF.add(vnf)
	
		try:
			tmpFile = open(constants.GRAPH_XML_FILE, "w")
			tmpFile.write(infrastructure.xml())
			tmpFile.close()
		except IOError as e:
			print "I/O error({0}): {1}".format(e.errno, e.strerror)
			return False

		sequence_number = sequence_number + 1
		LOG.debug("VNF '%s' considered",vnf_name['name'])
	# This is the base status after the virtualizer init. It is used to reset hte status after each interation only in full-content mode.
	base_xml = infrastructure.xml()
	
	LOG.info("Interaction with the name-resolver terminated")
	return True

'''
	The following code is executed by guicorn at the boot of the virtualizer
'''
	
api = falcon.API()

#Set the logger
LOG = logging.getLogger(__name__)
LOG.setLevel(logging.DEBUG)
LOG.propagate = False
sh = logging.StreamHandler()
f = logging.Formatter('[%(asctime)s][Virtualizer][%(levelname)s] %(message)s')
sh.setFormatter(f)
LOG.addHandler(sh)

#Global variables
unOrchestratorURL = "http://"
nameResolverURL = "http://"
infrastructureFile = ""
physicalPortsVirtualization = {}
graph_id = "1"
graph_name = "NF-FG"
tcp_port = 10000
unify_port_mapping = OrderedDict()
unify_monitoring = ""
cpu = ""
memory = ""
storage = ""
authentication = False
username = ""
password = ""
token = None
headers = {'Content-Type': 'application/json'}

# if debug_mode is True no interactions will be made with the UN
debug_mode = False

if not virtualizerInit():
	LOG.error("Failed to start up the virtualizer.")
	LOG.error("Please, press 'ctrl+c' and restart the virtualizer.")

api.add_route('/',DoUsage())
api.add_route('/ping',DoPing())
api.add_route('/get-config',DoGetConfig())
api.add_route('/edit-config',DoEditConfig())

#in_file = open ("config/nffg_examples/passthrough_with_vnf_nffg_v5.xml")
#in_file = open ("config/nffg_examples/simple_passthrough_nffg.xml")
#in_file = open ("config/nffg_examples/nffg_delete_flow_vnf.xml")
#in_file = open ("config/nffg_examples/er_nffg_virtualizer5.xml")
#in_file = open ("config/nffg_examples/step1.xml")
#in_file = open ("config/nffg_examples/a.xml")
#DoEditConfig().on_post(in_file.read(), None)
#in_file = open ("config/nffg_examples/b.xml")
#DoEditConfig().on_post(in_file.read(), None)
