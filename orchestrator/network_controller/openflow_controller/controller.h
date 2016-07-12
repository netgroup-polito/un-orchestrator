#ifndef CONTROLLER_H_
#define CONTROLLER_H_ 1

#pragma once

#include <rofl/common/logging.h>

#include "../../node_resource_manager/graph/low_level_graph/graph.h"
#include "../../utils/logger.h"
#include "../../utils/constants.h"

#define OFCONTROLLER_MODULE_NAME	"Openflow-Controller"

using namespace rofl;
using namespace lowlevel;

/**
*	@brief: Openflow controller associated with one tenant. It inserts into the
*		LSI rules derived from the commands received by the node-orchestrator
*		through the REST interface, but it does not react to events coming from
*		the LSI itself.
*/

class Controller : public crofbase
{
friend class GraphManager;

private:
	/**
	*	@brief: handle to the datapath
	*/
	rofl::crofdpt *dpt;

	/**
	*	@brief: flag indicating whereas the connection with the datapath is open
	*/
	bool isOpen;

	/**
	*	@brief: NFs graph to be translated into flowmod messages
	*		and instantiated within the LSI
	*/
	Graph graph;

	/**
	*	@brief: TCP port that the dpath should use to contact the controller
	*/
	string controllerPort;

	/**
	*	@brief: all the operations in the controller are serialized with
	*		this mutex
	*/
	pthread_mutex_t controller_mutex;

	/**
	*	@brief: install new rules in the datapath.
	*/
	bool installNewRulesIntoLSI(list<Rule> rules);

	/**
	*	@brief: remove rules from the datapath.
	*/
	bool removeRulesFromLSI(list<Rule> rules);

public:
	Controller(rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap,Graph graph,string controllerPort);

	void start();

	/**
	*	@brief: callback executed when the connection with the datapath is established.
	*		Downloads the rules to create the graph into the LSI.
	*/
	virtual void handle_dpt_open(crofdpt& dpt);

	/**
	*
	*	@brief: callback executed when the connection with the datapath is closed.
	**/
	virtual void handle_dpt_close(crofdpt& dpt);

//	virtual void handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid,rofl::openflow::cofmsg_packet_in& msg);

	/**
	*	@brief: install a new rule in the datapath.
	*/
	bool installNewRule(Rule rule);

	/**
	*	@brief: install new rules in the datapath.
	*/
	bool installNewRules(list<Rule> rules);

	/**
	*	@brief: remove a rule with a specific ID. If the graph does not have other
	*		identical rules (i.e., same match and same action), a flowmod to remove
	*		the flow from the LSI is sent to the LSI itself.
	*/
	bool removeRuleFromID(string ID);

	/**
	*	@brief: remove rules from the datapath.
	*/
	bool removeRules(list<Rule> rules);

	static void *loop(void *param);

protected:
	/**
	*	@brief: return the TCP port that the dpath should use to contact the controller
	*/
	string getControllerPort();

};

#endif //CONTROLLER_H_
