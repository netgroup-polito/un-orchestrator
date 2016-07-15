#ifndef MATCH_H_
#define MATCH_H_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <rofl/platform/unix/cunixenv.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/common/cparams.h>

#include <rofl/common/ciosrv.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/logging.h>

#include "../../utils/logger.h"
#include "../../utils/constants.h"

#include <iostream>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;
using namespace rofl;
using namespace std;

namespace graph
{

class Match
{

protected:

	/*
	*	Ethernet
	*/
	char *eth_src;
	char *eth_src_mask;
	char *eth_dst;
	char *eth_dst_mask;
	bool isEthType;
	uint16_t ethType;

	/*
	*	VLAN
	*/
	bool isVlanID;
	uint16_t vlanID;
	bool isNoVlan;
	bool isAnyVlan;
	bool isVlanPCP;
	uint8_t vlanPCP;
	bool isEndpointVlanID;

	/*
	*	IPv4
	*/
	bool isIpDSCP;
	uint8_t ipDSCP;
	bool isIpECN;
	uint8_t ipECN;
	bool isIpProto;
	uint8_t ipProto;
	char *ipv4_src;
	char *ipv4_src_mask;
	char *ipv4_dst;
	char *ipv4_dst_mask;

	/*
	*	TCP
	*/
	bool isTcpSrc;
	uint16_t tcp_src;
	bool isTcpDst;
	uint16_t tcp_dst;

	/*
	*	UDP
	*/
	bool isUdpSrc;
	uint16_t udp_src;
	bool isUdpDst;
	uint16_t udp_dst;

	/*
	*	SCTP
	*/
	bool isSctpSrc;
	uint16_t sctp_src;
	bool isSctpDst;
	uint16_t sctp_dst;

	/*
	*	ICMPv4
	*/
	bool isIcmpv4Type;
	uint8_t icmpv4Type;
	bool isIcmpv4Code;
	uint8_t icmpv4Code;

	/*
	*	ARP
	*/
	bool isArpOpcode;
	uint16_t arpOpcode;
	char *arp_spa;
	char *arp_spa_mask;
	char *arp_tpa;
	char *arp_tpa_mask;
	char *arp_sha;
	char *arp_tha;

	/*
	*	IPv6
	*/
	char *ipv6_src;
	char *ipv6_src_mask;
	char *ipv6_dst;
	char *ipv6_dst_mask;
	bool isIpv6Flabel;
	uint32_t ipv6_flabel;
	char *ipv6_nd_target;
	char *ipv6_nd_sll;
	char *ipv6_nd_tll;

	/*
	*	ICMPv6
	*/
	bool isIcmpv6Type;
	uint8_t icmpv6Type;
	bool isIcmpv6Code;
	uint8_t icmpv6Code;

	/*
	*	MPLS
	*/
	bool isMplsLabel;
	uint32_t mplsLabel;
	bool isMplsTC;
	uint8_t mplsTC;

	/*
	*	GRE
	*/
	char *gre_key;

	Match();

	bool isEqual(const Match &other) const;

public:

	void setEthSrc(char *eth_src);
	void setEthSrcMask(char *eth_src_mask);
	void setEthDst(char *eth_dst);
	void setEthDstMask(char *eth_dst_mask);
	void setEthType(uint16_t ethType);
	void setVlanID(uint16_t vlanID);
	void setVlanIDNoVlan();
	void setVlanIDAnyVlan();
	void setVlanPCP(uint8_t vlanPCP);
	void setIpDSCP(uint8_t ipDSCP);
	void setIpECN(uint8_t ipECN);
	void setIpProto(uint8_t ipProto);
	void setIpv4Src(char *ipv4_src);
	void setIpv4SrcMask(char *ipv4_src_mask);
	void setIpv4Dst(char *ipv4_dst);
	void setIpv4DstMask(char *ipv4_dst_mask);
	void setTcpSrc(uint16_t tcp_src);
	void setTcpDst(uint16_t tcp_dst);
	void setUdpSrc(uint16_t udp_src);
	void setUdpDst(uint16_t udp_dst);
	void setSctpSrc(uint16_t sctp_src);
	void setSctpDst(uint16_t sctp_dst);
	void setIcmpv4Type(uint8_t icmpv4Type);
	void setIcmpv4Code(uint8_t icmpv4Code);
	void setArpOpCode(uint16_t arpOpcode);
	void setArpSpa(char *arp_spa);
	void setArpSpaMask(char *arp_spa_mask);
	void setArpTpa(char *arp_tpa);
	void setArpTpaMask(char *arp_tpa_mask);
	void setArpSha(char *arp_sha);
	void setArpTha(char *arp_tha);
	void setIpv6Src(char *ipv6_src);
	void setIpv6SrcMask(char *ipv6_src_mask);
	void setIpv6Dst(char *ipv6_dst);
	void setIpv6DstMask(char *ipv6_dst_mask);
	void setIpv6Flabel(uint32_t ipv6_flabel);
	void setIpv6NdTarget(char *ipv6_nd_target);
	void setIpv6NdSll(char *ipv6_nd_sll);
	void setIpv6NdTll(char *ipv6_nd_tll);
	void setIcmpv6Type(uint8_t icmpv6Type);
	void setIcmpv6Code(uint8_t icmpv6Code);
	void setMplsLabel(uint32_t mplsLabel);
	void setMplsTC(uint8_t mplsTC);
	void setEndpointVlanID(uint16_t vlanID);
	void setGreKey(char *gre_key);

	virtual void setAllCommonFields(Match match);

	virtual string prettyPrint();
	virtual void toJSON(Object &match);
};

}

#endif //MATCH_H_
