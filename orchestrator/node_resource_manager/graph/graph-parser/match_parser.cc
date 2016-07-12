#include "match_parser.h"

string MatchParser::graphID(string name_port)
{
	return nfId(name_port);
}

string MatchParser::nfId(string id_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,id_port.c_str());
	char *pnt=strtok(tmp, delimiter);
	while( pnt!= NULL )
	{
		return string(pnt);
	}

	return "";
}

unsigned int MatchParser::nfPort(string name_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,name_port.c_str());
	char *pnt=strtok((char*)/*name_port.c_str()*/tmp, delimiter);
	unsigned int port = 0;

	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 1:
				sscanf(pnt,"%u",&port);
				return port;
			break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return port;
}

bool MatchParser::nfIsPort(string name_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,name_port.c_str());
	char *pnt=strtok((char*)/*name_port.c_str()*/tmp, delimiter);
	unsigned int port = 0;

	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 1:
				sscanf(pnt,"%u",&port);
				return true;
			break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return false;
}

string MatchParser::epName(string name_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,name_port.c_str());
	char *pnt=strtok((char*)/*name_port.c_str()*/tmp, delimiter);

	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 1:
				return pnt;
			break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return "";
}

unsigned int MatchParser::epPort(string name_port)
{
	return nfPort(name_port);
}

/**
*	http://stackoverflow.com/questions/4792035/how-do-you-validate-that-a-string-is-a-valid-mac-address-in-c
*/
bool MatchParser::validateMac(const char* mac)
{
	int i = 0;
	int s = 0;

	while (*mac)
	{
		if (isxdigit(*mac))
			i++;
		else if (*mac == ':' || *mac == '-')
		{
			if (i == 0 || i / 2 - 1 != s)
			break;

			++s;
		}
		else
			s = -1;

		++mac;
	}

    return (i == 12 && (s == 5 || s == 0));
}

/**
*	http://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
*/
bool MatchParser::validateIpv4(const string &ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
    return result != 0;
}

bool MatchParser::validateIpv6(const string &ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET6, ipAddress.c_str(), &(sa.sin_addr));
    return result != 0;
}

bool MatchParser::validateIpv4Netmask(const string &netmask)
{
	if(!validateIpv4(netmask))
		return false;

	bool zero = true;
	unsigned int mask;

	int first, second, third, fourth;
	sscanf(netmask.c_str(),"%d.%d.%d.%d",&first,&second,&third,&fourth);
	mask = (first << 24) + (second << 16) + (third << 8) + fourth;

	for(int i = 0; i < 32; i++)
	{
		if(((mask & 0x1) == 0) && !zero)
			return false;
		if(((mask & 0x1) == 1) && zero)
			zero = false;

		mask = mask >> 1;
	}

	return true;
}

