#include "low_level_match.h"

static const char LOG_MODULE_NAME[] = "Low-Level-Match";

namespace lowlevel
{

Match::Match() :
	graph::Match(),isInput_port(false),is_local_port(false)
{

}

Match::Match(bool is_local_port) :
	graph::Match(),isInput_port(false), is_local_port(is_local_port)
{

}

bool Match::operator==(const Match &other) const
{
	ULOG_DBG("Matches to be compared:");
	ULOG_DBG("\tisInputPort %s vs %s ",(isInput_port)?"yes":"no",(other.isInput_port)?"yes":"no");
	ULOG_DBG("\tinput port: %d vs %d",input_port,other.input_port);

	if((isInput_port && !other.isInput_port) ||
		(!isInput_port && other.isInput_port) )
		return false;

	if(input_port != other.input_port)
		return false;

	ULOG_DBG("Comparing other fields...");

	return this->isEqual(other);
}

void Match::fillFlowmodMessage(rofl::openflow::cofflowmod &message)
{
	if(isInput_port)
		message.set_match().set_in_port(input_port);
	if(is_local_port)
		message.set_match().set_in_port(rofl::openflow::OFPP_LOCAL);

	if(eth_src != NULL)
	{
		if(eth_src_mask)
			message.set_match().set_eth_src(cmacaddr(eth_src), cmacaddr(eth_src_mask));
		else
			message.set_match().set_eth_src(cmacaddr(eth_src));
	}

	if(eth_dst != NULL)
	{
		if(eth_dst_mask)
			message.set_match().set_eth_dst(cmacaddr(eth_dst), cmacaddr(eth_dst_mask));
		else
			message.set_match().set_eth_dst(cmacaddr(eth_dst));
	}
	if(isEthType)
		message.set_match().set_eth_type(ethType);
	if(isVlanID || isEndpointVlanID)
	{
		/**
		*	This field is vlan_tci and, according to the documentation provided at
		*		http://openvswitch.org/support/dist-docs/ovs-ofctl.8.txt
		*	requires that the VLAN ID is put in OR with 0x1000.
		*	Thanks to Roberto Bonafiglia for his help :D !
		*/
		message.set_match().set_vlan_vid(vlanID | 0x1000 );
	}
	else if(isAnyVlan)
		message.set_match().set_vlan_present();
	else if(isNoVlan)
		message.set_match().set_vlan_untagged();
	if(isVlanPCP)
		message.set_match().set_vlan_pcp(vlanPCP);
	if(isIpDSCP)
		message.set_match().set_ip_dscp(ipDSCP);
	if(isIpECN)
		message.set_match().set_ip_ecn(ipECN);
	if(isIpProto)
		message.set_match().set_ip_proto(ipProto);
	if(ipv4_src)
	{
		if(ipv4_src_mask)
			message.set_match().set_ipv4_src(caddress_in4(ipv4_src),caddress_in4(ipv4_src_mask));
		else
			message.set_match().set_ipv4_src(caddress_in4(ipv4_src));
	}
	if(ipv4_dst)
	{
		if(ipv4_dst_mask)
			message.set_match().set_ipv4_dst(caddress_in4(ipv4_dst),caddress_in4(ipv4_dst_mask));
		else
			message.set_match().set_ipv4_dst(caddress_in4(ipv4_dst));
	}
	if(isTcpSrc)
		message.set_match().set_tcp_src(tcp_src);
	if(isTcpDst)
		message.set_match().set_tcp_dst(tcp_dst);
	if(isUdpSrc)
		message.set_match().set_udp_src(udp_src);
	if(isUdpDst)
		message.set_match().set_udp_dst(udp_dst);
	if(isSctpSrc)
		message.set_match().set_sctp_src(sctp_src);
	if(isSctpDst)
		message.set_match().set_sctp_dst(sctp_dst);
	if(isIcmpv4Type)
		message.set_match().set_icmpv4_type(icmpv4Type);
	if(isIcmpv4Code)
		message.set_match().set_icmpv4_code(icmpv4Code);
	if(isArpOpcode)
		message.set_match().set_arp_opcode(arpOpcode);
	if(arp_spa)
	{
		if(arp_spa_mask)
			message.set_match().set_arp_spa(caddress_in4(arp_spa),caddress_in4(arp_spa_mask));
		else
			message.set_match().set_arp_spa(caddress_in4(arp_spa));
	}
	if(arp_tpa)
	{
		if(arp_tpa_mask)
			message.set_match().set_arp_tpa(caddress_in4(arp_tpa),caddress_in4(arp_tpa_mask));
		else
			message.set_match().set_arp_tpa(caddress_in4(arp_tpa));
	}
	if(arp_sha)
		message.set_match().set_arp_sha(cmacaddr(arp_sha));
	if(arp_tha)
		message.set_match().set_arp_tha(cmacaddr(arp_tha));
	if(ipv6_src)
	{
		if(ipv6_src_mask)
			message.set_match().set_ipv6_src(caddress_in6(ipv6_src),caddress_in6(ipv6_src_mask));
		else
			message.set_match().set_ipv6_src(caddress_in6(ipv6_src));
	}
	if(ipv6_dst)
	{
		if(ipv6_dst_mask)
			message.set_match().set_ipv6_dst(caddress_in6(ipv6_dst),caddress_in6(ipv6_dst_mask));
		else
			message.set_match().set_ipv6_dst(caddress_in6(ipv6_dst));
	}
	if(isIpv6Flabel)
		message.set_match().set_ipv6_flabel(ipv6_flabel);
	if(isIcmpv6Type)
		message.set_match().set_icmpv6_type(icmpv6Type);
	if(isIcmpv6Code)
		message.set_match().set_icmpv6_code(isIcmpv6Code);
	if(ipv6_nd_target)
		message.set_match().set_ipv6_nd_target(caddress_in6(ipv6_nd_target));
	if(ipv6_nd_sll)
		message.set_match().set_ipv6_nd_sll(cmacaddr(ipv6_nd_sll));
	if(ipv6_nd_tll)
		message.set_match().set_ipv6_nd_tll(cmacaddr(ipv6_nd_tll));
	if(isMplsLabel)
		message.set_match().set_mpls_label(mplsLabel);
	if(isMplsTC)
		message.set_match().set_mpls_tc(mplsTC);
	if(gre_key)
	{
		unsigned int key = 0;

		if(sscanf(gre_key, "%x", &key)!=1)
			sscanf(gre_key, "%u", &key);
		message.set_match().set_tunnel_id(key);
	}
}

void Match::setInputPort(unsigned int input_port)
{
	this->input_port = input_port;
	isInput_port = true;
}

void Match::setAllCommonFields(graph::Match match)
{
	graph::Match::setAllCommonFields(match);
}

void Match::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "\t\tmatch:" << endl << "\t\t{" << endl;
		if(is_local_port)
			cout << "\t\t\tport: "<< "LOCAL" << endl;
		else
			cout << "\t\t\tport: "<< input_port << endl;

