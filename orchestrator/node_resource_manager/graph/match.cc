#include "match.h"

namespace graph
{

Match::Match() :
	eth_src(NULL), eth_src_mask(NULL), eth_dst(NULL), eth_dst_mask(NULL), isEthType(false),
	isVlanID(false),isNoVlan(false),isAnyVlan(false),isVlanPCP(false),isEndpointVlanID(false),
	isIpDSCP(false),isIpECN(false),isIpProto(false), ipv4_src(NULL), ipv4_src_mask(NULL),
	ipv4_dst(NULL), ipv4_dst_mask(NULL), isTcpProtocol(false), isUdpProtocol(false),
	isSctpProtocol(false), transport_src_port(0), transport_dst_port(0), isIcmpv4Type(false), isIcmpv4Code(false),
	isArpOpcode(false), arp_spa(NULL),arp_spa_mask(NULL), arp_tpa(NULL), arp_tpa_mask(NULL), arp_sha(NULL), arp_tha(NULL),
	ipv6_src(NULL), ipv6_src_mask(NULL), ipv6_dst(NULL), ipv6_dst_mask(NULL), isIpv6Flabel(false),
	ipv6_flabel(false),ipv6_nd_target(NULL), ipv6_nd_sll(NULL), ipv6_nd_tll(NULL),
	isIcmpv6Type(false), isIcmpv6Code(false),
	isMplsLabel(false), mplsLabel(false), isMplsTC(false), mplsTC(false),
	gre_key(NULL)
{

}

bool Match::isEqual(const Match &other) const
{
	/*
	*	Ethernet
	*/
	if((eth_src == NULL && other.eth_src != NULL) ||
		(eth_src != NULL && other.eth_src == NULL))
		return false;

	if(eth_src != NULL && other.eth_src != NULL)
	{
		if(strcmp(eth_src,other.eth_src) != 0)
			return false;
	}

	if((eth_src_mask == NULL && other.eth_src_mask != NULL) ||
		(eth_src_mask != NULL && other.eth_src_mask == NULL))
		return false;

	if(eth_src_mask != NULL && other.eth_src_mask != NULL)
	{
		if(strcmp(eth_src_mask,other.eth_src_mask) != 0)
			return false;
	}

	if((eth_dst == NULL && other.eth_dst != NULL) ||
		(eth_dst != NULL && other.eth_dst == NULL))
		return false;

	if(eth_dst != NULL && other.eth_dst != NULL)
	{
		if(strcmp(eth_dst,other.eth_dst) != 0)
			return false;
	}

	if((eth_dst_mask == NULL && other.eth_dst_mask != NULL) ||
		(eth_dst_mask != NULL && other.eth_dst_mask == NULL))
		return false;

	if(eth_dst_mask != NULL && other.eth_dst_mask != NULL)
	{
		if(strcmp(eth_dst_mask,other.eth_dst_mask) != 0)
			return false;
	}

	if((isEthType && !other.isEthType) || (!isEthType && other.isEthType))
		return false;
	if(isEthType && ethType != other.ethType)
		return false;

	/*
	*	VLAN
	*/
	if((isVlanID && !other.isVlanID) || (!isVlanID && other.isVlanID))
		return false;
	if(isVlanID && vlanID != other.vlanID)
		return false;

	if((isNoVlan && !other.isNoVlan) || (!isNoVlan && other.isNoVlan))
		return false;

	if((isAnyVlan && !other.isAnyVlan) || (!isAnyVlan && other.isAnyVlan))
		return false;

	if((isVlanPCP && !other.isVlanPCP) || (!isVlanPCP && other.isVlanPCP))
		return false;
	if(isVlanPCP && vlanPCP != other.vlanPCP)
		return false;

	/*
	*	IPv4
	*/
	if((isIpDSCP && !other.isIpDSCP) || (!isIpDSCP && other.isIpDSCP))
		return false;
	if(isIpDSCP && ipDSCP != other.ipDSCP)
		return false;

	if((isIpECN && !other.isIpECN) || (!isIpECN && other.isIpECN))
		return false;
	if(isIpECN && ipECN != other.ipECN)
		return false;

	if((isIpProto && !other.isIpProto) || (!isIpProto && other.isIpProto))
		return false;
	if(isIpProto && ipProto != other.ipProto)
		return false;

	if((ipv4_src == NULL && other.ipv4_src != NULL) ||
		(ipv4_src != NULL && other.ipv4_src == NULL))
		return false;

	if(ipv4_src != NULL && other.ipv4_src != NULL)
	{
		if(strcmp(ipv4_src,other.ipv4_src) != 0)
			return false;
	}

	if((ipv4_src_mask == NULL && other.ipv4_src_mask != NULL) ||
		(ipv4_src_mask != NULL && other.ipv4_src_mask == NULL))
		return false;

	if(ipv4_src_mask != NULL && other.ipv4_src_mask != NULL)
	{
		if(strcmp(ipv4_src_mask,other.ipv4_src_mask) != 0)
			return false;
	}

	if((ipv4_dst == NULL && other.ipv4_dst != NULL) ||
		(ipv4_dst != NULL && other.ipv4_dst == NULL))
		return false;

	if(ipv4_dst != NULL && other.ipv4_dst != NULL)
	{
		if(strcmp(ipv4_dst,other.ipv4_dst) != 0)
			return false;
	}

	if((ipv4_dst_mask == NULL && other.ipv4_dst_mask != NULL) ||
		(ipv4_dst_mask != NULL && other.ipv4_dst_mask == NULL))
		return false;

	if(ipv4_dst_mask != NULL && other.ipv4_dst_mask != NULL)
	{
		if(strcmp(ipv4_dst_mask,other.ipv4_dst_mask) != 0)
			return false;
	}

	/*
	*	TCP
	*/
	if((isTcpProtocol != other.isTcpProtocol))
		return false;
	if(isTcpProtocol && (transport_src_port != other.transport_src_port || transport_dst_port != other.transport_dst_port))
		return false;


	/*
	*	UDP
	*/
	if(isUdpProtocol != other.isUdpProtocol)
		return false;
	if(isUdpProtocol && (transport_src_port != other.transport_src_port || transport_dst_port != other.transport_dst_port))
		return false;

	/*
	*	SCTP
	*/
	if(isSctpProtocol != other.isSctpProtocol)
	return false;
	if(isSctpProtocol && (transport_src_port != other.transport_src_port || transport_dst_port != other.transport_dst_port))
		return false;

	/*
	*	ICMPv4
	*/
	if((isIcmpv4Type && !other.isIcmpv4Type) || (!isIcmpv4Type && other.isIcmpv4Type))
		return false;
	if(isIcmpv4Type && icmpv4Type != other.icmpv4Type)
		return false;

	if((isIcmpv4Code && !other.isIcmpv4Code) || (!isIcmpv4Code && other.isIcmpv4Code))
		return false;
	if(isIcmpv4Code && icmpv4Code != other.icmpv4Code)
		return false;

	/*
	*	ARP
	*/
	if((isArpOpcode && !other.isArpOpcode) || (!isArpOpcode && other.isArpOpcode))
		return false;
	if(isArpOpcode && arpOpcode != other.arpOpcode)
		return false;

	if((arp_spa == NULL && other.arp_spa != NULL) ||
		(arp_spa != NULL && other.arp_spa == NULL))
		return false;

	if(arp_spa != NULL && other.arp_spa != NULL)
	{
		if(strcmp(arp_spa,other.arp_spa) != 0)
			return false;
	}

	if((arp_spa_mask == NULL && other.arp_spa_mask != NULL) ||
		(arp_spa_mask != NULL && other.arp_spa_mask == NULL))
		return false;

	if(arp_spa_mask != NULL && other.arp_spa_mask != NULL)
	{
		if(strcmp(arp_spa_mask,other.arp_spa_mask) != 0)
			return false;
	}

	if((arp_tpa == NULL && other.arp_tpa != NULL) ||
		(arp_tpa != NULL && other.arp_tpa == NULL))
		return false;

	if(arp_tpa != NULL && other.arp_tpa != NULL)
	{
		if(strcmp(arp_tpa,other.arp_tpa) != 0)
			return false;
	}

	if((arp_tpa_mask == NULL && other.arp_tpa_mask != NULL) ||
		(arp_tpa_mask != NULL && other.arp_tpa_mask == NULL))
		return false;

	if(arp_tpa_mask != NULL && other.arp_tpa_mask != NULL)
	{
		if(strcmp(arp_tpa_mask,other.arp_tpa_mask) != 0)
			return false;
	}

	if((arp_sha == NULL && other.arp_sha != NULL) ||
		(arp_sha != NULL && other.arp_sha == NULL))
		return false;

	if(arp_sha != NULL && other.arp_sha != NULL)
	{
		if(strcmp(arp_sha,other.arp_sha) != 0)
			return false;
	}

	if((arp_tha == NULL && other.arp_tha != NULL) ||
		(arp_tha != NULL && other.arp_tha == NULL))
		return false;

	if(arp_tha != NULL && other.arp_tha != NULL)
	{
		if(strcmp(arp_tha,other.arp_tha) != 0)
			return false;
	}


	/*
	*	IPv6
	*/
	if((ipv6_src == NULL && other.ipv6_src != NULL) ||
		(ipv6_src != NULL && other.ipv6_src == NULL))
		return false;

	if(ipv6_src != NULL && other.ipv6_src != NULL)
	{
		if(strcmp(ipv6_src,other.ipv6_src) != 0)
			return false;
	}

	if((ipv6_src_mask == NULL && other.ipv6_src_mask != NULL) ||
		(ipv6_src_mask != NULL && other.ipv6_src_mask == NULL))
		return false;

	if(ipv6_src_mask != NULL && other.ipv6_src_mask != NULL)
	{
		if(strcmp(ipv6_src_mask,other.ipv6_src_mask) != 0)
			return false;
	}


	if((ipv6_dst == NULL && other.ipv6_dst != NULL) ||
		(ipv6_dst != NULL && other.ipv6_dst == NULL))
		return false;

	if(ipv6_dst != NULL && other.ipv6_dst != NULL)
	{
		if(strcmp(ipv6_dst,other.ipv6_dst) != 0)
			return false;
	}

	if((ipv6_dst_mask == NULL && other.ipv6_dst_mask != NULL) ||
		(ipv6_dst_mask != NULL && other.ipv6_dst_mask == NULL))
		return false;

	if(ipv6_dst_mask != NULL && other.ipv6_dst_mask != NULL)
	{
		if(strcmp(ipv6_dst_mask,other.ipv6_dst_mask) != 0)
			return false;
	}

	if((isIpv6Flabel && !other.isIpv6Flabel) || (!isIpv6Flabel && other.isIpv6Flabel))
		return false;
	if(isIpv6Flabel && ipv6_flabel != other.ipv6_flabel)
		return false;

	if((ipv6_nd_target == NULL && other.ipv6_nd_target != NULL) ||
		(ipv6_nd_target != NULL && other.ipv6_nd_target == NULL))
		return false;

	if(ipv6_nd_target != NULL && other.ipv6_nd_target != NULL)
	{
		if(strcmp(ipv6_nd_target,other.ipv6_nd_target) != 0)
			return false;
	}

	if((ipv6_nd_sll == NULL && other.ipv6_nd_sll != NULL) ||
		(ipv6_nd_sll != NULL && other.ipv6_nd_sll == NULL))
		return false;

	if(ipv6_nd_sll != NULL && other.ipv6_nd_sll != NULL)
	{
		if(strcmp(ipv6_nd_sll,other.ipv6_nd_sll) != 0)
			return false;
	}

	if((ipv6_nd_tll == NULL && other.ipv6_nd_tll != NULL) ||
		(ipv6_nd_tll != NULL && other.ipv6_nd_tll == NULL))
		return false;

	if(ipv6_nd_tll != NULL && other.ipv6_nd_tll != NULL)
	{
		if(strcmp(ipv6_nd_tll,other.ipv6_nd_tll) != 0)
			return false;
	}

	/*
	*	ICMPv6
	*/
	if((isIcmpv6Type && !other.isIcmpv6Type) || (!isIcmpv6Type && other.isIcmpv6Type))
		return false;
	if(isIcmpv6Type && icmpv6Type != other.icmpv6Type)
		return false;

	if((isIcmpv6Code && !other.isIcmpv6Code) || (!isIcmpv6Code && other.isIcmpv6Code))
		return false;
	if(isIcmpv6Code && icmpv6Code != other.icmpv6Code)
		return false;

	/*
	*	MPLS
	*/
	if((isMplsLabel && !other.isMplsLabel) || (!isMplsLabel && other.isMplsLabel))
		return false;
	if(isMplsLabel && mplsLabel != other.mplsLabel)
		return false;

	if((isMplsTC && !other.isMplsTC) || (!isMplsTC && other.isMplsTC))
		return false;
	if(isMplsTC && mplsTC != other.mplsTC)
		return false;

	/*
	*	GRE
	*/
	if((gre_key == NULL && other.gre_key != NULL) ||
		(gre_key != NULL && other.gre_key == NULL))
		return false;

	if(gre_key != NULL && other.gre_key != NULL)
	{
		if(strcmp(gre_key,other.gre_key) != 0)
			return false;
	}

	return true;
}


void Match::setAllCommonFields(Match match)
{
	/*
	*	Ethernet
	*/
	if(match.eth_src != NULL)
		setEthSrc(match.eth_src);
	if(match.eth_src_mask)
		setEthSrcMask(match.eth_src_mask);
	if(match.eth_dst != NULL)
		setEthDst(match.eth_dst);
	if(match.eth_dst_mask)
		setEthDstMask(match.eth_dst_mask);
	if(match.isEthType)
		setEthType(match.ethType);

	/*
	*	VLAN
	*/
	if(match.isVlanID || match.isEndpointVlanID)
		setVlanID(match.vlanID);
	else if(match.isAnyVlan)
		setVlanIDAnyVlan();
	else if(match.isNoVlan)
		setVlanIDNoVlan();
	if(match.isVlanPCP)
		setVlanPCP(match.vlanPCP);

	/*
	*	IPv4
	*/
	if(match.isIpDSCP)
		setIpDSCP(match.ipDSCP);
	if(match.isIpECN)
		setIpECN(match.ipECN);
	if(match.isIpProto == true)
		setIpProto(match.ipProto);
	if(match.ipv4_src)
		setIpv4Src(match.ipv4_src);
	if(match.ipv4_src_mask)
		setIpv4SrcMask(match.ipv4_src_mask);
	if(match.ipv4_dst)
		setIpv4Dst(match.ipv4_dst);
	if(match.ipv4_dst_mask)
		setIpv4DstMask(match.ipv4_dst_mask);

	/*
	*	TCP, UDP, SCTP
	*/
	if(match.isTcpProtocol || match.isUdpProtocol || match.isSctpProtocol)
	{
		setTransportSrcPort(match.transport_src_port);
		setTransportDstPort(match.transport_dst_port);
	}

	/*
	*	ICMPv4
	*/
	if(match.isIcmpv4Type)
		setIcmpv4Type(match.icmpv4Type);
	if(match.isIcmpv4Code)
		setIcmpv4Code(match.icmpv4Code);

	/*
	*	ARP
	*/
	if(match.isArpOpcode)
		setArpOpCode(match.arpOpcode);
	if(match.arp_spa)
		setArpSpa(match.arp_spa);
	if(match.arp_spa_mask)
		setArpSpaMask(match.arp_spa_mask);
	if(match.arp_tpa)
		setArpTpa(match.arp_tpa);
	if(match.arp_tpa_mask)
		setArpTpaMask(match.arp_tpa_mask);
	if(match.arp_sha)
		setArpSha(match.arp_sha);
	if(match.arp_tha)
		setArpTha(match.arp_tha);

	/*
	*	IPv6
	*/
	if(match.ipv6_src)
		setIpv6Src(match.ipv6_src);
	if(match.ipv6_src_mask)
		setIpv6SrcMask(match.ipv6_src_mask);
	if(match.ipv6_dst)
		setIpv6Dst(match.ipv6_dst);
	if(match.ipv6_dst_mask)
		setIpv6DstMask(match.ipv6_dst_mask);
	if(match.isIpv6Flabel)
		setIpv6Flabel(match.ipv6_flabel);
	if(match.ipv6_nd_target)
		setIpv6NdTarget(match.ipv6_nd_target);
	if(match.ipv6_nd_sll)
		setIpv6NdSll(match.ipv6_nd_sll);
	if(match.ipv6_nd_tll)
		setIpv6NdTll(match.ipv6_nd_tll);

	/*
	*	ICMPv6
	*/
	if(match.isIcmpv6Type)
		setIcmpv6Type(match.icmpv6Type);
	if(match.isIcmpv6Code)
		setIcmpv6Code(match.icmpv6Code);

	/*
	*	MPLS
	*/
	if(match.isMplsLabel)
		setMplsLabel(match.mplsLabel);
	if(match.isMplsTC)
		setMplsTC(match.mplsTC);

	/*
	*	GRE
	*/
	if(match.gre_key)
		setGreKey(match.gre_key);
	if(match.ipv6_dst)
		setGreKey(match.gre_key);
}

void Match::setEthSrc(char *eth_src)
{
	this->eth_src = (char*)malloc(sizeof(char)*(strlen(eth_src)+1));
	strcpy(this->eth_src,eth_src);
}

void Match::setEthSrcMask(char *eth_src_mask)
{
	this->eth_src_mask = (char*)malloc(sizeof(char)*(strlen(eth_src_mask)+1));
	strcpy(this->eth_src_mask,eth_src_mask);
}

void Match::setEthDst(char *eth_dst)
{
	this->eth_dst = (char*)malloc(sizeof(char)*(strlen(eth_dst)+1));
	strcpy(this->eth_dst,eth_dst);
}

void Match::setEthDstMask(char *eth_dst_mask)
{
	this->eth_dst_mask = (char*)malloc(sizeof(char)*(strlen(eth_dst_mask)+1));
	strcpy(this->eth_dst_mask,eth_dst_mask);
}

void Match::setEthType(uint16_t ethType)
{
	this->ethType = ethType;
	isEthType = true;
}

void Match::setVlanID(uint16_t vlanID)
{
	this->vlanID = vlanID;
	isVlanID = true;
}

void Match::setVlanIDNoVlan()
{
	isVlanID = false;
	isAnyVlan = false;

	isNoVlan = true;
}

void Match::setVlanIDAnyVlan()
{
	isVlanID = false;
	isNoVlan = false;

	isAnyVlan = true;
}

void Match::setVlanPCP(uint8_t vlanPCP)
{
	this->vlanPCP = vlanPCP;
	isVlanPCP = true;
}

void Match::setIpDSCP(uint8_t ipDSCP)
{
	this->ipDSCP = ipDSCP;
	isIpDSCP = true;
}

void Match::setIpECN(uint8_t ipECN)
{
	this->ipECN = ipECN;
	isIpECN = true;
}

void Match::setIpProto(uint8_t ipProto)
{
	this->ipProto = ipProto;
	isIpProto = true;
	switch (ipProto)
	{
		case 6: isTcpProtocol=true;
			break;
		case 17: isUdpProtocol=true;
			break;
		case 132: isSctpProtocol=true;
			break;
	}
}

void Match::setIpv4Src(char *ipv4_src)
{
	this->ipv4_src = (char*)malloc(sizeof(char)*(strlen(ipv4_src)+1));
	strcpy(this->ipv4_src,ipv4_src);
}

void Match::setIpv4SrcMask(char *ipv4_src_mask)
{
	this->ipv4_src_mask = (char*)malloc(sizeof(char)*(strlen(ipv4_src_mask)+1));
	strcpy(this->ipv4_src_mask,ipv4_src_mask);
}

void Match::setIpv4Dst(char *ipv4_dst)
{
	this->ipv4_dst = (char*)malloc(sizeof(char)*(strlen(ipv4_dst)+1));
	strcpy(this->ipv4_dst,ipv4_dst);
}

void Match::setIpv4DstMask(char *ipv4_dst_mask)
{
	this->ipv4_dst_mask = (char*)malloc(sizeof(char)*(strlen(ipv4_dst_mask)+1));
	strcpy(this->ipv4_dst_mask,ipv4_dst_mask);
}

void Match::setTransportSrcPort(uint16_t tcp_src)
{
	this->transport_src_port = tcp_src;
}

void Match::setTransportDstPort(uint16_t tcp_dst)
{
	this->transport_dst_port = tcp_dst;
}

void Match::setTcpProtocol()
{
	isTcpProtocol = true;
}

void Match::setUdpProtocol()
{
	isUdpProtocol = true;
}

void Match::setSctpProtocol()
{
	isSctpProtocol = true;
}

void Match::setIcmpv4Type(uint8_t icmpv4Type)
{
	this->icmpv4Type = icmpv4Type;
	isIcmpv4Type = true;
}

void Match::setIcmpv4Code(uint8_t icmpv4Code)
{
	this->icmpv4Code = icmpv4Code;
	isIcmpv4Code = true;
}

void Match::setArpOpCode(uint16_t arpOpcode)
{
	this->arpOpcode = arpOpcode;
	isArpOpcode = true;
}

void Match::setArpSpa(char *arp_spa)
{
	this->arp_spa = (char*)malloc(sizeof(char)*(strlen(arp_spa)+1));
	strcpy(this->arp_spa,arp_spa);
}

void Match::setArpSpaMask(char *arp_spa_mask)
{
	this->arp_spa_mask = (char*)malloc(sizeof(char)*(strlen(arp_spa_mask)+1));
	strcpy(this->arp_spa_mask,arp_spa_mask);
}

void Match::setArpTpa(char *arp_tpa)
{
	this->arp_tpa = (char*)malloc(sizeof(char)*(strlen(arp_tpa)+1));
	strcpy(this->arp_tpa,arp_tpa);
}

void Match::setArpTpaMask(char *arp_tpa_mask)
{
	this->arp_tpa_mask = (char*)malloc(sizeof(char)*(strlen(arp_tpa_mask)+1));
	strcpy(this->arp_tpa_mask,arp_tpa_mask);
}

void Match::setArpSha(char *arp_sha)
{
	this->arp_sha = (char*)malloc(sizeof(char)*(strlen(arp_sha)+1));
	strcpy(this->arp_sha,arp_sha);
}

void Match::setArpTha(char *arp_tha)
{
	this->arp_tha = (char*)malloc(sizeof(char)*(strlen(arp_tha)+1));
	strcpy(this->arp_tha,arp_tha);
}

void Match::setIpv6Src(char *ipv6_src)
{
	this->ipv6_src = (char*)malloc(sizeof(char)*(strlen(ipv6_src)+1));
	strcpy(this->ipv6_src,ipv6_src);
}

void Match::setIpv6SrcMask(char *ipv6_src_mask)
{
	this->ipv6_src_mask = (char*)malloc(sizeof(char)*(strlen(ipv6_src_mask)+1));
	strcpy(this->ipv6_src_mask,ipv6_src_mask);
}

void Match::setIpv6Dst(char *ipv6_dst)
{
	this->ipv6_dst = (char*)malloc(sizeof(char)*(strlen(ipv6_dst)+1));
	strcpy(this->ipv6_dst,ipv6_dst);
}

void Match::setIpv6DstMask(char *ipv6_dst_mask)
{
	this->ipv6_dst_mask = (char*)malloc(sizeof(char)*(strlen(ipv6_dst_mask)+1));
	strcpy(this->ipv6_dst_mask,ipv6_dst_mask);
}

void Match::setIpv6Flabel(uint32_t ipv6_flabel)
{
	this->ipv6_flabel = ipv6_flabel;
	isIpv6Flabel = true;
}

void Match::setIpv6NdTarget(char *ipv6_nd_target)
{
	this->ipv6_nd_target = (char*)malloc(sizeof(char)*(strlen(ipv6_nd_target)+1));
	strcpy(this->ipv6_nd_target,ipv6_nd_target);
}

void Match::setIpv6NdSll(char *ipv6_nd_sll)
{
	this->ipv6_nd_sll = (char*)malloc(sizeof(char)*(strlen(ipv6_nd_sll)+1));
	strcpy(this->ipv6_nd_sll,ipv6_nd_sll);
}

void Match::setIpv6NdTll(char *ipv6_nd_tll)
{
	this->ipv6_nd_tll = (char*)malloc(sizeof(char)*(strlen(ipv6_nd_tll)+1));
	strcpy(this->ipv6_nd_tll,ipv6_nd_tll);
}

void Match::setIcmpv6Type(uint8_t icmpv6Type)
{
	this->icmpv6Type = icmpv6Type;
	isIcmpv6Type = true;
}

void Match::setIcmpv6Code(uint8_t icmpv6Code)
{
	this->icmpv6Code = icmpv6Code;
	isIcmpv6Code = true;
}

void Match::setMplsLabel(uint32_t mplsLabel)
{
	this->mplsLabel = mplsLabel;
	isMplsLabel = true;
}

void Match::setMplsTC(uint8_t mplsTC)
{
	this->mplsTC = mplsTC;
	isMplsTC = true;
}

void Match::setEndpointVlanID(uint16_t vlanID)
{
	this->vlanID = vlanID;
	isEndpointVlanID = true;
}

void Match::setGreKey(char *gre_key)
{
	this->gre_key = (char*)malloc(sizeof(char)*(strlen(gre_key)+1));
	strcpy(this->gre_key,gre_key);
}

void Match::toJSON(Object &match)
{
	/*
	*	Ethernet
	*/
	if(eth_src != NULL)
		match[ETH_SRC] = eth_src;
	if(eth_src_mask != NULL)
		match[ETH_SRC_MASK] = eth_src_mask;
	if(eth_dst)
		match[ETH_DST] = eth_dst;
	if(eth_dst_mask)
		match[ETH_DST_MASK] = eth_dst_mask;
	if(isEthType)
	{
		stringstream ethtype;
		ethtype << hex << ethType;
		match[ETH_TYPE] = ethtype.str().c_str();
	}

	/*
	*	VLAN
	*/
	if(isVlanID)
	{
		stringstream vlanid;
		vlanid << dec << vlanID;
		match[VLAN_ID] = vlanid.str().c_str();
	}
	else if(isAnyVlan)
		match[VLAN_ID] = ANY_VLAN;
	else if(isNoVlan)
		match[VLAN_ID] = NO_VLAN;

	if(isVlanPCP)
	{
		stringstream vlanpcp;
		vlanpcp << vlanPCP;
		match[VLAN_PCP] = vlanpcp.str().c_str();
	}

	/*
	*	IPv4
	*/
	if(isIpDSCP)
	{
		stringstream ipdscp;
		ipdscp << ipDSCP;
		match[IP_DSCP] = ipdscp.str().c_str();
	}
	if(isIpECN)
	{
		stringstream ipecn;
		ipecn << ipECN;
		match[IP_ECN] = ipecn.str().c_str();
	}
	if(isIpProto)
	{
		stringstream ipproto;
		ipproto << (ipProto & 0xFF);
		match[IP_PROTO] = ipproto.str().c_str();
	}
	if(ipv4_src)
		match[IP_SRC] =  ipv4_src;
	if(ipv4_src_mask)
		match[IPv4_SRC_MASK] =  ipv4_src_mask;
	if(ipv4_dst)
		match[IP_DST] =  ipv4_dst;
	if(ipv4_dst_mask)
		match[IPv4_DST_MASK] =  ipv4_dst_mask;

	/*
	*	TCP, UDP
	*/
	if(isTcpProtocol || isUdpProtocol)
	{
		stringstream tcpUdpProto;
		if(transport_src_port)
		{
			tcpUdpProto << transport_src_port;
			match[PORT_SRC] = tcpUdpProto.str().c_str();
		}
		else if(transport_dst_port)
		{
			tcpUdpProto << transport_dst_port;
			match[PORT_DST] = tcpUdpProto.str().c_str();
		}
	}

	/*
	*	SCTP
	*/
	if(isSctpProtocol)
	{
		stringstream scptProto;
		if(transport_src_port)
		{
			scptProto << transport_src_port;
			match[SCTP_SRC] = scptProto.str().c_str();
		}
		else if(transport_dst_port)
		{
			scptProto << transport_dst_port;
			match[SCTP_DST] = scptProto.str().c_str();
		}
	}

	/*
	*	ICMPv4
	*/
	if(isIcmpv4Type)
	{
		stringstream icmpv4type;
		icmpv4type << icmpv4Type;
		match[ICMPv4_TYPE] = icmpv4type.str().c_str();
	}
	if(isIcmpv4Code)
	{
		stringstream icmpv4code;
		icmpv4code << icmpv4Code;
		match[ICMPv4_CODE] = icmpv4code.str().c_str();
	}

	/*
	*	ARP
	*/
	if(isArpOpcode)
	{
		stringstream arpopcode;
		arpopcode << arpOpcode;
		match[ARP_OPCODE] = arpopcode.str().c_str();
	}
	if(arp_spa)
		match[ARP_SPA] = arp_spa;
	if(arp_spa_mask)
	 	match[ARP_SPA_MASK] = arp_spa_mask;
	if(arp_tpa)
		match[ARP_TPA] = arp_tpa;
	if(arp_tpa_mask)
		match[ARP_TPA_MASK] = arp_tpa_mask;
	if(arp_sha)
		match[ARP_SHA] = arp_sha;
	if(arp_tha)
		match[ARP_THA] = arp_tha;

	/*
	*	IPv6
	*/
	if(ipv6_src)
		match[IP_SRC] = ipv6_src;
	if(ipv6_src_mask)
		match[IPv6_SRC_MASK] = ipv6_src_mask;
	if(ipv6_dst)
		match[IP_DST] = ipv6_dst;
	if(ipv6_dst_mask)
		match[IPv6_DST_MASK] = ipv6_dst_mask;
	if(isIpv6Flabel)
	{
		stringstream ipv6flabel;
		ipv6flabel << ipv6_flabel;
		match[IPv6_FLABEL] = ipv6flabel.str().c_str();
	}
	if(ipv6_nd_target)
		 match[IPv6_ND_TARGET] = ipv6_nd_target;
	if(ipv6_nd_sll)
		  match[IPv6_ND_SLL] = ipv6_nd_sll;
	if(ipv6_nd_tll)
		 match[IPv6_ND_TLL] = ipv6_nd_tll;

	/*
	*	ICMPv6
	*/
	if(isIcmpv6Type)
	{
		stringstream icmpv6type;
		icmpv6type << icmpv6Type;
		match[ICMPv6_TYPE] = icmpv6type.str().c_str();
	}
	if(isIcmpv6Code)
	{
		stringstream icmpv6code;
		icmpv6code << icmpv6Code;
		match[ICMPv6_CODE] = icmpv6code.str().c_str();
	}

	/*
	*	MPLS
	*/
	if(isMplsLabel)
	{
		stringstream mplslabel;
		mplslabel << mplsLabel;
		match[MPLS_LABEL] = mplslabel.str().c_str();
	}
	if(isMplsTC)
	{
		stringstream mplstc;
		mplstc << mplsTC;
		match[MPLS_TC] = mplstc.str().c_str();
	}

	/*
	*	GRE
	*/
	if(gre_key)
		match[GRE_KEY] = gre_key;
}


string Match::prettyPrint()
{
	stringstream ss;
	ss << "";

	/*
	*	Ethernet
	*/
	if(eth_src != NULL)
		ss << " # ethernet src: " << eth_src;
	if(eth_dst != NULL)
		ss << " # ethernet dst: " << eth_dst;
	if(eth_src_mask)
		ss << " # ethernet src mask: " << eth_src_mask;
	if(eth_dst_mask)
		ss << " # ethernet dst mask: " << eth_dst_mask;
	if(isEthType)
		ss << " # ethertype: " <<  "0x" << hex << ethType;

	/*
	*	VLAN
	*/
	if(isVlanID)
		ss << " # VLAN ID: " << hex << "0x" << vlanID;
	else if(isAnyVlan)
		ss << " # VLAN ID: ANY";
	else if(isNoVlan)
		ss << " # NO VLAN";

	if(isVlanPCP)
		ss << " # VLAN PCP: " << int(vlanPCP);

	/*
	*	IPv4
	*/
	if(isIpDSCP)
		ss << " # IPv4 dscp: " << int(ipDSCP);
	if(isIpECN)
		ss << " # IPv4 ecn: " << int(ipECN);
	if(isIpProto)
		ss << " # IPv4 proto: " << (ipProto & 0xFF);
	if(ipv4_src)
		ss << " # IPv4 src: " << ipv4_src;
	if(ipv4_src_mask)
		ss << " # IPv4 src mask: " << ipv4_src_mask;
	if(ipv4_dst)
		ss << " # IPv4 dst: " << ipv4_dst;
	if(ipv4_dst_mask)
		ss << " # IPv4 dst mask: " << ipv4_dst_mask;

	/*
	*	TCP
	*/
	if(isTcpProtocol && transport_src_port)
		ss << " # TCP src port: " << transport_src_port;
	if(isTcpProtocol && transport_dst_port)
		ss << " # TCP dst port: " << transport_dst_port;

	/*
	*	UDP
	*/
	if(isUdpProtocol && transport_src_port)
		ss << " # UDP src port: " << transport_src_port;
	if(isUdpProtocol && transport_dst_port)
		ss << " # UDP dst port: " << transport_dst_port;

	/*
	*	SCTP
	*/
	if(isSctpProtocol && transport_src_port)
		ss << " # SCTP src port: " << transport_src_port;
	if(isSctpProtocol && transport_dst_port)
		ss << " # SCTP dst port: " << transport_dst_port;

	/*
	*	ICMPv4
	*/
	if(isIcmpv4Type)
		ss << " # ICMPv4 type: " << int(icmpv4Type);
	if(isIcmpv4Code)
		ss << " # ICMPv4 code: " << int(icmpv4Code);

	/*
	*	ARP
	*/
	if(isArpOpcode)
		ss << " # ARP opcode: " << arpOpcode;
	if(arp_spa)
		ss << " # ARP spa: " << arp_spa;
	if(arp_spa_mask)
	 	ss << " # ARP spa mask: " << arp_spa_mask;
	if(arp_tpa)
		ss << " # ARP tpa: " << arp_tpa;
	if(arp_tpa_mask)
		ss << " # ARP tpa mask: " << arp_tpa_mask;
	if(arp_sha)
		ss << " # ARP sha: " << arp_sha;
	if(arp_tha)
		ss << " # ARP tha: " << arp_tha;

	/*
	*	IPv6
	*/
	if(ipv6_src)
		ss << " # IPv6 src: " << ipv6_src;
	if(ipv6_src_mask)
		ss << " # IPv6 src mask: " << ipv6_src_mask;
	if(ipv6_dst)
		ss << " # IPv6 dst: " << ipv6_dst;
	if(ipv6_dst_mask)
		ss << " # IPv6 dst mask: " << ipv6_dst_mask;
	if(isIpv6Flabel)
		ss << " # IPv6 flabel: " << ipv6_flabel;
	if(ipv6_nd_target)
		 ss << " # IPv6 nd target: " << ipv6_nd_target;
	if(ipv6_nd_sll)
		 ss << " # IPv6 nd sll: " << ipv6_nd_sll;
	if(ipv6_nd_tll)
		ss << " # IPv6 nd tll: " << ipv6_nd_tll;

	/*
	*	ICMPv6
	*/
	if(isIcmpv6Type)
		ss << " # ICMPv6 type: "<<  int(icmpv6Type);
	if(isIcmpv6Code)
		ss << " # ICMPv6 code: " << int(icmpv6Code);

	/*
	*	MPLS
	*/
	if(isMplsLabel)
		ss << " # MPLS label: " << mplsLabel;
	if(isMplsTC)
		ss << " # MPLS tc: " << int(mplsTC);

	/*
	*	GRE
	*/
	if(gre_key)
		ss << " # GRE key: " << gre_key;

	return ss.str();
}

}
