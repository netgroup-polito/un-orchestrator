#include "graph_parser.h"
#include "../high_level_graph/high_level_graph_vnf.h"

static const char LOG_MODULE_NAME[] = "Graph-Parser";

void GraphParser::parseGraph(Value value, highlevel::Graph &graph, bool newGraph, GraphManager *gm)
{
	//for each endpoint (interface), contains the id
	map<string, string> iface_id;
	//for each endpoint (internal), contains the internal-group id
	map<string, string > internal_id;
	//for each endpoint (gre), contains the gre informations (local-ip, remote-ip ..)
	map<string, gre_info_t> gre_id;
	//for each endpoint (vlan), contains the pair vlan id, interface
	map<string, pair<string, string> > vlan_id;
	//for each hoststack, contains its informations
	map<string, hoststack_info_t> hoststack_id;
#ifdef ENABLE_NODE_CONFIG
	//contains the id of default gateway node configuration
	string nodeConfigDG_id;
#endif
	//The following two data structures are used for managing trusted/untrusted VNF ports
	//for each VNF id, contains the pair port id, trusted/untrusted
	map<string, map<string, bool> > trusted_ports;
	//for each VNF id, contains the pair port id, mac address
	map<string, map<string, string> > trusted_ports_mac_addresses;

	/**
	*	The graph is defined according to this schema:
	*		https://github.com/netgroup-polito/nffg-library/blob/master/schema.json
	*/
	try
	{
		Object obj = value.getObject();
		vector<Object> gre_array(256);
		Object big_switch, ep_gre;
		bool foundFlowGraph = false;
		bool foundBigSwitch = false;

		//Iterates on the json received
		for(Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
		{
			const string& name  = i->first;
			const Value&  value = i->second;

			//Identify the forwarding graph
			if(name == FORWARDING_GRAPH)
			{
				foundFlowGraph = true;

				vector<string> id_gre (256);

				Object forwarding_graph;
				try
				{
					forwarding_graph = value.getObject();
				} catch(std::exception& e)
				{
					string error = string("The content does not respect the JSON syntax: ") + FORWARDING_GRAPH + " should be an Object";
					ULOG_WARN(error.c_str());
					throw new GraphParserException(std::move(error));
				}
				for(Object::const_iterator fg = forwarding_graph.begin(); fg != forwarding_graph.end(); fg++)
				{
					bool e_if = false, e_vlan = false, e_internal = false, e_gre=false, e_hs=false;
#ifdef ENABLE_NODE_CONFIG
                    bool nc_dg = false;
                    string dgIP;
#endif
					string id, v_id, node, iface, e_name, ep_id, interface, in_group;

					const string& fg_name  = fg->first;
					const Value&  fg_value = fg->second;

					if(fg_name == _NAME)
					{
						ULOG_DBG("\"%s\"->\"%s\": \"%s\"",FORWARDING_GRAPH,_NAME,fg_value.getString().c_str());

						//set name of the graph
						graph.setName(fg_value.getString());
					}
					else if(fg_name == F_DESCR)
					{
						ULOG_DBG("\"%s\"->\"%s\": \"%s\"",FORWARDING_GRAPH,F_DESCR,fg_value.getString().c_str());

						//XXX: currently, this information is ignored
					}
#ifdef ENABLE_NODE_CONFIG
					//Identify the node configurations
					else if(fg_name == NODE_CONFIG)
					{
						try{
							try
							{
								fg_value.getArray();
							} catch(std::exception& e)
							{
								string error = string("The content does not respect the JSON syntax: ") + NODE_CONFIG + " should be an Array";
								ULOG_WARN(error.c_str());
								throw new GraphParserException(std::move(error));
							}

							/**
							* A node configuration can contain:
							*	- id
							*	- type
							*		- dafault-gateway
							*
						    *	- Other information that depend on the type
							*/
							const Array& node_configurations_array = fg_value.getArray();

							ULOG_DBG("\"%s\"",NODE_CONFIG);

							//Iterate on the node-configurations
							set<string> usedNodeConfigurationsID;
							for( unsigned int nc = 0; nc < node_configurations_array.size(); ++nc )
							{
								try{
									node_configurations_array[nc].getObject();
								} catch(std::exception& e)
								{
									string error = string("The content does not respect the JSON syntax: element of ") + NODE_CONFIG + " should be an Object";
									ULOG_WARN(error.c_str());
									throw new GraphParserException(std::move(error));
								}

								//This is a node configuration, with a type, and a default gateway
								Object node_configuration = node_configurations_array[nc].getObject();
								//Iterate on the elements of a node configuration
								for(Object::const_iterator nodeConfig = node_configuration.begin(); nodeConfig != node_configuration.end(); nodeConfig++)
								{
									const string& nc_name  = nodeConfig->first;
									const Value&  nc_value = nodeConfig->second;

									if(nc_name == _ID)
									{
										id = nc_value.getString();
										//Two node configurations cannot have the same ID
										if(usedNodeConfigurationsID.count(id) != 0)
										{
											string error = string("Found two node configurations with the same ID: ") + id.c_str() + ". This is not valid.";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}
										usedNodeConfigurationsID.insert(id);
									}
									else if(nc_name == NC_TYPE)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",NODE_CONFIG,NC_TYPE,nc_value.getString().c_str());
										string type = nc_value.getString();
									}
									//identify default gateway node configuration
									else if(nc_name == NC_DG)
									{
										try
										{
											nc_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + NC_DG + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										Object nc_defaultGateway = nc_value.getObject();
										if(!nc_dg) {
											nc_dg = true;

											for (Object::const_iterator ncDG = nc_defaultGateway.begin();
												 ncDG != nc_defaultGateway.end(); ncDG++) {
												const string &ncDG_name = ncDG->first;
												const Value &ncDG_value = ncDG->second;

												if (ncDG_name == NC_DG_IP)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%s\"", NC_DG, NC_DG_IP,
															 ncDG_value.getString().c_str());
													dgIP = ncDG_value.getString();
													nodeConfigDG_id = ncDG_value.getString();
													ULOG_DBG("\"%s\"->\"%s\"", id.c_str(), nodeConfigDG_id.c_str());
												} else {
													string error =string("Invalid key ") + ncDG_name.c_str() + " inside " + NC_DG_IP;
													ULOG_WARN(error.c_str());
													throw new GraphParserException(std::move(error));
												}
											}
										} else
										{
											string error = string("More than one ") + nc_name.c_str() + " key present in an Object of " + NODE_CONFIG;
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}
									}
									else
									{
										string error = string("Invalid key ") + nc_name.c_str() + " in an Object of " + NODE_CONFIG;
										ULOG_WARN(error.c_str());
										throw new GraphParserException(std::move(error));
									}
								}//End of iteration on the elements of a node configuration

								//add default gateway node configuration
								if(nc_dg)
								{
									highlevel::NodeConfigDefaultGateway nodeConfigDG(id, dgIP);
									graph.addNodeConfigDefaultGateway(nodeConfigDG);
								}
							}//End iteration on the node configurations
						} catch(std::exception& e)
						{
							string error = string("The ") + NODE_CONFIG + " element does not respect the JSON syntax: " + e.what();
							ULOG_WARN(error.c_str());
							throw new GraphParserException(error);
						}
					}//End if(fg_name == NODE_CONFIG)
#endif
					//Identify the VNFs
					else if(fg_name == VNFS)
					{
						try
						{
							try
							{
								fg_value.getArray();
							} catch(std::exception& e)
							{
								string error = string("The content does not respect the JSON syntax: ") + VNFS + " should be an Array";
								ULOG_WARN(error.c_str());
								throw new GraphParserException(std::move(error));
							}

							const Array& vnfs_array = fg_value.getArray();

							//XXX We may have no VNFs in the following cases:
							//*	graph with only physical ports
							//*	update of a graph that only adds new flows
							//However, when there are no VNFs, we provide a warning

							//Itearate on the VNFs
							set<string> usedVNFID;
							for( unsigned int vnf = 0; vnf < vnfs_array.size(); ++vnf )
							{
								try
								{
									vnfs_array[vnf].getObject();
								} catch(std::exception& e)
								{
									string error = string("The content does not respect the JSON syntax: element of ") + VNFS + " should be an Object";
									ULOG_WARN(error.c_str());
									throw new GraphParserException(std::move(error));
								}

								/**
								*	According to https://github.com/netgroup-polito/nffg-library/blob/master/schema.json , a VNF can contain:
								*		- id
								*		- name	(mandatory)
								*		- vnf_template
								 *		- functional-capability
								*		- domain
								*		- ports
								*			- id
								*			- name
								*			- mac
								*			- unify-ip
								*		- unify-control
								*		- groups
								*/

								Object network_function = vnfs_array[vnf].getObject();

								bool foundName = false;
								bool foundFunctionalCapability = false;

								string id, name, vnf_template, functional_capability, port_id, port_name;
								list<string> groups;
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
								int vnf_tcp_port = 0, host_tcp_port = 0;
								//list of pair element "host TCP port" and "VNF TCP port" related by the VNF
								list<port_mapping_t> controlPorts;
								//list of environment variables in the form "variable=value"
								list<string> environmentVariables;
#endif
								//list of ports of the VNF
								list<highlevel::vnf_port_t> portS;
								// coordinates of the vnf (to be displayed by the GUI)
								highlevel::Position *vnfPosition=NULL;

								//Parse the network function
								for(Object::const_iterator nf = network_function.begin(); nf != network_function.end(); nf++)
								{
									const string& nf_name  = nf->first;
									const Value&  nf_value = nf->second;

									if(nf_name == _NAME)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNFS,_NAME,nf_value.getString().c_str());
										foundName = true;
										name = nf_value.getString();
									}
									else if(nf_name == VNF_TEMPLATE)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNFS,VNF_TEMPLATE,nf_value.getString().c_str());
										vnf_template = nf_value.getString();
									}
									else if(nf_name == FUNCTIONAL_CAPABILITY)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNFS,FUNCTIONAL_CAPABILITY,nf_value.getString().c_str());
										foundFunctionalCapability = true;
										functional_capability = nf_value.getString();
									}
									else if(nf_name == _ID)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNFS,_ID,nf_value.getString().c_str());

										if(usedVNFID.count(nf_value.getString()) != 0)
										{
											string error = string("Found two VNFs with the same ID: ") + (nf_value.getString()).c_str() + ". This is not valid.";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}
										usedVNFID.insert(nf_value.getString());
										//store value of VNF id
										id.assign(nf_value.getString().c_str());
									}
									else if(nf_name == UNIFY_CONTROL)
									{
#ifndef ENABLE_UNIFY_PORTS_CONFIGURATION
										ULOG_WARN("Key \"%s\" is ignored in this configuration of the %s!",UNIFY_CONTROL,MODULE_NAME);
										continue;
#else
										try{
											nf_value.getArray();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + UNIFY_CONTROL + " should be an Array";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										const Array& control_array = nf_value.getArray();

										//Itearate on the control ports
										for( unsigned int ctrl = 0; ctrl < control_array.size(); ++ctrl )
										{
											try{
												control_array[ctrl].getObject();
											} catch(std::exception& e)
											{
												string error = string("The content does not respect the JSON syntax: element of ") + UNIFY_CONTROL + " should be an Object";
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}

											//This is a VNF control port, with an host TCP port and a vnf VNF port
											Object control = control_array[ctrl].getObject();

											port_mapping_t port_mapping;

											vector<string> port_descr(4);

											//Parse the control port
											for(Object::const_iterator c = control.begin(); c != control.end(); c++)
											{
												const string& c_name  = c->first;
												const Value&  c_value = c->second;

												if(c_name == HOST_PORT)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%d\"",UNIFY_CONTROL,HOST_PORT,c_value.getInt());

													host_tcp_port = c_value.getInt();
												}
												else if(c_name == VNF_PORT)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%d\"",UNIFY_CONTROL,VNF_PORT,c_value.getInt());

													vnf_tcp_port = c_value.getInt();
												}
											}

											stringstream ss, sss;
											ss << host_tcp_port;

											sss << vnf_tcp_port;

											port_mapping_t control_port;
											control_port.host_port = ss.str();
											control_port.guest_port = sss.str();
											controlPorts.push_back(control_port);
										}//end iteration on the control ports
