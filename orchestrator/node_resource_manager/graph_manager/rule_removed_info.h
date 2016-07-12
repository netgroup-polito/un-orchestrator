#ifndef RULE_REMOVED_INFO_H_
#define RULE_REMOVED_INFO_H_ 1

#pragma once

/**
*	@brief: class containing information on VNFs and virtual links to be removed from the graph
*/

class RuleRemovedInfo
{
public:
	/**
	* If the action to be removed contained a NF port, it is inserted here
	*/
	string nf_port;
	bool isNFport;

	/**
	*	If the action to be removed contained a port, it is inserted here
	*/
	string port;
	bool isPort;

	/**
	*	If the action to be removed contained an endpoint internal, it is inserted here
	*/
	string endpointInternal;
	bool isEndpointInternal;

	/**
	*	If the action to be removed contained an endpoint gre, it is inserted here
	*/
	string endpointGre;
	bool isEndpointGre;
};

#endif //RULE_REMOVED_INFO