		/*
		*	Ethernet
		*/
		if(eth_src != NULL)
			cout << "\t\t\tethernet src: " << eth_src << endl;
		if(eth_src_mask)
			cout << "\t\t\tethernet src mask: " << eth_src_mask << endl;
		if(eth_dst != NULL)
			cout << "\t\t\tethernet dst: " << eth_dst << endl;
		if(eth_dst_mask)
			cout << "\t\t\tethernet dst mask: " << eth_dst_mask << endl;
		if(isEthType)
			cout << "\t\t\tethertype: " <<  "0x" << hex << ethType << endl;

		/*
		*	VLAN
		*/
		if(isVlanID || isEndpointVlanID)
			cout << "\t\t\tVLAN ID: " << hex << "0x" << vlanID << endl;
		else if(isAnyVlan)
			cout << "\t\t\tVLAN ID: ANY" << endl;
		else if(isNoVlan)
			cout << "\t\t\tNO VLAN" << endl;

		if(isVlanPCP)
		{
			cout << "\t\t\tVLAN PCP: " << int(vlanPCP) << endl;
		}

		/*
		*	IPv4
		*/
		if(isIpDSCP)
			cout << "\t\t\tIPv4 dscp: " << int(ipDSCP) << endl;
		if(isIpECN)
			cout << "\t\t\tIPv4 ecn: " << int(ipECN) << endl;
		if(isIpProto)
			cout << "\t\t\tIPv4 proto: " << (ipProto & 0xF) << endl;
		if(ipv4_src)
			cout << "\t\t\tIPv4 src: " << ipv4_src << endl;
		if(ipv4_src_mask)
			cout << "\t\t\tIPv4 src mask: " << ipv4_src_mask << endl;
		if(ipv4_dst)
			cout << "\t\t\tIPv4 dst: " << ipv4_dst << endl;
		if(ipv4_dst_mask)
			cout << "\t\t\tIPv4 dst mask: " << ipv4_dst_mask << endl;

		/*
		*	TCP
		*/
		if(isTcpSrc)
			cout << "\t\t\tTCP src port: " << tcp_src << endl;
		if(isTcpDst)
			cout << "\t\t\tTCP dst port: " << tcp_dst << endl;