#endif
									}
									else if(nf_name == UNIFY_ENV_VARIABLES)
									{
#ifndef ENABLE_UNIFY_PORTS_CONFIGURATION
										ULOG_WARN("Key \"%s\" is ignored in this configuration of the %s!",UNIFY_ENV_VARIABLES,MODULE_NAME);
										continue;
#else

										try{
											nf_value.getArray();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + UNIFY_ENV_VARIABLES + " should be an Array";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										const Array& env_variables_array = nf_value.getArray();

										//Itearate on the environment variables
										for(unsigned int env_var = 0; env_var < env_variables_array.size(); ++env_var)
										{
											try{
												env_variables_array[env_var].getObject();
											} catch(std::exception& e)
											{
												string error = string("The content does not respect the JSON syntax: element of ") + UNIFY_ENV_VARIABLES + " should be an Object";
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}

											//This is an envirnment variable
											Object env_variable = env_variables_array[env_var].getObject();

											stringstream theEnvVar;

											//Parse the environment variable
											for(Object::const_iterator ev = env_variable.begin(); ev != env_variable.end(); ev++)
											{
												const string& ev_name  = ev->first;
												const Value&  ev_value = ev->second;

												if(ev_name == VARIABLE)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%s\"",UNIFY_ENV_VARIABLES,VARIABLE,(ev_value.getString()).c_str());
													theEnvVar << ev_value.getString();
												}
												else
												{
													string error = string("Invalid key ") + ev_name.c_str() + " in an element of " + UNIFY_ENV_VARIABLES;
													ULOG_WARN(error.c_str());
													throw new GraphParserException(std::move(error));
												}
											}
											environmentVariables.push_back(theEnvVar.str());
										}//end iteration on the environment variables

