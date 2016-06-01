#ifndef MONITORING_CONTROLLER_H_
#define MONITORING_CONTROLLER_H_ 1

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "../node_resource_manager/pub_sub/pub_sub.h"
#include "../utils/logger.h"
#include "monitoring_controller_constants.h"

using namespace json_spirit;

class MonitoringController
{

public:
	MonitoringController();
	
	/**
	*	@brief: set a monitoring session on the monitoring plugin
	*
	*	@param measure_string: string formatted according to the MEASURE language
	*	@param vnfsMapping: list that maps the VNF ID defined in the graph on the name of the "executable"
	*	@param portsMapping: list that, for each VNF of the graph, maps the ports ID into the real name of
	*						thos ports on the LSI
	*/
	void startMonitoring(string measure_string, list< pair<string, string> > vnfsMapping, list<map<unsigned int, string> > portsMapping);
	
	/**
	*	@brief: stop a monitoring session
	*/
	void stopMonitoring();
};

#endif //MONITORING_CONTROLLER_H_
