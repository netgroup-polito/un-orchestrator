#include "NFtemplate.h"
#include <string>
#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;
using namespace std;

/**
*	@brief: this class parses a network function template
*/

class Template_Parser{

public:
	/**
	 * 	@brief: given a template in a list, it fills an object with data receveid
	 * 	@param temp:  object to fill with data parsed
	 * 	@param obj:   object containing the json representing a template
	 */

	static void setTemplateFromJson(NFtemplate & temp,Object obj);

	/**
	* 	@brief: It fills a list of templates with data returned by VNF Repository
	* 	@param templates:  list to fill in,
	* 	@param data:  data returned by VNF repository
	* 	@param checkVnfTemplate:   flag which allows to check if field vnf_template is in NF-FG
	*/

	static bool parse(std::list<NFtemplate>& templates,string data,bool checkVnfTemplate);

	/**
	* 	@brief: It adds in a template object an object representing a port
	* 	@param temp:  template to fill in with data
	* 	@param port:  port to add in the template
	*/

	static bool parsePort(NFtemplate& temp,Object port);

	/**
	* 	@brief: It adds in a template object an object representing a CPUrequirements
	* 	@param temp:  template to fill in with data
	* 	@param CPUrequirements:  CPURequirements to add in the template
	*/

	static bool parseCoreNumbers(NFtemplate& temp,Object CPUrequirements);
};