#endif
									}
									else if(nf_name == VNF_PORTS)
									{
										try{
											nf_value.getArray();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + VNF_PORTS + " should be an Array";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										const Array& ports_array = nf_value.getArray();

										//Itearate on the ports
										for( unsigned int ports = 0; ports < ports_array.size(); ++ports )
										{
											try{
												ports_array[ports].getObject();
											} catch(std::exception& e)
											{
												string error = string("The content does not respect the JSON syntax: element of ") + VNF_PORTS + " should be an Object";
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}

											//This is a VNF port, with an ID and a name
											Object port = ports_array[ports].getObject();

											highlevel::vnf_port_t port_descr;
											port_descr.configuration.trusted = false; //by default the port is not trusted
											
											//Parse the port
											for(Object::const_iterator p = port.begin(); p != port.end(); p++)
											{
												const string& p_name  = p->first;
												const Value&  p_value = p->second;

												if(p_name == _ID)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNF_PORTS,_ID,p_value.getString().c_str());

													port_id = p_value.getString();

													port_descr.id = port_id;
												}
												else if(p_name == _NAME)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNF_PORTS,_NAME,p_value.getString().c_str());

													port_name = p_value.getString();

													port_descr.name = port_name;
												}
												else if(p_name == PORT_MAC)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNF_PORTS,PORT_MAC,p_value.getString().c_str());
													if(!AddressValidator::validateMac(p_value.getString().c_str()))
													{
														string error = string("Key ") + PORT_MAC + " with wrong value: " + p_value.getString().c_str() + ". Please specify a correct MAC address.";
														ULOG_WARN(error.c_str());
														throw new GraphParserException(std::move(error));
													}
													if(!AddressValidator::isUnicastMac(p_value.getString().c_str()))
													{
														string error = string("Key ") + PORT_MAC + " with wrong value: " + p_value.getString().c_str() + ". Multicast address cannot be assigned to VNF ports.";
														ULOG_WARN(error.c_str());
														throw new GraphParserException(std::move(error));
													}
													port_descr.configuration.mac_address = p_value.getString();
												}
												else if(p_name == PORT_IP)
												{
#ifndef ENABLE_UNIFY_PORTS_CONFIGURATION
													ULOG_WARN("Key \"%s\" is ignored in this configuration of the %s!",PORT_IP,MODULE_NAME);
													continue;
#else
													ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNF_PORTS,PORT_IP,p_value.getString().c_str());
													port_descr.configuration.ip_address = p_value.getString();
#endif
												}
												else if(p_name == PORT_TRUSTED)
												{
													ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNF_PORTS,PORT_MAC,(p_value.getBool())? "true" : "false");
													port_descr.configuration.trusted = p_value.getBool();
												}
												else if(p_name == POSITION)
												{

													try
													{
														p_value.getObject();
													} catch(std::exception& e)
													{
														string error = string("The content does not respect the JSON syntax: ") + POSITION + " element should be an Object";
														ULOG_WARN(error.c_str());
														throw new GraphParserException(std::move(error));
													}
													Object position_Object = p_value.getObject();
													highlevel::Position *portPosition = new highlevel::Position();
													//Parse the position object
													for(Object::const_iterator pos = position_Object.begin(); pos != position_Object.end(); pos++) {
														const string &pos_name = pos->first;
														const Value &pos_value = pos->second;

														if (pos_name == X_POSITION) {
															ULOG_DBG("\"%s\"->\"%s\": \"%d\"", VNFS, X_POSITION, pos_value.getInt());
															portPosition->setX(pos_value.getInt());
														}
														else if (pos_name == Y_POSITION) {
															ULOG_DBG("\"%s\"->\"%s\": \"%d\"", VNFS, Y_POSITION, pos_value.getInt());
															portPosition->setY(pos_value.getInt());
														}
														else
														{
															string error = string("Invalid key ") + pos_name.c_str() + " in an Object: " + POSITION;
															ULOG_WARN(error.c_str());
															throw new GraphParserException(std::move(error));
														}
													}
													port_descr.position=portPosition;
												}
												else
												{
													string error = string("Invalid key ") + p_name.c_str() + " in a PORT of " + VNF_PORTS;
													ULOG_WARN(error.c_str());
													throw new GraphParserException(std::move(error));
												}
											}

											if(port_descr.configuration.trusted && port_descr.configuration.mac_address == "")
											{
												string error = "A 'trusted' VNF port must be associated with a MAC address";
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}

											//Each VNF port has its own configuration if provided
											portS.push_back(port_descr);
										}
									}
									else if(nf_name == VNF_GROUPS)
									{

										try
										{
											nf_value.getArray();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + VNF_GROUPS + " should be an Array";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}
										const Array& myGroups_Array = nf_value.getArray();
										for(unsigned int i = 0; i<myGroups_Array.size();i++)
										{
											ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VNFS,VNF_GROUPS,myGroups_Array[i].getString().c_str());
											string group = myGroups_Array[i].getString();
											groups.push_back(group);
										}
										ULOG_WARN("Key \"%s\" found. It is ignored in the current implementation of the %s",VNF_GROUPS,MODULE_NAME);
									}
									else if(nf_name == POSITION)
									{

										try
										{
											nf_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + POSITION + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}
										Object position_Object = nf_value.getObject();
										vnfPosition = new highlevel::Position();
										//Parse the position object
										for(Object::const_iterator pos = position_Object.begin(); pos != position_Object.end(); pos++) {
											const string &pos_name = pos->first;
											const Value &pos_value = pos->second;

											if (pos_name == X_POSITION) {
												ULOG_DBG("\"%s\"->\"%s\": \"%d\"", VNFS, X_POSITION, pos_value.getInt());
												vnfPosition->setX(pos_value.getInt());
											}
											else if (pos_name == Y_POSITION) {
												ULOG_DBG("\"%s\"->\"%s\": \"%d\"", VNFS, Y_POSITION, pos_value.getInt());
												vnfPosition->setY(pos_value.getInt());
											}
											else
											{
												string error = string("Invalid key ") + pos_name.c_str() + " in an Object: " + POSITION;
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}
										}
									}
									else
									{
										string error = string("Invalid key ") + nf_name.c_str() + " in a VNF of " + VNFS;
										ULOG_WARN(error.c_str());
										throw new GraphParserException(std::move(error));
									}
								}
								if(!foundName)
								{
									string error = string("Key ") + _NAME + " not found in an element of " + VNFS;
									ULOG_WARN(error.c_str());
									throw new GraphParserException(std::move(error));
								}
								if(!foundFunctionalCapability){
									string error = string("Functional capability not found in a VNF of ") + VNFS;
									ULOG_WARN(error.c_str());
									functional_capability = "";
								}

