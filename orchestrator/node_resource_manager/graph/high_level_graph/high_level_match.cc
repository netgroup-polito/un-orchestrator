#include  "high_level_match.h"

namespace highlevel
{

Match::Match() :
	graph::Match(), nf_port(0), type(MATCH_GENERIC)
{

}

bool Match::setInputPort(string input_port)
{
	if(type != MATCH_GENERIC)
		return false;

	input = (char*)malloc(sizeof(char)*(input_port.length()+1));
	strcpy(input,input_port.c_str());
	type = MATCH_PORT;

	return true;
}

bool Match::setNFport(string network_function, int port)
{
	if(type != MATCH_GENERIC)
		return false;

	input = (char*)malloc(sizeof(char)*(network_function.length()+1));
	strcpy(input,network_function.c_str());
	this->nf_port = port;
	type = MATCH_NF;

	return true;
}

bool Match::setEndPointInternal(/*string graphID, unsigned int endpoint*/string group)
{
	if(type != MATCH_GENERIC)
		return false;

	/*input = (char*)malloc(sizeof(char)*(graphID.length()+1));
	strcpy(input,graphID.c_str());*/
	//this->endpoint = endpoint;

	sscanf(group.c_str(), "%u", &this->endpointInternalGroup);
	type = MATCH_ENDPOINT_INTERNAL;

	return true;
}

bool Match::setEndPointGre(string endpointGreID)
{
	if(type != MATCH_GENERIC)
		return false;

	this->endpointGreID = endpointGreID;
	type = MATCH_ENDPOINT_GRE;

	return true;
}

bool Match::setInputEndpoint(string input_endpoint)
{
	this->input_endpoint = (char*)malloc(sizeof(char)*(input_endpoint.length()+1));
	strcpy(this->input_endpoint,input_endpoint.c_str());

	input = (char*)malloc(sizeof(char)*(input_endpoint.length()+1));
	strcpy(input,input_endpoint.c_str());

	return true;
}

bool Match::setNFEndpointPort(string nf_endpoint_port)
{
	this->nf_endpoint_port = (char*)malloc(sizeof(char)*(nf_endpoint_port.length()+1));
	strcpy(this->nf_endpoint_port,nf_endpoint_port.c_str());

	return true;
}

bool Match::matchOnPort()
{
	if(type == MATCH_PORT)
		return true;
	return false;
}

bool Match::matchOnNF()
{
	if(type == MATCH_NF)
		return true;
	return false;
}

bool Match::matchOnEndPointInternal()
{
	if(type == MATCH_ENDPOINT_INTERNAL)
		return true;
	return false;
}

bool Match::matchOnEndPointGre()
{
	if(type == MATCH_ENDPOINT_GRE)
		return true;
	return false;
}

string Match::getPhysicalPort()
{
	assert(type == MATCH_PORT);
	return input;
}

string Match::getNF()
{
	assert(type == MATCH_NF);
	return input;
}

int Match::getPortOfNF()
{
	assert(type == MATCH_NF);
	return nf_port;
}

string Match::getGraphID()
{
	assert(type == MATCH_ENDPOINT_INTERNAL);
	return input;
}

string Match::getEndPointGre()
{
	assert(type == MATCH_ENDPOINT_GRE);

	return endpointGreID;
}

unsigned int Match::getEndPointInternal()
{
	assert(type == MATCH_ENDPOINT_INTERNAL);

	return endpointInternalGroup;
}

char *Match::getInputEndpoint()
{
	assert(type == MATCH_ENDPOINT_GRE);

	//Check the name of port
	char delimiter[] = ":";
	char * pnt;

	string str;

	char tmp[BUFFER_SIZE];
	strcpy(tmp,(char *)input_endpoint);
	pnt=strtok(tmp, delimiter);
	int i = 0;

	while( pnt!= NULL )
	{
		switch(i)
		{
			case 1:
				str = string(pnt);
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return (char *)str.c_str();
}

bool Match::checkVlanPresence()
{
	return isVlanID || isAnyVlan || isVlanPCP;
}

Object Match::toJSON()
{
	Object match;

	if(type == MATCH_PORT || type == MATCH_ENDPOINT_GRE || type == MATCH_ENDPOINT_INTERNAL)
		match[PORT_IN]  = input_endpoint;
	else if(type == MATCH_NF)
	{
		stringstream nf;
		nf << input << ":" << nf_port;
		match[PORT_IN] = nf_endpoint_port;
	}

	graph::Match::toJSON(match);

	return match;
}

}