bool MatchParser::parseMatch(Object object, highlevel::Match &match, highlevel::Action &action, map<string,string > &iface_id, map<string,string> &internal_id, map<string,pair<string,string> > &vlan_id, map<string,string> &gre_id, highlevel::Graph &graph)
{
	bool foundOne = false;
	bool foundEndPointID = false, foundProtocolField = false, definedInCurrentGraph = false;
	bool is_tcp = false;
	enum port_type { VNF_PORT_TYPE, EP_PORT_TYPE, EP_INTERNAL_TYPE };

	for(Object::const_iterator i = object.begin(); i != object.end(); i++)
	{
		const string& name  = i->first;
		const Value&  value = i->second;

		if(name == PORT_IN)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,PORT_IN,value.getString().c_str());
			if(foundOne)
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Only one between keys \"%s\", \"%s\" and \"%s\" are allowed in \"%s\"",PORT_IN,VNF,ENDPOINT,MATCH);
				return false;
			}

			foundOne = true;

			string internal_group;
			string port_in_name = value.getString();
			string realName;
			string v_id;
			string graph_id;
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
					case 3:
						if(p_type == VNF_PORT_TYPE)
						{
							strcat(vnf_id_tmp,pnt);
						}
				}

				pnt = strtok( NULL, delimiter );
				i++;
			}

			//VNFs port type
			if(p_type == VNF_PORT_TYPE)
			{
				//convert char *vnf_id_tmp to string vnf_name
				string vnf_name(vnf_id_tmp, strlen(vnf_id_tmp));

				string nf_name = nfId(vnf_name);
				char *tmp_vnf_name = new char[BUFFER_SIZE];
				strcpy(tmp_vnf_name, (char *)vnf_name.c_str());
				unsigned int port = nfPort(string(tmp_vnf_name));
				bool is_port = nfIsPort(string(tmp_vnf_name));

				if(nf_name == "" || !is_port)
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Network function \"%s\" is not valid. It must be in the form \"name:port\"",vnf_id_tmp);
					return false;
				}
				/*nf port starts from 0 - here we want that the ID starts from 1*/
				port++;

				match.setNFport(nf_name,port);

			}
			//end-points port type
			else if(p_type == EP_PORT_TYPE)
			{
				bool iface_found = false, internal_found = false, vlan_found = false, gre_found=false;
				char *s_value = new char[BUFFER_SIZE];
				strcpy(s_value, (char *)value.getString().c_str());
				string eP = epName(value.getString());
				if(eP != ""){
					map<string,string>::iterator it = iface_id.find(eP);
					map<string,string>::iterator it1 = internal_id.find(eP);
					map<string,pair<string,string> >::iterator it2 = vlan_id.find(eP);
					map<string,string>::iterator it3 = gre_id.find(eP);
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
						logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Internal endpoint \"%s\" is not valid. It must have the \"%s\" attribute",value.getString().c_str(), INTERNAL_GROUP);
						return false;
					}

					match.setEndPointInternal(/*graph_id,endPoint*/internal_group);
				}
				/*vlan endpoint*/
				else if(vlan_found)
				{
					uint32_t vlanID = 0;
					vlan_action_t actionType = ACTION_VLAN_POP;

					if((sscanf(v_id.c_str(),"%"SCNd32,&vlanID) != 1) && (vlanID > 4094))
					{
						logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",VLAN_ID,value.getString().c_str());
						return false;
					}

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
			}
		}
		else if(name == HARD_TIMEOUT)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "\"%s\" \"%s\" not available",MATCH,HARD_TIMEOUT);

			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,HARD_TIMEOUT,value.getString().c_str());

			//XXX: currently, this information is ignored
		}
		else if(name == ETH_TYPE)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ETH_TYPE,value.getString().c_str());
			uint32_t ethType;
			if((sscanf(value.getString().c_str(),"%"SCNi32,&ethType) != 1) || (ethType > 65535))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ETH_TYPE,value.getString().c_str());
				return false;
			}
			match.setEthType(ethType & 0xFFFF);
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ETH_TYPE,value.getString().c_str(),ethType);
			foundProtocolField = true;
		}
		else if(name == VLAN_ID)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,VLAN_ID,value.getString().c_str());
			if(value.getString() == ANY_VLAN)
				match.setVlanIDAnyVlan();
			else if(value.getString() == NO_VLAN)
				match.setVlanIDNoVlan();
			else
			{
				uint32_t vlanID;
				if((sscanf(value.getString().c_str(),"%"SCNi32,&vlanID) != 1) && (vlanID > 4094))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",VLAN_ID,value.getString().c_str());
					return false;
				}
				match.setVlanID(vlanID & 0xFFFF);
			}
			foundProtocolField = true;
		}
		else if(name == VLAN_PRIORITY)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "\"%s\" \"%s\" not available",MATCH,VLAN_PRIORITY);

			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,VLAN_PRIORITY,value.getString().c_str());

			//XXX: currently, this information is ignored
		}
		else if(name == ETH_SRC)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ETH_SRC,value.getString().c_str());
			if(!validateMac(value.getString().c_str()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ETH_SRC,value.getString().c_str());
				return false;
			}
			match.setEthSrc((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ETH_SRC_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ETH_SRC_MASK,value.getString().c_str());
			if(!validateMac(value.getString().c_str()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ETH_SRC_MASK,value.getString().c_str());
				return false;
			}
			match.setEthSrcMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ETH_DST)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ETH_DST,value.getString().c_str());
			if(!validateMac(value.getString().c_str()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ETH_DST,value.getString().c_str());
				return false;
			}
			match.setEthDst((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ETH_DST_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ETH_DST_MASK,value.getString().c_str());
			if(!validateMac(value.getString().c_str()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ETH_DST_MASK,value.getString().c_str());
				return false;
			}
			match.setEthDstMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == VLAN_PCP)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,VLAN_PCP,value.getString().c_str());
			uint16_t vlanPCP;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&vlanPCP) != 1) || (vlanPCP > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",VLAN_PCP,value.getString().c_str());
				return false;
			}
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%x\"",MATCH,VLAN_PCP,vlanPCP);
			match.setVlanPCP(vlanPCP & 0xFF);
			foundProtocolField = true;
		}
		else if(name == IP_DSCP)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IP_DSCP,value.getString().c_str());
			uint16_t ipDSCP;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&ipDSCP) != 1) || (ipDSCP > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IP_DSCP,value.getString().c_str());
				return false;
			}
			match.setIpDSCP(ipDSCP & 0xFF);
		}
		else if(name == IP_ECN)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IP_ECN,value.getString().c_str());
			uint16_t ipECN;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&ipECN) != 1) || (ipECN > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IP_ECN,value.getString().c_str());
				return false;
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
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IP_SRC,value.getString().c_str());
				if(!validateIpv6(value.getString()))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IP_SRC,value.getString().c_str());
					return false;
				}
				match.setIpv6Src((char*)value.getString().c_str());
				foundProtocolField = true;
			//IPv4
			} else {
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IP_SRC,value.getString().c_str());
				if(!validateIpv4(value.getString()))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IP_SRC,value.getString().c_str());
					return false;
				}
				match.setIpv4Src((char*)value.getString().c_str());
				foundProtocolField = true;
			}
		}
		else if(name == IPv4_SRC_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv4_SRC_MASK,value.getString().c_str());
			if(!validateIpv4Netmask(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IPv4_SRC_MASK,value.getString().c_str());
				return false;
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
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IP_DST,value.getString().c_str());
				if(!validateIpv6(value.getString()))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IP_DST,value.getString().c_str());
					return false;
				}
				match.setIpv6Dst((char*)value.getString().c_str());
				foundProtocolField = true;
			} else {
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IP_SRC,value.getString().c_str());
				if(!validateIpv4(value.getString()))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IP_SRC,value.getString().c_str());
					return false;
				}
				match.setIpv4Dst((char*)value.getString().c_str());
				foundProtocolField = true;
			}
		}
		else if(name == IPv4_DST_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv4_DST_MASK,value.getString().c_str());
			if(!validateIpv4Netmask(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IPv4_DST_MASK,value.getString().c_str());
				return false;
			}
			match.setIpv4DstMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == TOS_BITS)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "\"%s\" \"%s\" not available",MATCH,TOS_BITS);

			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,TOS_BITS,value.getString().c_str());

			//XXX: currently, this information is ignored
		}
		else if(name == PORT_SRC)
		{
			//TCP
			if(is_tcp)
			{
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,PORT_SRC,value.getString().c_str());
				uint32_t tcpSrc;
				if((sscanf(value.getString().c_str(),"%"SCNd32,&tcpSrc) != 1) || (tcpSrc > 65535))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",PORT_SRC,value.getString().c_str());
					return false;
				}
				match.setTcpSrc(tcpSrc & 0xFFFF);
				foundProtocolField = true;
			//UDP
			} else {
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,PORT_SRC,value.getString().c_str());
				uint32_t udpSrc;
				if((sscanf(value.getString().c_str(),"%"SCNd32,&udpSrc) != 1)  || (udpSrc > 65535))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",PORT_SRC,value.getString().c_str());
					return false;
				}
				match.setUdpSrc(udpSrc & 0xFFFF);
				foundProtocolField = true;
			}
		}
		else if(name == PORT_DST)
		{
			//TCP
			if(is_tcp)
			{
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,PORT_DST,value.getString().c_str());
				uint32_t tcpDst;
				if((sscanf(value.getString().c_str(),"%"SCNd32,&tcpDst) != 1)  || (tcpDst > 65535))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",PORT_DST,value.getString().c_str());
					return false;
				}
				match.setTcpDst(tcpDst & 0xFFFF);
				foundProtocolField = true;
			//UDP
			} else {
				logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,PORT_DST,value.getString().c_str());
				uint32_t udpDst;
				if((sscanf(value.getString().c_str(),"%"SCNd32,&udpDst) != 1)  || (udpDst > 65535))
				{
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",PORT_DST,value.getString().c_str());
					return false;
				}
				match.setUdpDst(udpDst & 0xFFFF);
				foundProtocolField = true;
			}
		}
		else if(name == SCTP_SRC)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,SCTP_SRC,value.getString().c_str());
			uint32_t sctpSrc;
			if((sscanf(value.getString().c_str(),"%"SCNd32,&sctpSrc) != 1)  || (sctpSrc > 65535))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",SCTP_SRC,value.getString().c_str());
				return false;
			}
			match.setSctpSrc(sctpSrc & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == SCTP_DST)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,SCTP_DST,value.getString().c_str());
			uint32_t sctpDst;
			if((sscanf(value.getString().c_str(),"%"SCNd32,&sctpDst) != 1)  || (sctpDst > 65535))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",SCTP_DST,value.getString().c_str());
				return false;
			}
			match.setSctpDst(sctpDst & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == ICMPv4_TYPE)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv4_TYPE,value.getString().c_str());
			uint16_t icmpv4Type;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&icmpv4Type) != 1) || (icmpv4Type > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ICMPv4_TYPE,value.getString().c_str());
				return false;
			}
			match.setIcmpv4Type(icmpv4Type & 0xFF);
			foundProtocolField = true;
		}
		else if(name == ICMPv4_CODE)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv4_CODE,value.getString().c_str());
			uint16_t icmpv4Code;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&icmpv4Code) != 1) || (icmpv4Code > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ICMPv4_CODE,value.getString().c_str());
				return false;
			}
			match.setIcmpv4Code(icmpv4Code & 0xFF);
			foundProtocolField = true;
		}
		else if(name == ARP_OPCODE)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ARP_OPCODE,value.getString().c_str());
			uint32_t arpOpCode;
			if((sscanf(value.getString().c_str(),"%"SCNd32,&arpOpCode) != 1) || (arpOpCode > 65535) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_OPCODE,value.getString().c_str());
				return false;
			}
			match.setArpOpCode(arpOpCode & 0xFFFF);
			foundProtocolField = true;
		}
		else if(name == ARP_SPA)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ARP_SPA,value.getString().c_str());
			//This is an IPv4 adddress
			if(!validateIpv4(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_SPA,value.getString().c_str());
				return false;
			}
			match.setArpSpa((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_SPA_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ARP_SPA_MASK,value.getString().c_str());
			//This is an IPv4 mask
			if(!validateIpv4Netmask(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_SPA_MASK,value.getString().c_str());
				return false;
			}
			match.setArpSpaMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_TPA)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ARP_TPA,value.getString().c_str());
			//This is an IPv4 adddress
			if(!validateIpv4(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_TPA,value.getString().c_str());
				return false;
			}
			match.setArpTpa((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_TPA_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ARP_TPA_MASK,value.getString().c_str());
			//This is an IPv4 mask
			if(!validateIpv4Netmask(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_TPA_MASK,value.getString().c_str());
				return false;
			}
			match.setArpTpaMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_SHA)
		{
			//This is an ethernet address
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ARP_SHA,value.getString().c_str());
			if(!validateMac(value.getString().c_str()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_SHA,value.getString().c_str());
				return false;
			}
			match.setArpSha((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ARP_THA)
		{
			//This is an ethernet address
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ARP_THA,value.getString().c_str());
			if(!validateMac(value.getString().c_str()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_THA,value.getString().c_str());
				return false;
			}
			match.setArpTha((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_SRC_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_SRC_MASK,value.getString().c_str());
			if(!validateIpv6(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IPv6_SRC_MASK,value.getString().c_str());
				return false;
			}
			match.setIpv6SrcMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_DST_MASK)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_DST_MASK,value.getString().c_str());
			if(!validateIpv6(value.getString()))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",IPv6_DST_MASK,value.getString().c_str());
				return false;
			}
			match.setIpv6DstMask((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_FLABEL)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_FLABEL,value.getString().c_str());
			uint64_t ipv6FLabel;
			if((sscanf(value.getString().c_str(),"%"SCNd64,&ipv6FLabel) != 1) || (ipv6FLabel > 4294967295UL) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ARP_OPCODE,value.getString().c_str());
				return false;
			}
			match.setIpv6Flabel(ipv6FLabel & 0xFFFFFFFF);
			foundProtocolField = true;
		}
		else if(name == IPv6_ND_TARGET)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_ND_TARGET,value.getString().c_str());
			//FIXME: validate it?
			match.setIpv6NdTarget((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_ND_SLL)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_ND_SLL,value.getString().c_str());
			//FIXME: validate it?
			match.setIpv6NdSll((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == IPv6_ND_TLL)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,IPv6_ND_TLL,value.getString().c_str());
			//FIXME: validate it?
			match.setIpv6NdTll((char*)value.getString().c_str());
			foundProtocolField = true;
		}
		else if(name == ICMPv6_TYPE)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv6_TYPE,value.getString().c_str());
			uint16_t icmpv6Type;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&icmpv6Type) != 1) || (icmpv6Type > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ICMPv6_TYPE,value.getString().c_str());
				return false;
			}
			match.setIcmpv6Type(icmpv6Type & 0xFF);
			foundProtocolField = true;
		}
		else if(name == ICMPv6_CODE)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,ICMPv6_CODE,value.getString().c_str());
			uint16_t icmpv6Code;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&icmpv6Code) != 1) || (icmpv6Code > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",ICMPv6_CODE,value.getString().c_str());
				return false;
			}
			match.setIcmpv6Code(icmpv6Code & 0xFF);
			foundProtocolField = true;
		}
		else if(name == MPLS_LABEL)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,MPLS_LABEL,value.getString().c_str());
			uint64_t mplsLabel;
			if((sscanf(value.getString().c_str(),"%"SCNd64,&mplsLabel) != 1) || (mplsLabel > 1048575) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",MPLS_LABEL,value.getString().c_str());
				return false;
			}
			match.setMplsLabel(mplsLabel & 0xFFFFFFFF);
			foundProtocolField = true;
		}
		else if(name == MPLS_TC)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,MPLS_TC,value.getString().c_str());
			uint16_t mplsTC;
			if((sscanf(value.getString().c_str(),"%"SCNd16,&mplsTC) != 1) || (mplsTC > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",MPLS_TC,value.getString().c_str());
				return false;
			}
			match.setMplsTC(mplsTC & 0xFF);
			foundProtocolField = true;
		}
		else if(name == PROTOCOL)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\"%s\"->\"%s\": \"%s\"",MATCH,PROTOCOL,value.getString().c_str());
			uint16_t ipProto;

			if(value.getString().compare("0x06") == 0)
				is_tcp = true;
			else
				is_tcp = false;

			if((sscanf(value.getString().c_str(),"%"SCNd16,&ipProto) != 1) || (ipProto > 255) )
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Key \"%s\" with wrong value \"%s\"",PROTOCOL,value.getString().c_str());
				return false;
			}
			match.setIpProto(ipProto & 0xFF);
			foundProtocolField = true;
		}
		else
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid key: %s",name.c_str());
			return false;
		}
	}

	if(!foundOne)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Neither Key \"%s\", nor key \"%s\" found in \"%s\"",PORT,_ID,MATCH);
		return false;
	}

	if(foundProtocolField && foundEndPointID && definedInCurrentGraph)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "A \"%s\" specifying an \"%s\" (defined in the current graph) and at least a protocol field was found. This is not supported.",MATCH,ENDPOINT);
		return false;
	}

	return true;
}


