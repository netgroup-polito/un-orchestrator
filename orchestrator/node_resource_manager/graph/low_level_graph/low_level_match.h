#ifndef LOW_LEVEL_MATCH_H_
#define LOW_LEVEL_MATCH_H_ 1

#pragma once

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <ostream>

/*
IVANO: these include should be useless
#include <rofl/platform/unix/cunixenv.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/common/cparams.h>
*/

//#include <rofl/common/ciosrv.h>
#include <rofl/common/logging.h>
#include <rofl/common/openflow/coxmatch.h>

#include "../../../utils/logger.h"
#include "../match.h"
#include "../../graph_manager/lsi.h"

using namespace rofl;
using namespace std;

namespace lowlevel
{

class Match : public graph::Match
{
private:

	bool isInput_port;
	unsigned int input_port;
	bool is_local_port;

public:
	Match();

	Match(bool is_local_port);

	bool operator==(const Match &other) const;

	/**
	*	@brief: insert the match into a flowmod message
	*
	*	@param: message		flowmod message
	*/
	void fillFlowmodMessage(rofl::openflow::cofflowmod &message);

	/**
	*	@brief: it is a sort of copy constructor. In fact, starting
	*		from a generic Match, it creates a lowlevel match with the
	*		same "common" parameters
	*/
	void setAllCommonFields(graph::Match match);

	void setInputPort(unsigned int input_port);

	void print();
	string prettyPrint(LSI *lsi0,map<string,LSI *> lsis);
};

}
#endif //LOW_LEVEL_MATCH_H_
