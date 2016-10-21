#include "eth_action.h"

EthAction::EthAction(eth_action_t type, string mac_address):
	GenericAction(), type(type), mac_address(mac_address)
{

}

EthAction::~EthAction()
{

}

void EthAction::toJSON(Array &action)
{
	if(type == ACTION_TRUSTED_PORT)
		//We don't add anything in case of the action implements a trusted port
		return;

	//[+] add here further code in case new actions on ethernet are defined
}

void EthAction::fillFlowmodMessage(rofl::openflow::cofflowmod &message, unsigned int *position)
{
	switch(OFP_VERSION)
	{
		case OFP_10:
			assert(0 && "TODO");
			//TODO
			exit(0);
			break;
		case OFP_12:
		case OFP_13:
			assert(type == ACTION_TRUSTED_PORT);

			message.set_instructions().set_inst_apply_actions().set_actions().add_action_set_field(rofl::cindex(*position)).set_oxm(rofl::openflow::coxmatch_ofb_eth_src(rofl::cmacaddr(mac_address)));
			(*position)++;

			break;
	}
}

void EthAction::print()
{
	stringstream type_print;
	switch(type)
	{
		case ACTION_TRUSTED_PORT:
			cout << "\t\t\tSET SOURCE MAC ADDRESS " << mac_address << endl;
			break;
	}

}

string EthAction::prettyPrint()
{
	stringstream ss;
	
	if(type == ACTION_TRUSTED_PORT)
	{
		ss << " # trusted on MAC address: " << mac_address;
	}
	//[+] add here code to handle further types

	return ss.str();
}