#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
								highlevel::VNFs vnfs(id, name, groups, vnf_template, functional_capability, portS, controlPorts,environmentVariables);
#else
								highlevel::VNFs vnfs(id, name, groups, vnf_template, functional_capability, portS);
#endif

								//update information on the trusted status of VNF ports
								for(list<highlevel::vnf_port_t>::iterator port = portS.begin(); port != portS.end(); port++)
								{
									map<string, bool> trusted_port_vnf = trusted_ports[id];
									trusted_port_vnf[port->id] = port->configuration.trusted;
									trusted_ports[id] = trusted_port_vnf;
									
									map<string,string> trusted_ports_mac_addresses_vnf = trusted_ports_mac_addresses[id];
									trusted_ports_mac_addresses_vnf[port->id] = port->configuration.mac_address;
									trusted_ports_mac_addresses[id] = trusted_ports_mac_addresses_vnf;
									
									ULOG_DBG("VNF \"%s\" - port \"%s\" - \"%s\"",id.c_str(),port->id.c_str(),(port->configuration.trusted)? "trusted":"untrusted");
								}

								if(vnfPosition!=NULL)
									vnfs.setPosition(vnfPosition);

								graph.addVNF(vnfs);
								portS.clear();
#ifdef ENABLE_UNIFY_PORTS_CONFIGURATION
								controlPorts.clear();
								environmentVariables.clear();
