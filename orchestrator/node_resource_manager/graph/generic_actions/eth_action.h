#ifndef ETH_ACTION_H_
#define ETH_ACTION_H_ 1

#define __STDC_FORMAT_MACROS
#include "generic_action.h"

//#include <inttypes.h>
//#include <iostream>
//#include <sstream>

//#include <rofl/common/protocols/fvlanframe.h>
#include <rofl/common/caddress.h>

enum eth_action_t 
{
	ACTION_TRUSTED_PORT //it sets the source mac address
	// [+] add here new actions related to ethernet
};

class EthAction : public GenericAction
{
private:
	eth_action_t type;
	string mac_address;

public:
	EthAction(eth_action_t type, string mac_address);
	~EthAction();

	void toJSON(Array &json);

	/**
	*	@brief: insert the generic action into a flowmod message
	*
	*	@param: message		flowmod message
	*	@param: position	position, in the flowmod, in which the action must be inserted
	*/
	void fillFlowmodMessage(rofl::openflow::cofflowmod &message, unsigned int *position);

	void print();
	string prettyPrint();
};


#endif //ETH_ACTION_H_
