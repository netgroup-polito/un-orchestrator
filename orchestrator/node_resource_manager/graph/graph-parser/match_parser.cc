#ifndef __STDC_FORMAT_MACROS
	#define __STDC_FORMAT_MACROS
	#include <inttypes.h>
	#undef __STDC_FORMAT_MACROS
#else
	#include <inttypes.h>
#endif
#include "match_parser.h"

#include <stdio.h>
//#include <inttypes.h>

static const char LOG_MODULE_NAME[] = "Match-Parser";

void MatchParser::parseMatch(Object match_element, highlevel::Match &match, highlevel::Action &action, map<string,string > &iface_id, map<string,string> &internal_id, map<string,pair<string,string> > &vlan_id, map<string,gre_info_t> &gre_id, map<string,hoststack_info_t> &hostStack_id, map<string, map<string, bool> > trusted_ports, map<string, map<string,string> >trusted_ports_mac_addresses)
{
	bool foundOne = false;
	bool foundEndPointID = false, foundProtocolField = false, definedInCurrentGraph = false;
	enum port_type { VNF_PORT_TYPE, EP_PORT_TYPE, EP_INTERNAL_TYPE };

	for(Object::const_iterator i = match_element.begin(); i != match_element.end(); i++)
	{
		const string& name  = i->first;
		const Value&  value = i->second;

		if(name == PORT_IN)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,PORT_IN,value.getString().c_str());
			if(foundOne)
			{
				string error = string("Only one between keys \"") + PORT_IN + "\", \"" + VNF + "\" and \"" + ENDPOINT + "\" are allowed in \"" + MATCH + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}

			foundOne = true;

			string internal_group;
			string port_in_name = value.getString();
			string realName;
			string v_id;
			string graph_id;
			const char *port_in_name_tmp = port_in_name.c_str();
			char vnf_id_tmp[BUFFER_SIZE], real_port_id[BUFFER_SIZE];

			//Check the name of port
			char delimiter[] = ":";
		 	char * pnt;

			port_type p_type = VNF_PORT_TYPE;

			char tmp[BUFFER_SIZE];
			strcpy(tmp,(char *)port_in_name_tmp);
			pnt=strtok(tmp, delimiter);
			int i = 0;

			while( pnt!= NULL )
			{
				switch(i)
				{
					case 0:
						//VNFs port type
						if(strcmp(pnt,VNF) == 0)
						{
							p_type = VNF_PORT_TYPE;
							match.setNFEndpointPort(port_in_name_tmp);
						}
						//end-point port type
						else if(strcmp(pnt,ENDPOINT) == 0){
							p_type = EP_PORT_TYPE;
							match.setInputEndpoint(port_in_name_tmp);
						}
						break;
					case 1:
						if(p_type == VNF_PORT_TYPE)
						{
							strcpy(vnf_id_tmp,pnt);
							strcat(vnf_id_tmp,":");
						}
						break;
					case 2:
						if(p_type == VNF_PORT_TYPE)
						{
							//We need the real port id to understand if a VNF port is trusted 
							//or not
							//XXX I don't know why the port name must be in the form abc:1, and just the number is actually used
							strcpy(real_port_id,pnt);
							strcat(real_port_id,":");
						}
						break;
					case 3:
						if(p_type == VNF_PORT_TYPE)
						{
							strcat(vnf_id_tmp,pnt);
							strcat(real_port_id,pnt);
						}
				}

				pnt = strtok( NULL, delimiter );
				i++;
			}

			//VNFs port type
			if(p_type == VNF_PORT_TYPE)
			{
				string vnf_and_port_id(vnf_id_tmp, strlen(vnf_id_tmp));

				string vnf_id = GraphParserUtils::nfId(vnf_and_port_id);
				char *tmp_vnf_name = new char[BUFFER_SIZE];
				strcpy(tmp_vnf_name, (char *)vnf_and_port_id.c_str());
				unsigned int port_id = GraphParserUtils::nfPort(string(tmp_vnf_name));
				bool is_port = GraphParserUtils::nfIsPort(string(tmp_vnf_name));

				if(vnf_id == "" || !is_port)
				{
					string error = string("Network function \"") + vnf_id_tmp + "\" is not valid. It must be in the form \"name:port\"";
					ULOG_WARN(error.c_str());
					throw new MatchParserException(std::move(error));
				}
				/*nf port starts from 0 - here we want that the ID starts from 1*/
				port_id++;

				match.setNFport(vnf_id,port_id);

				//check whether the port is trusted or not
				map<string,bool> trusted_ports_vnf = trusted_ports[vnf_id];
				bool trusted = trusted_ports_vnf[string(real_port_id)];

				ULOG_DBG_INFO("Network function \"%s\" has the port \"%s\" which is %s",vnf_id.c_str(), real_port_id, (trusted)? "trusted" : "untrusted");
				if(trusted)
				{
					//We have to extend the action with a rule that set the source mac address as specified in the NF-FG
					map<string,string> mac_addresses_vnf = trusted_ports_mac_addresses[vnf_id];
					GenericAction *ga = new EthAction(ACTION_TRUSTED_PORT,mac_addresses_vnf[real_port_id]);
					action.addGenericAction(ga);
				}

			}
			//endpoints port type
			else if(p_type == EP_PORT_TYPE)
			{
				bool iface_found = false, internal_found = false, vlan_found = false, gre_found=false, hoststack_found=false;
				char *s_value = new char[BUFFER_SIZE];
				strcpy(s_value, (char *)value.getString().c_str());
				string eP = GraphParserUtils::epName(value.getString());
				if(eP != ""){
					map<string,string>::iterator it = iface_id.find(eP);
					map<string,string>::iterator it1 = internal_id.find(eP);
					map<string,pair<string,string> >::iterator it2 = vlan_id.find(eP); //check if the endpoint has a vlan ID associated (in this case, the endpoint is a vlan endpoint)
					map<string,gre_info_t>::iterator it3 = gre_id.find(eP);
					map<string,hoststack_info_t>::iterator it4 = hostStack_id.find(eP);

					if(it != iface_id.end())
					{
						//physical port
						realName.assign(iface_id[eP]);
						iface_found = true;
					}
					else if(it1 != internal_id.end())
					{
						//internal
						internal_group.assign(internal_id[eP]);
						internal_found = true;
					}
					else if(it2 != vlan_id.end())
					{
						//vlan
						v_id.assign(vlan_id[eP].first);
						vlan_found = true;
					}
					else if(it3 != gre_id.end())
					{
						//gre
						gre_found = true;
					}
					else if(it4 != hostStack_id.end())
					{
						//hoststack endpoint
						hoststack_found = true;
					}
				}
				/*physical endpoint*/
				if(iface_found)
				{
					match.setInputPort(realName);
				}
				else if(internal_found)
				{
					//unsigned int endPoint = epPort(string(endpoint_internal));
					if(/*endPoint == 0*/internal_group == "")
					{
						string error = string("Internal endpoint \"") + value.getString().c_str() + "\" is not valid. It must have the \""+INTERNAL_GROUP+"\" attribute";
						ULOG_WARN(error.c_str());
						throw new MatchParserException(std::move(error));
					}

					match.setEndPointInternal(/*graph_id,endPoint*/internal_group);
				}
				/*vlan endpoint*/
				else if(vlan_found)
				{
					uint32_t vlanID = 0;
					vlan_action_t actionType = ACTION_ENDPOINT_VLAN_POP;

					if((sscanf(v_id.c_str(),"%" SCNd32,&vlanID) != 1) && (vlanID > 4094))
					{
						string error = string("Key \"") + VLAN_ID + "\" with wrong value \"" + value.getString().c_str() + "\"";
						ULOG_WARN(error.c_str());
						throw new MatchParserException(std::move(error));
					}

					//The match on a vlan endpoint requires to
					// - match the input port
					// - match the vlan id
					// - pop the vlan id

					/*add match on "vlan_id"*/
					match.setEndpointVlanID(vlanID & 0xFFFF);

					/*add match on "port_in"*/
					match.setInputPort(vlan_id[eP].second);

					/*add "pop_vlan" action*/
					GenericAction *ga = new VlanAction(actionType,string(""),vlanID);
					action.addGenericAction(ga);
				}
				/*gre-tunnel endpoint*/
				else if(gre_found)
				{
					match.setEndPointGre(eP);
				}
				/*hoststack endpoint*/
				else if(hoststack_found)
				{
					match.setEndPointHoststack(eP);
				}
			}
		}
		else if(name == HARD_TIMEOUT)
		{
			ULOG_WARN("\"%s\" \"%s\" not available",MATCH,HARD_TIMEOUT);

			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,HARD_TIMEOUT,value.getString().c_str());

			//XXX: currently, this information is ignored
		}
		else if(name == ETH_TYPE)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ETH_TYPE,value.getString().c_str());
			uint32_t ethType;
			if((sscanf(value.getString().c_str(),"%" SCNi32,&ethType) != 1) || (ethType > 65535))
			{
				string error = string("Key \"") + ETH_TYPE + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setEthType(ethType & 0xFFFF);
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ETH_TYPE,value.getString().c_str(),ethType);
			foundProtocolField = true;
		}
		else if(name == VLAN_ID)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,VLAN_ID,value.getString().c_str());
			if(value.getString() == ANY_VLAN)
				match.setVlanIDAnyVlan();
			else if(value.getString() == NO_VLAN)
				match.setVlanIDNoVlan();
			else
			{
				uint32_t vlanID;
				if((sscanf(value.getString().c_str(),"%" SCNi32,&vlanID) != 1) && (vlanID > 4094))
				{
					string error = string("Key \"") + VLAN_ID + "\" with wrong value \"" + value.getString().c_str() + "\"";
					ULOG_WARN(error.c_str());
					throw new MatchParserException(std::move(error));
				}
				match.setVlanID(vlanID & 0xFFFF);
			}
			foundProtocolField = true;
		}
		else if(name == VLAN_PRIORITY)
		{
			ULOG_WARN("\"%s\" \"%s\" not available",MATCH,VLAN_PRIORITY);

			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,VLAN_PRIORITY,value.getString().c_str());

			//XXX: currently, this information is ignored
		}
		else if(name == ETH_SRC)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ETH_SRC,value.getString().c_str());
			if(!AddressValidator::validateMac(value.getString().c_str()))
			{
				string error = string("Key \"") + ETH_SRC + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setEthSrc((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ETH_SRC_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ETH_SRC_MASK,value.getString().c_str());
			if(!AddressValidator::validateMac(value.getString().c_str()))
			{
				string error = string("Key \"") + ETH_SRC_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setEthSrcMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ETH_DST)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ETH_DST,value.getString().c_str());
			if(!AddressValidator::validateMac(value.getString().c_str()))
			{
				string error = string("Key \"") + ETH_DST + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setEthDst((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ETH_DST_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ETH_DST_MASK,value.getString().c_str());
			if(!AddressValidator::validateMac(value.getString().c_str()))
			{
				string error = string("Key \"") + ETH_DST_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setEthDstMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == VLAN_PCP)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,VLAN_PCP,value.getString().c_str());
			uint16_t vlanPCP;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&vlanPCP) != 1) || (vlanPCP > 255) )
			{
				string error = string("Key \"") + VLAN_PCP + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			ULOG_DBG("\"%s\"->\"%s\": \"%x\"",MATCH,VLAN_PCP,vlanPCP);
			match.setVlanPCP(vlanPCP & 0xFF);
			foundProtocolField = true;
		}
		else if(name == IP_DSCP)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IP_DSCP,value.getString().c_str());
			uint16_t ipDSCP;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&ipDSCP) != 1) || (ipDSCP > 255) )
			{
				string error = string("Key \"") + IP_DSCP + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpDSCP(ipDSCP & 0xFF);
		}
		else if(name == IP_ECN)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IP_ECN,value.getString().c_str());
			uint16_t ipECN;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&ipECN) != 1) || (ipECN > 255) )
			{
				string error = string("Key \"") + IP_ECN + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpECN(ipECN & 0xFF);
			foundProtocolField = true;
		}
		else if(name == IP_SRC)
		{
			size_t found = value.getString().find(':');
			//IPv6
			if(found!=string::npos)
			{
				ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IP_SRC,value.getString().c_str());
				if(!AddressValidator::validateIpv6(value.getString()))
				{
					string error = string("Key \"") + IP_SRC + "\" with wrong value \"" + value.getString().c_str() + "\"";
					ULOG_WARN(error.c_str());
					throw new MatchParserException(std::move(error));
				}
				match.setIpv6Src((char*)value.getString().c_str());
				foundProtocolField = true;
			//IPv4
			} else {
				ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IP_SRC,value.getString().c_str());
				if(!AddressValidator::validateIpv4(value.getString()))
				{
					string error = string("Key \"") + IP_SRC + "\" with wrong value \"" + value.getString().c_str() + "\"";
					ULOG_WARN(error.c_str());
					throw new MatchParserException(std::move(error));
				}
				match.setIpv4Src((char*)value.getString().c_str());
				foundProtocolField = true;
			}
		}
		else if(name == IPv4_SRC_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv4_SRC_MASK,value.getString().c_str());
			if(!AddressValidator::validateIpv4Netmask(value.getString()))
			{
				string error = string("Key \"") + IPv4_SRC_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpv4SrcMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IP_DST)
		{
			size_t found = value.getString().find(':');
			//IPv6
			if(found!=string::npos)
			{
				ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IP_DST,value.getString().c_str());
				if(!AddressValidator::validateIpv6(value.getString()))
				{
					string error = string("Key \"") + IP_DST + "\" with wrong value \"" + value.getString().c_str() + "\"";
					ULOG_WARN(error.c_str());
					throw new MatchParserException(std::move(error));
				}
				match.setIpv6Dst((char*)value.getString().c_str());
				foundProtocolField = true;
			} else {
				ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IP_SRC,value.getString().c_str());
				if(!AddressValidator::validateIpv4(value.getString()))
				{
					string error = string("Key \"") + IP_SRC + "\" with wrong value \"" + value.getString().c_str() + "\"";
					ULOG_WARN(error.c_str());
					throw new MatchParserException(std::move(error));
				}
				match.setIpv4Dst((char*)value.getString().c_str());
				foundProtocolField = true;
			}
		}
		else if(name == IPv4_DST_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv4_DST_MASK,value.getString().c_str());
			if(!AddressValidator::validateIpv4Netmask(value.getString()))
			{
				string error = string("Key \"") + IPv4_DST_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpv4DstMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == TOS_BITS)
		{
			ULOG_WARN("\"%s\" \"%s\" not available",MATCH,TOS_BITS);

			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,TOS_BITS,value.getString().c_str());

			//XXX: currently, this information is ignored
		}
		else if(name == PORT_SRC)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,PORT_SRC,value.getString().c_str());
			uint32_t transportSrcPort;
			if((sscanf(value.getString().c_str(),"%" SCNd32,&transportSrcPort) != 1) || (transportSrcPort > 65535))
			{
				string error = string("Key \"") + PORT_SRC + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setTransportSrcPort(transportSrcPort & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == SCTP_SRC)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,SCTP_SRC,value.getString().c_str());
			uint32_t transportSrcPort;
			if((sscanf(value.getString().c_str(),"%" SCNd32,&transportSrcPort) != 1) || (transportSrcPort > 65535))
			{
				string error = string("Key \"") + SCTP_SRC + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setTransportSrcPort(transportSrcPort & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == PORT_DST)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,PORT_DST,value.getString().c_str());
			uint32_t transportDstPort;
			if((sscanf(value.getString().c_str(),"%" SCNd32,&transportDstPort) != 1)  || (transportDstPort > 65535))
			{
				string error = string("Key \"") + PORT_DST + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setTransportDstPort(transportDstPort & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == SCTP_DST)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,SCTP_DST,value.getString().c_str());
			uint32_t transportDstPort;
			if((sscanf(value.getString().c_str(),"%" SCNd32,&transportDstPort) != 1)  || (transportDstPort > 65535))
			{
				string error = string("Key \"") + SCTP_DST + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setTransportDstPort(transportDstPort & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == ICMPv4_TYPE)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv4_TYPE,value.getString().c_str());
			uint16_t icmpv4Type;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&icmpv4Type) != 1) || (icmpv4Type > 255) )
			{
				string error = string("Key \"") + ICMPv4_TYPE + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIcmpv4Type(icmpv4Type & 0xFF);
			foundProtocolField = true;
		}
		else if(name == ICMPv4_CODE)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv4_CODE,value.getString().c_str());
			uint16_t icmpv4Code;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&icmpv4Code) != 1) || (icmpv4Code > 255) )
			{
				string error = string("Key \"") + ICMPv4_CODE + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIcmpv4Code(icmpv4Code & 0xFF);
			foundProtocolField = true;
		}
		else if(name == ARP_OPCODE)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ARP_OPCODE,value.getString().c_str());
			uint32_t arpOpCode;
			if((sscanf(value.getString().c_str(),"%" SCNd32,&arpOpCode) != 1) || (arpOpCode > 65535) )
			{
				string error = string("Key \"") + ARP_OPCODE + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setArpOpCode(arpOpCode & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == ARP_SPA)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ARP_SPA,value.getString().c_str());
			//This is an IPv4 adddress
			if(!AddressValidator::validateIpv4(value.getString()))
			{
				string error = string("Key \"") + ARP_SPA + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setArpSpa((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_SPA_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ARP_SPA_MASK,value.getString().c_str());
			//This is an IPv4 mask
			if(!AddressValidator::validateIpv4Netmask(value.getString()))
			{
				string error = string("Key \"") + ARP_SPA_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setArpSpaMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_TPA)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ARP_TPA,value.getString().c_str());
			//This is an IPv4 adddress
			if(!AddressValidator::validateIpv4(value.getString()))
			{
				string error = string("Key \"") + ARP_TPA + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setArpTpa((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_TPA_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ARP_TPA_MASK,value.getString().c_str());
			//This is an IPv4 mask
			if(!AddressValidator::validateIpv4Netmask(value.getString()))
			{
				string error = string("Key \"") + ARP_TPA_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setArpTpaMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_SHA)
		{
			//This is an ethernet address
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ARP_SHA,value.getString().c_str());
			if(!AddressValidator::validateMac(value.getString().c_str()))
			{
				string error = string("Key \"") + ARP_SHA + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setArpSha((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_THA)
		{
			//This is an ethernet address
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ARP_THA,value.getString().c_str());
			if(!AddressValidator::validateMac(value.getString().c_str()))
			{
				string error = string("Key \"") + ARP_THA + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setArpTha((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_SRC_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_SRC_MASK,value.getString().c_str());
			if(!AddressValidator::validateIpv6(value.getString()))
			{
				string error = string("Key \"") + IPv6_SRC_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpv6SrcMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_DST_MASK)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_DST_MASK,value.getString().c_str());
			if(!AddressValidator::validateIpv6(value.getString()))
			{
				string error = string("Key \"") + IPv6_DST_MASK + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpv6DstMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_FLABEL)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_FLABEL,value.getString().c_str());
			uint64_t ipv6FLabel;
			if((sscanf(value.getString().c_str(),"%" SCNd64,&ipv6FLabel) != 1) || (ipv6FLabel > 4294967295UL) )
			{
				string error = string("Key \"") + IPv6_FLABEL + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpv6Flabel(ipv6FLabel & 0xFFFFFFFF);
			foundProtocolField = true;
		}
		else if(name == IPv6_ND_TARGET)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_ND_TARGET,value.getString().c_str());
			//FIXME: validate it?
			match.setIpv6NdTarget((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_ND_SLL)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_ND_SLL,value.getString().c_str());
			//FIXME: validate it?
			match.setIpv6NdSll((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_ND_TLL)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_ND_TLL,value.getString().c_str());
			//FIXME: validate it?
			match.setIpv6NdTll((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ICMPv6_TYPE)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv6_TYPE,value.getString().c_str());
			uint16_t icmpv6Type;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&icmpv6Type) != 1) || (icmpv6Type > 255) )
			{
				string error = string("Key \"") + ICMPv6_TYPE + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIcmpv6Type(icmpv6Type & 0xFF);
			foundProtocolField = true;
		}
		else if(name == ICMPv6_CODE)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv6_CODE,value.getString().c_str());
			uint16_t icmpv6Code;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&icmpv6Code) != 1) || (icmpv6Code > 255) )
			{
				string error = string("Key \"") + ICMPv6_CODE + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIcmpv6Code(icmpv6Code & 0xFF);
			foundProtocolField = true;
		}
		else if(name == MPLS_LABEL)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,MPLS_LABEL,value.getString().c_str());
			uint64_t mplsLabel;
			if((sscanf(value.getString().c_str(),"%" SCNd64,&mplsLabel) != 1) || (mplsLabel > 1048575) )
			{
				string error = string("Key \"") + MPLS_LABEL + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setMplsLabel(mplsLabel & 0xFFFFFFFF);
			foundProtocolField = true;
		}
		else if(name == MPLS_TC)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,MPLS_TC,value.getString().c_str());
			uint16_t mplsTC;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&mplsTC) != 1) || (mplsTC > 255) )
			{
				string error = string("Key \"") + MPLS_TC + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setMplsTC(mplsTC & 0xFF);
			foundProtocolField = true;
		}
		else if(name == PROTOCOL)
		{
			ULOG_DBG("\"%s\"->\"%s\": \"%s\"",MATCH,PROTOCOL,value.getString().c_str());
			uint16_t ipProto;
			if((sscanf(value.getString().c_str(),"%" SCNd16,&ipProto) != 1) || (ipProto > 255) )
			{
				string error = string("Key \"") + PROTOCOL + "\" with wrong value \"" + value.getString().c_str() + "\"";
				ULOG_WARN(error.c_str());
				throw new MatchParserException(std::move(error));
			}
			match.setIpProto(ipProto & 0xFF);
			foundProtocolField = true;
		}
		else
		{
			string error = string("Invalid key: ") + name.c_str();
			ULOG_WARN(error.c_str());
			throw new MatchParserException(std::move(error));
		}
	}

	if(!foundOne)
	{
		string error = string("Neither Key \"") + PORT + "\", nor key \"" + _ID+ "\" found in \"" + MATCH + "\"";
		ULOG_WARN(error.c_str());
		throw new MatchParserException(std::move(error));
	}

	if(foundProtocolField && foundEndPointID && definedInCurrentGraph)
	{
		string error = string("A \"") + MATCH + "\" specifying an \"" + ENDPOINT + "\" (defined in the current graph) and at least a protocol field was found. This is not supported.";
		ULOG_WARN(error.c_str());
		throw new MatchParserException(std::move(error));
	}

	return;
}