#endif
							}// end iteration on VNFs
						}
						catch(std::exception& e)
						{
								string error = string("The ") + VNFS + " element does not respect the JSON syntax: " + e.what();
								ULOG_WARN(error.c_str());
								throw new GraphParserException(error);
						}
					}
					//Identify the end-points
					else if(fg_name == END_POINTS)
					{
						try
						{
							try
							{
								fg_value.getArray();
							} catch(std::exception& e)
							{
								string error = string("The content does not respect the JSON syntax: ") + END_POINTS + " should be an Array";
								ULOG_WARN(error.c_str());
								throw new GraphParserException(std::move(error));
							}

							/**
							*	According to https://github.com/netgroup-polito/nffg-library/blob/master/schema.json , an endpoint can contain:
							*	- id
							*	- name
							*	- type
							*		- internal
							*		- interface
							*		- gre-tunnel
							*		- vlan
							*	- Other information that depend on the type
							*/

							const Array& end_points_array = fg_value.getArray();

							ULOG_DBG("\"%s\"",END_POINTS);

							//Iterate on the end-points
							set<string> usedEndpointID;
							for( unsigned int ep = 0; ep < end_points_array.size(); ++ep )
							{
								try{
									end_points_array[ep].getObject();
								} catch(std::exception& e)
								{
									string error = string("The content does not respect the JSON syntax: element of ") + END_POINTS + " should be an Object";
									ULOG_WARN(error.c_str());
									throw new GraphParserException(std::move(error));
								}

								//This is a endpoints, with a name, a type, and an interface
								Object end_points = end_points_array[ep].getObject();
								highlevel::Position *endpointPosition = NULL;
								//Iterate on the elements of an endpoint
								for(Object::const_iterator aep = end_points.begin(); aep != end_points.end(); aep++)
								{
									const string& ep_name  = aep->first;
									const Value&  ep_value = aep->second;

									if(ep_name == _ID)
									{
										id = ep_value.getString();
										//Two endpoints cannot have the same ID
										if(usedEndpointID.count(id) != 0)
										{
											string error = string("Found two endpoints with the same ID: ") + id.c_str() + ". This is not valid.";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}
										usedEndpointID.insert(id);
									}
									else if(ep_name == _NAME)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",END_POINTS,_NAME,ep_value.getString().c_str());
										e_name = ep_value.getString();
									}
									else if(ep_name == EP_TYPE)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",END_POINTS,EP_TYPE,ep_value.getString().c_str());
										string type = ep_value.getString();
									}
									//identify interface end-points
									else if(ep_name == EP_IFACE)
									{
										try
										{
											ep_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + EP_IFACE + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										Object ep_iface = ep_value.getObject();
										e_if = true;

										for(Object::const_iterator epi = ep_iface.begin(); epi != ep_iface.end(); epi++)
										{
											const string& epi_name  = epi->first;
											const Value&  epi_value = epi->second;

											if(epi_name == NODE_ID)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_IFACE,NODE_ID,epi_value.getString().c_str());
												ULOG_WARN("Element \"%s\" is ignored by the current implementation of the %s", NODE_ID,EP_IFACE);
											}
											else if(epi_name == IF_NAME)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_IFACE,IF_NAME,epi_value.getString().c_str());

												interface = epi_value.getString();
												iface_id[id] = epi_value.getString();
												ULOG_DBG("\"%s\"->\"%s\"",id.c_str(), iface_id[id].c_str());
											}
											else
											{
												string error = string("Invalid key ") + epi_name.c_str() + " inside " + EP_IFACE;
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}
										}
									}
									else if(ep_name == EP_INTERNAL)
									{
										ULOG_DBG("Element \"%s\" is an empty object!", EP_INTERNAL,MODULE_NAME);

										try{
											ep_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + EP_INTERNAL + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										Object ep_internal = ep_value.getObject();

										e_internal = true;

										for(Object::const_iterator epi = ep_internal.begin(); epi != ep_internal.end(); epi++)
										{
											const string& epi_name  = epi->first;
											const Value&  epi_value = epi->second;

											if(epi_name == INTERNAL_GROUP)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_INTERNAL,INTERNAL_GROUP,epi_value.getString().c_str());
												in_group = epi_value.getString();
											}
										}

										internal_id[id] = in_group;
										ULOG_DBG("\"%s\"->\"%s\"",id.c_str(),internal_id[id].c_str());
									}
									//identify interface-out end-points
									else if(ep_name == EP_IFACE_OUT)
									{
										ULOG_WARN("Element \"%s\" is ignored by the current implementation of the %s. This type of end-point is not supported!", EP_IFACE_OUT,MODULE_NAME);
									}
									//identify vlan end-points
									else if(ep_name == VLAN)
									{
										try{
											ep_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + VLAN + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										Object ep_vlan = ep_value.getObject();

										e_vlan = true;

										for(Object::const_iterator epi = ep_vlan.begin(); epi != ep_vlan.end(); epi++)
										{
											const string& epi_name  = epi->first;
											const Value&  epi_value = epi->second;

											if(epi_name == V_ID)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VLAN,VLAN_ID,epi_value.getString().c_str());
												v_id = epi_value.getString();
											}
											else if(epi_name == IF_NAME)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VLAN,IF_NAME,epi_value.getString().c_str());
												interface = epi_value.getString();
											}
											else if(epi_name == NODE_ID)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",VLAN,NODE_ID,epi_value.getString().c_str());
												ULOG_WARN("Element \"%s\" is ignored by the current implementation of the %s", NODE_ID,VLAN);
											}
										}

										vlan_id[id] = make_pair(v_id, interface); //it maps: endpoint id - (vlan id, physical interface)
										ULOG_DBG("\"%s\"->\"%s\":\"%s\"",id.c_str(),vlan_id[id].first.c_str(),vlan_id[id].second.c_str());
									}
									else if(ep_name == EP_GRE)
									{
										try
										{
											ep_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + EP_GRE + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}


										e_gre=true;

										//In order to get e-d-point ID (it wil be parsed later)
										Object::const_iterator aep2 = aep;
										aep2++;
										for(; aep2 != end_points.end(); aep2++)
										{
											const string& ep2_name  = aep2->first;
											const Value&  ep2_value = aep2->second;
											if(ep2_name == _ID)
											{
												id = ep2_value.getString();
												break;
											}
										}

										gre_info_t gre_info;

										bool found_local_ip = false, found_remote_ip = false, found_key = false;

										ep_gre=ep_value.getObject();

										for(Object::const_iterator epi = ep_gre.begin(); epi != ep_gre.end(); epi++)
										{
											const string& epi_name  = epi->first;
											const Value&  epi_value = epi->second;

											if(epi_name == LOCAL_IP)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_GRE,LOCAL_IP,epi_value.getString().c_str());
												found_local_ip = true;
												gre_info.local_ip = epi_value.getString();
											}
											else if(epi_name == REMOTE_IP)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_GRE,REMOTE_IP,epi_value.getString().c_str());
												found_remote_ip = true;
												gre_info.remote_ip = epi_value.getString();
											}
											else if(epi_name == TTL)
											{
												ULOG_DBG_INFO("\"%s\"->\"%s\": \"%s\"",EP_GRE,TTL,epi_value.getString().c_str());

												gre_info.ttl = epi_value.getString();
											}
											else if(epi_name == GRE_KEY)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_GRE,GRE_KEY,epi_value.getString().c_str());
												found_key = true;
												gre_info.key = epi_value.getString();
											}
											else if(epi_name == SAFE)
											{
												ULOG_DBG_INFO("\"%s\"->\"%s\": \"%d\"",EP_GRE,SAFE,epi_value.getBool());

												gre_info.safe = epi_value.getBool();
											}
											else
											{
												string error = string("Invalid key ") + epi_name.c_str() + " inside " + EP_GRE;
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}
										}

										if(!found_local_ip || !found_remote_ip || !found_key)
										{
											string error = string("Key ") + LOCAL_IP + ", or  Key " + REMOTE_IP + ", or Key " + GRE_KEY + " not found in " + EP_GRE;
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										gre_id[id] = gre_info;

									}
									else if(ep_name == EP_HOSTSTACK)
									{

										try
										{
											ep_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + EP_HOSTSTACK + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										e_hs=true;

										//In order to get end-point ID (it wil be parsed later, but i need it now)
										Object::const_iterator aep2 = aep;
										aep2++;
										for(; aep2 != end_points.end(); aep2++)
										{
											const string& ep2_name  = aep2->first;
											const Value&  ep2_value = aep2->second;
											if(ep2_name == _ID)
											{
												id = ep2_value.getString();
												break;
											}
										}

										hoststack_info_t hs_info;
										Object ep_hostStack = ep_value.getObject();
										hs_info.configuration=NONE;
										string ipAddress,macAddress;
										for(Object::const_iterator eph = ep_hostStack.begin(); eph != ep_hostStack.end(); eph++)
										{
											const string& eph_name  = eph->first;
											const Value&  eph_value = eph->second;


											if(eph_name == CONFIGURATION)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_HOSTSTACK,CONFIGURATION,eph_value.getString().c_str());
												string confTemp = eph_value.getString();
												if(strcmp(confTemp.c_str(),CONF_DHCP)==0)
													hs_info.configuration=DHCP;
												else if(strcmp(confTemp.c_str(),CONF_STATIC)==0)
													hs_info.configuration=STATIC;
												else if(strcmp(confTemp.c_str(),CONF_PPPOE)==0)
													hs_info.configuration=PPPOE;
												else
												{
													string error = string("Invalid value ") + confTemp.c_str() + " for key " + eph_name.c_str() + " inside " + EP_HOSTSTACK;
													ULOG_WARN(error.c_str());
													throw new GraphParserException(std::move(error));
												}
											}
											else if(eph_name == IP_ADDRESS)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_HOSTSTACK,IP_ADDRESS,eph_value.getString().c_str());
												hs_info.ipAddress=eph_value.getString();
											}
											else if(eph_name == MAC_ADDRESS)
											{
												ULOG_DBG("\"%s\"->\"%s\": \"%s\"",EP_HOSTSTACK,MAC_ADDRESS,eph_value.getString().c_str());
												hs_info.macAddress=eph_value.getString();
											}
											else
											{
												string error = string("Invalid key ") + eph_name.c_str() + " inside " + EP_HOSTSTACK;
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}
										}

										if(hs_info.configuration==NONE)
										{
											string error = string("Keywork ") + CONFIGURATION + " must be present inside " + EP_HOSTSTACK;
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}

										hoststack_id[id]=hs_info;
									}
									else if(ep_name == POSITION)
									{
										try
										{
											ep_value.getObject();
										} catch(std::exception& e)
										{
											string error = string("The content does not respect the JSON syntax: ") + POSITION + " should be an Object";
											ULOG_WARN(error.c_str());
											throw new GraphParserException(std::move(error));
										}
										Object position_Object = ep_value.getObject();
										endpointPosition = new highlevel::Position();
										//Parse the position object
										for(Object::const_iterator pos = position_Object.begin(); pos != position_Object.end(); pos++) {
											const string &pos_name = pos->first;
											const Value &pos_value = pos->second;

											if (pos_name == X_POSITION) {
												ULOG_DBG("\"%s\"->\"%s\": \"%d\"", END_POINTS, X_POSITION, pos_value.getInt());
												endpointPosition->setX(pos_value.getInt());
											}
											else if (pos_name == Y_POSITION) {
												ULOG_DBG("\"%s\"->\"%s\": \"%d\"", END_POINTS, Y_POSITION, pos_value.getInt());
												endpointPosition->setY(pos_value.getInt());
											}
											else
											{
												string error = string("Invalid key ") + pos_name.c_str() + " inside " + POSITION;
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}
										}
									}
									else
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,END_POINTS,ep_value.getString().c_str());
								}//End of iteration on the elements of an endpoint

								//add interface end-points
								if(e_if)
								{
									//FIXME: are we sure that "interface" has been specified?
									highlevel::EndPointInterface ep_if(id, e_name, interface);
									if(endpointPosition!=NULL)
										ep_if.setPosition(endpointPosition);
									graph.addEndPointInterface(ep_if);
									e_if = false;
								}
								//add internal end-points
								else if(e_internal)
								{
									highlevel::EndPointInternal ep_internal(id, e_name, in_group);
									if(endpointPosition!=NULL)
										ep_internal.setPosition(endpointPosition);
									graph.addEndPointInternal(ep_internal);

									e_internal = false;
								}
								//add vlan end-points
								else if(e_vlan)
								{
									//FIXME: are we sure that "interface" and "v_id" have been specified?
									highlevel::EndPointVlan ep_vlan(id, e_name, v_id, interface);
									if(endpointPosition!=NULL)
										ep_vlan.setPosition(endpointPosition);
									graph.addEndPointVlan(ep_vlan);
									e_vlan = false;
								}
								//Add gre-tunnel end-points
								else if(e_gre)
								{
									gre_info_t gre_info = gre_id[id];
									highlevel::EndPointGre ep_gre(id, e_name, gre_info.local_ip, gre_info.remote_ip, gre_info.key, gre_info.ttl, gre_info.safe);
									if(endpointPosition!=NULL)
										ep_gre.setPosition(endpointPosition);
									graph.addEndPointGre(ep_gre);
									e_gre = false;
								}
								//Add hoststack end-points
								else if(e_hs)
								{
									hoststack_info_t hoststack_info = hoststack_id[id];
									highlevel::EndPointHostStack ep_hs(id, e_name, hoststack_info.configuration, hoststack_info.ipAddress, hoststack_info.macAddress);
									if(endpointPosition!=NULL)
										ep_hs.setPosition(endpointPosition);
									graph.addEndPointHostStack(ep_hs);
									e_hs = false;
								}

							}//End iteration on the endpoints
						}
						catch(std::exception& e)
						{
							string error = string("The ") + END_POINTS + " element does not respect the JSON syntax: " + e.what();
							ULOG_WARN(error.c_str());
							throw new GraphParserException(error);
						}
					}//End if(fg_name == END_POINTS)
					//Identify the big-switch
					else if(fg_name == BIG_SWITCH)
					{
						foundBigSwitch = true;
						try{
							fg_value.getObject();
						} catch(std::exception& e)
						{
							string error = string("The content does not respect the JSON syntax: ") + BIG_SWITCH + " should be an Object";
							ULOG_WARN(error.c_str());
							throw new GraphParserException(std::move(error));
						}

						big_switch = fg_value.getObject();
						//The content of the "big-switch" element will be parsed later.
						//In fact it requires that the "end-points" element and the "VNFs" element will be found
					}
					else
					{
						string error = string("Invalid key ") + fg_name.c_str() + " inside " + FORWARDING_GRAPH;
						ULOG_WARN(error.c_str());
						throw new GraphParserException(std::move(error));
					}

				}// End iteration on the elements of "forwarding-graph"

				/*******************************************/
				// Iterate on the element of the big-switch
				bool foundFlowRules = false; //flow rules are mandatory if the big-switch is specified
				for(Object::const_iterator bs = big_switch.begin(); bs != big_switch.end(); bs++)
				{
					const string& bs_name  = bs->first;
					const Value&  bs_value = bs->second;

					if (bs_name == FLOW_RULES)
					{
						foundFlowRules = true;

						try
						{
							try{
								bs_value.getArray();
							} catch(std::exception& e)
							{
								string error = string("The content does not respect the JSON syntax: ") + FLOW_RULES + " should be an Array";
								ULOG_WARN(error.c_str());
								throw new GraphParserException(std::move(error));
							}

							const Array& flow_rules_array = bs_value.getArray();

							//Itearate on the flow rules
							for( unsigned int fr = 0; fr < flow_rules_array.size(); ++fr )
							{
								//This is a rule, with a match, an action, and an ID
								Object flow_rule = flow_rules_array[fr].getObject();
								highlevel::Action *action = new highlevel::Action();
								list<GenericAction*> genericActions;
								list<OutputAction*> outputActions;
								highlevel::Match match;
								string ruleID;
								uint64_t priority = 0;

								bool foundAction = false;
								bool foundMatch = false;
								bool foundID = false;

								//Parse the rule
								for(Object::const_iterator afr = flow_rule.begin(); afr != flow_rule.end(); afr++)
								{
									const string& fr_name  = afr->first;
									const Value&  fr_value = afr->second;
									if(fr_name == _ID)
									{
										foundID = true;
										ruleID = fr_value.getString();
									}
									else if(fr_name == F_DESCR)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%s\"",FLOW_RULES,F_DESCR,fr_value.getString().c_str());

										//XXX: currently, this information is ignored
									}
									else if(fr_name == PRIORITY)
									{
										ULOG_DBG("\"%s\"->\"%s\": \"%d\"",FLOW_RULES,PRIORITY,fr_value.getInt());

										priority = fr_value.getInt();
									}
									else if(fr_name == MATCH)
									{
										try{
											foundMatch = true;
											MatchParser::parseMatch(fr_value.getObject(),match,(*action),iface_id,internal_id,vlan_id,gre_id,hoststack_id,trusted_ports,trusted_ports_mac_addresses);
										} catch(std::exception& e)
										{
											string error = string("The ") + MATCH + " element does not respect the JSON syntax: " + e.what();
											ULOG_WARN(error.c_str());
											throw new MatchParserException(error);
										}
									}
									else if(fr_name == ACTIONS)
									{
										try
										{
											try
											{
												fr_value.getArray();
											} catch(std::exception& e)
											{
												string error = string("The content does not respect the JSON syntax: ") + ACTIONS + " should be an Array";
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}

											const Array& actions_array = fr_value.getArray();

											enum port_type { VNF_PORT_TYPE, EP_PORT_TYPE, EP_INTERNAL_TYPE };

											//One and only one output_to_port is allowed
											bool foundOneOutputToPort = false;

											//Itearate on all the actions specified for this flowrule
											for( unsigned int ac = 0; ac < actions_array.size(); ++ac )
											{
												foundAction = true;
												try{
													actions_array[ac].getObject();
												} catch(std::exception& e)
												{
													string error = string("The content does not respect the JSON syntax: element of ") + ACTIONS + " should be an Object";
													ULOG_WARN(error.c_str());
													throw new GraphParserException(std::move(error));
												}

												//A specific action of the array can have a single keyword inside
												Object theAction = actions_array[ac].getObject();
												if(theAction.size() != 1)
												{
													string error = string("Too many keywords in an element of ") + ACTIONS;
													ULOG_WARN(error.c_str());
													throw new GraphParserException(std::move(error));
												}

												for(Object::const_iterator a = theAction.begin(); a != theAction.end(); a++)
												{
													const string& a_name  = a->first;
													const Value&  a_value = a->second;

													if(a_name == OUTPUTX)
													{
														//The action is "output_to_port"

														string internal_group;
														string port_in_name = a_value.getString();
														string graph_id;
														string realName;
														const char *port_in_name_tmp = port_in_name.c_str();
														char vnf_id_tmp[BUFFER_SIZE];

														//Check the name of port
														char delimiter[] = ":";
													 	char * pnt;

														port_type p_type = VNF_PORT_TYPE;

														char tmp[BUFFER_SIZE];
														strcpy(tmp,(char *)port_in_name_tmp);
														pnt=strtok(tmp, delimiter);
														int i = 0;

														//The "output_to_port" action can refer to:
														//	- an endpoint
														//	- the port of a VNF
														while( pnt!= NULL )
														{
															switch(i)
															{
																case 0:
																	//VNFs port type
																	if(strcmp(pnt,VNF) == 0)
																	{
																		p_type = VNF_PORT_TYPE;
																	}
																	//end-points port type
																	else if (strcmp(pnt,ENDPOINT) == 0)
																	{
																		p_type = EP_PORT_TYPE;
																	}
																	break;
																case 1:
																	if(p_type == VNF_PORT_TYPE)
																	{
																		strcpy(vnf_id_tmp,pnt);
																		strcat(vnf_id_tmp, ":");
																	}
																	break;
																case 3:
																	if(p_type == VNF_PORT_TYPE)
																	{
																		strcat(vnf_id_tmp,pnt);
																	}
															}

															pnt = strtok( NULL, delimiter );
															i++;
														}

														foundOneOutputToPort = true;

														if(p_type == VNF_PORT_TYPE)
														{
															//This is an output action referred to a VNF port

															//convert char *vnf_id_tmp to string vnf_id
															string vnf_id(vnf_id_tmp, strlen(vnf_id_tmp));

															ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,VNF,vnf_id.c_str());

															string id = GraphParserUtils::nfId(vnf_id);
															char *tmp_vnf_id = new char[BUFFER_SIZE];
															strcpy(tmp_vnf_id, (char *)vnf_id.c_str());
															unsigned int port = GraphParserUtils::nfPort(string(tmp_vnf_id));
															bool is_port = GraphParserUtils::nfIsPort(string(tmp_vnf_id));

															if(id == "" || !is_port)
															{
																string error = string("Network function ") + vnf_id.c_str() + " is not valid. It must be in the form \"id:port\"";
																ULOG_WARN(error.c_str());
																throw new GraphParserException(std::move(error));
															}

															/*nf port starts from 0 - here we want that they start from 1*/
															port++;

															action->addOutputAction(new ActionNetworkFunction(id, string(port_in_name_tmp), port));
														}
														//endpoints port type
														else if(p_type == EP_PORT_TYPE)
														{
															//This is an output action referred to an endpoint

															bool iface_found = false, internal_found = false, vlan_found = false, gre_found=false, hostStack_found=false;

															char *s_a_value = new char[BUFFER_SIZE];
															strcpy(s_a_value, (char *)a_value.getString().c_str());
															string epID = GraphParserUtils::epName(a_value.getString());
															if(epID != "")
															{
																map<string,string>::iterator it = iface_id.find(epID);
																map<string,string>::iterator it1 = internal_id.find(epID);
																map<string,pair<string,string> >::iterator it2 = vlan_id.find(epID);
																map<string,gre_info_t>::iterator it3 = gre_id.find(epID);
																map<string,hoststack_info_t>::iterator it4 = hoststack_id.find(epID);

																if(it != iface_id.end())
																{
																	//physical port
																	realName.assign(iface_id[epID]);
																	iface_found = true;
																}
																else if(it1 != internal_id.end())
																{
																	//internal
																	internal_group.assign(internal_id[epID]);
																	internal_found = true;
																}
																else if(it2 != vlan_id.end())
																{
																	//vlan
																	vlan_found = true;
																}
																else if(it3 != gre_id.end())
																{
																	//gre
																	gre_found = true;
																}
																else if(it4 != hoststack_id.end())
																{
																	hostStack_found = true;
																}
															}
															//physical endpoint
															if(iface_found)
															{
																action->addOutputAction(new ActionPort(realName, string(s_a_value)));
															}
															else if(internal_found)
															{
																if(internal_group == "")
																{
																	string error = string("Internal endpoint ") + value.getString().c_str() + " is not valid. It must have attribute " + INTERNAL_GROUP;
																	ULOG_WARN(error.c_str());
																	throw new GraphParserException(std::move(error));
																}
																action->addOutputAction(new ActionEndpointInternal(internal_group, string(s_a_value)));
															}
															//vlan endpoint
															else if(vlan_found)
															{
																vlan_action_t actionType;
																unsigned int vlanID = 0;

																actionType = ACTION_ENDPOINT_VLAN_PUSH;

																sscanf(vlan_id[epID].first.c_str(),"%u",&vlanID);

																/*add "output_port" action*/
																action->addOutputAction(new ActionPort(vlan_id[epID].second, string(s_a_value)));
																/*add "push_vlan" action*/
																GenericAction *ga = new VlanAction(actionType,string(s_a_value),vlanID);
																action->addGenericAction(ga);
															}
															//gre-tunnel endpoint
															else if(gre_found)
															{
																action->addOutputAction(new ActionEndpointGre(epID, string(s_a_value)));
															}
															else if(hostStack_found)
															{
																action->addOutputAction(new ActionEndPointHostStack(epID, string(s_a_value)));
															}
														}
													}//End action == output_to_port
													else if(a_name == SET_VLAN_ID)
													{
														ULOG_DBG_INFO("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_VLAN_ID,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == SET_VLAN_PRIORITY)
													{
														ULOG_DBG_INFO("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_VLAN_PRIORITY,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == VLAN_PUSH)
													{
														//The action is "push_vlan"

														vlan_action_t actionType;
														unsigned int vlanID = 0;

														actionType = ACTION_VLAN_PUSH;

														string strVlanID = a_value.getString();
														vlanID = strtol (strVlanID.c_str(),NULL,0);

														GenericAction *ga = new VlanAction(actionType,string(""),vlanID);
														genericActions.push_back(ga);

													}//end if(a_name == VLAN_PUSH)
													else if(a_name == VLAN_POP)
													{
														//A vlan pop action is required
														vlan_action_t actionType;
														unsigned int vlanID = 0;

														bool is_vlan_pop = a_value.getBool();
														if(is_vlan_pop)
														{
															actionType = ACTION_VLAN_POP;

															//Finally, we are sure that the command is correct!
															GenericAction *ga = new VlanAction(actionType,string(""),vlanID);
															genericActions.push_back(ga);
														}
													}//end if(a_name == VLAN_POP)
													else if(a_name == SET_ETH_SRC_ADDR)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,SET_ETH_SRC_ADDR);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_ETH_SRC_ADDR,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == SET_ETH_DST_ADDR)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,SET_ETH_DST_ADDR);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_ETH_DST_ADDR,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == SET_IP_SRC_ADDR)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,SET_IP_SRC_ADDR);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_IP_SRC_ADDR,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == SET_IP_DST_ADDR)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,SET_IP_DST_ADDR);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_IP_DST_ADDR,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == SET_IP_TOS)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,SET_IP_TOS);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_IP_TOS,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == SET_L4_SRC_PORT)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,SET_L4_SRC_PORT);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_L4_SRC_PORT,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == SET_L4_DST_PORT)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,SET_L4_DST_PORT);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,SET_L4_DST_PORT,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == OUT_TO_QUEUE)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,OUT_TO_QUEUE);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,OUT_TO_QUEUE,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == DROP)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,DROP);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,DROP,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else if(a_name == OUTPUT_TO_CTRL)
													{
														ULOG_WARN("\"%s\" \"%s\" not available",ACTIONS,OUTPUT_TO_CTRL);

														ULOG_DBG("\"%s\"->\"%s\": \"%s\"",ACTIONS,OUTPUT_TO_CTRL,a_value.getString().c_str());

														//XXX: currently, this information is ignored
													}
													else
													{
														string error = string("Invalid key ") + a_name.c_str() + " inside " + ACTIONS;
														ULOG_WARN(error.c_str());
														throw new GraphParserException(std::move(error));
													}
												}//end iteration on the keywords of an action element (remember that a single keywork is allowed in each element)


											}//Here terminates the loop on the array actions
											if(!foundOneOutputToPort)
											{
												//"output_to_port" is a mandatory action
												string error = string("Key ") + OUTPUTX + " not found in " + ACTIONS;
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}
											assert(action != NULL);
											for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
												action->addGenericAction(*ga);
										}//end of try
										catch(std::exception& e)
										{
											string error = string("The ") + ACTIONS + " element does not respect the JSON syntax: " + e.what();
											ULOG_WARN(error.c_str());
											throw new GraphParserException(error);
										}
									}//end if(fr_name == ACTION)
									else
									{
										string error = string("Invalid key ") + name.c_str() + " in a rule of " + FLOW_RULES;
										ULOG_WARN(error.c_str());
										throw new GraphParserException(std::move(error));
									}
								}

								if(!foundAction || !foundMatch || !foundID)
								{
									string error = string("Key ") + _ID + ", or  Key " + MATCH + ", or Key " + ACTIONS + " or all of them not found in an elmenet of " + FLOW_RULES;
									ULOG_WARN(error.c_str());
									throw new GraphParserException(std::move(error));
								}

								highlevel::Rule rule(match,action,ruleID,priority);

								if(!graph.addRule(rule))
								{
									string error = string("The graph has at least two rules with the same ID: ") + ruleID.c_str() + ". This is not valid.";
									ULOG_WARN(error.c_str());
									throw new GraphParserException(std::move(error));
								}

								list<GenericAction*> gaList = action->getGenericActions();
								for(list<GenericAction*>::iterator ga = gaList.begin(); ga != gaList.end() ; ga++)
								{
									VlanAction* va = dynamic_cast<VlanAction*>(*ga);
									if(va!=NULL) {
										if(va->getType()==ACTION_VLAN_POP)
										{
											if(!match.checkVlanPresence())
											{
												string error = "A POP_VLAN action without specific match on vlan ID has been specified... The corresponding rule can not work";
												ULOG_WARN(error.c_str());
												throw new GraphParserException(std::move(error));
											}
										}
									}
								}

							}//for( unsigned int fr = 0; fr < flow_rules_array.size(); ++fr )

							bool same_priority = false;
							list<highlevel::Rule> rules = graph.getRules();
							for(list<highlevel::Rule>::iterator r = rules.begin(); r != rules.end(); r++)
							{
								list<highlevel::Rule>::iterator next_rule = r;
								next_rule++;
								if(next_rule != rules.end())
								{
									uint64_t priority = (*r).getPriority();
									for(list<highlevel::Rule>::iterator r1 = next_rule; r1 != rules.end(); r1++)
									{
										if((*r1).getPriority() == priority)
											same_priority = true;
									}
								}
							}

							if(same_priority)
							{
								ULOG_WARN("One or more flow rule with the same priority...");
								ULOG_WARN("Note that, if they match the same port, they may cause a conflict on the vSwitch!");
							}
						}
						catch(std::exception& e)
						{
							string error = string("The ") + FLOW_RULES + " element does not respect the JSON syntax: " + e.what();
							ULOG_WARN(error.c_str());
							throw new GraphParserException(error);
						}
					}// end  if (fg_name == FLOW_RULES)
					else
					{
						string error = string("Invalid key ") + bs_name.c_str() + " inside " + BIG_SWITCH;
						ULOG_WARN(error.c_str());
						throw new GraphParserException(std::move(error));
					}
				}//End iteration on the elements inside "big-switch"

				if(foundBigSwitch && !foundFlowRules)
				{
					string error = string("Key ") + FLOW_RULES + " not found in " + BIG_SWITCH;
					ULOG_WARN(error.c_str());
					throw new GraphParserException(std::move(error));
				}


			}//End if(name == FORWARDING_GRAPH)
			else
			{
				string error = string("Invalid key ") + name.c_str() + " inside ROOT";
				ULOG_WARN(error.c_str());
				throw new GraphParserException(std::move(error));
			}
		}
		if(!foundFlowGraph)
		{
			string error = string("Key ") + FORWARDING_GRAPH + " not found";
			ULOG_WARN(error.c_str());
			throw new GraphParserException(std::move(error));
		}
	}catch(std::exception& e)
	{
		string error = string("The content does not respect the JSON syntax: ") + e.what();
		ULOG_WARN(error.c_str());
		throw new GraphParserException(error);
	}

	return;
}