		/*
		*	UDP
		*/
		if(isUdpSrc)
			cout << "\t\t\tUDP src port: " << udp_src << endl;
		if(isUdpDst)
			cout << "\t\t\tUDP dst port: " << udp_dst << endl;

		/*
		*	SCTP
		*/
		if(isSctpSrc)
			cout << "\t\t\tSCTP src port: " << sctp_src << endl;
		if(isSctpDst)
			cout << "\t\t\tSCTP dst port: " << sctp_dst << endl;

		/*
		*	ICMPv4
		*/
		if(isIcmpv4Type)
			cout << "\t\t\tICMPv4 type: " << int(icmpv4Type) << endl;
		if(isIcmpv4Code)
			cout << "\t\t\tICMPv4 code: " << int(icmpv4Code) << endl;

		/*
		*	ARP
		*/
		if(isArpOpcode)
			cout << "\t\t\tARP opcode: " << arpOpcode << endl;
		if(arp_spa)
			cout << "\t\t\tARP spa: " << arp_spa << endl;
		if(arp_spa_mask)
		 	cout << "\t\t\tARP spa mask: " << arp_spa_mask << endl;
		if(arp_tpa)
			cout << "\t\t\tARP tpa: " << arp_tpa << endl;
		if(arp_tpa_mask)
			cout << "\t\t\tARP tpa mask: " << arp_tpa_mask << endl;
		if(arp_sha)
			cout << "\t\t\tARP sha: " << arp_sha << endl;
		if(arp_tha)
			cout << "\t\t\tARP tha: " << arp_tha << endl;

		/*
		*	IPv6
		*/
		if(ipv6_src)
			cout << "\t\t\tIPv6 src: " << ipv6_src << endl;
		if(ipv6_src_mask)
			cout << "\t\t\tIPv6 src mask: " << ipv6_src_mask << endl;
		if(ipv6_dst)
			cout << "\t\t\tIPv6 dst: " << ipv6_dst << endl;
		if(ipv6_dst_mask)
			cout << "\t\t\tIPv6 dst mask: " << ipv6_dst_mask << endl;
		if(isIpv6Flabel)
			cout << "\t\t\tIPv6 flabel: " << ipv6_flabel << endl;
		if(ipv6_nd_target)
			 cout << "\t\t\tIPv6 nd target: " << ipv6_nd_target << endl;
		if(ipv6_nd_sll)
			 cout << "\t\t\tIPv6 nd sll: " << ipv6_nd_sll << endl;
		if(ipv6_nd_tll)
			cout << "\t\t\tIPv6 nd tll: " << ipv6_nd_tll << endl;

		/*
		*	ICMPv6
		*/
		if(isIcmpv6Type)
			cout << "\t\t\tICMPv6 type: "<<  int(icmpv6Type) << endl;
		if(isIcmpv6Code)
			cout << "\t\t\tICMPv6 code: " << int(icmpv6Code) << endl;

		/*
		*	MPLS
		*/
		if(isMplsLabel)
			cout << "\t\t\tMPLS label: " << mplsLabel << endl;
		if(isMplsTC)
			cout << "\t\t\tMPLS tc: " << int(mplsTC) << endl;

		/*
		*	GRE
		*/
		if(gre_key)
			cout << "\t\t\tGRE key: " << gre_key << endl;

		cout << "\t\t}" << endl;
	}
}

string Match::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	stringstream ss;

	ss << "port: ";

	map<string,unsigned int> pysicalPorts = lsi0->getPhysicalPorts();
	for(map<string,unsigned int>::iterator it = pysicalPorts.begin(); it != pysicalPorts.end(); it++)
	{
		if(it->second == input_port)
		{
			ss << it->first << graph::Match::prettyPrint();
			return ss.str();
		}
	}

	//The port corresponds to a virtual link... we search the corresponding graph

	for(map<string,LSI *>::iterator it = lsis.begin(); it != lsis.end(); it++)
	{
		vector<VLink> vlinks = it->second->getVirtualLinks();
		for(vector<VLink>::iterator vl = vlinks.begin(); vl != vlinks.end(); vl++)
		{
			if(vl->getRemoteID() == input_port)
			{
				ss << input_port << " (graph: " << it->first << ")";
				return ss.str();
			}
		}
	}

	if(is_local_port)
		ss << "LOCAL" << " (LOCAL graph)";
	else
	{
		//The code could be here only when a SIGINT is received and all the graph are going to be removed
		ss << input_port << " (unknown graph)";
	}

	return ss.str();
}

}
