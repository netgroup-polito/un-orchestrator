#include "graph.h"

static const char LOG_MODULE_NAME[] = "Low-Level-Graph";

namespace lowlevel
{

void Graph::addRules(list<Rule> rules)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
		addRule(*r);
}

void Graph::addRule(Rule rule)
{
	ULOG_DBG("Looking for rule: %s",rule.getID().c_str());

	rules.push_back(rule);

	ULOG_DBG("The graph contains the following rules: ");
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		ULOG_DBG("\t%s",it->getID().c_str());
	}
}

Rule Graph::getRule(string ID)
{
	ULOG_DBG("Looking for rule: %s",ID.c_str());
	ULOG_DBG("The graph contains the following rules: ");
	list<Rule>::iterator it  = rules.begin();
	for(; it != rules.end(); it++)
	{
		ULOG_DBG("\t%s",it->getID().c_str());
	}

	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		if(it->getID() == ID)
			return *it;
	}

	//The rule searched does not exist in this graph
	throw new GraphException();
}

void Graph::removeRules(list<Rule> rules)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
		removeRule(*r);
}

void Graph::removeRule(Rule rule)
{
	ULOG_DBG("Removing rule: %s",rule.getID().c_str());
	ULOG_DBG("The graph contains the following rules: ");
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		ULOG_DBG("\t%s",it->getID().c_str());
	}

	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		if(*it == rule)
		{
			rules.erase(it);
			return;
		}
	}

	assert(0);
}

bool Graph::removeRuleFromID(string ID)
{
	ULOG_DBG("Removing rule: %s",ID.c_str());
	ULOG_DBG("The graph contains the following rules: ");
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		ULOG_DBG("\t%s",it->getID().c_str());
	}

	bool retVal = false;
	for(list<Rule>::iterator it = rules.begin(); it != rules.end(); it++)
	{
		if(it->getID() == ID)
		{
			//the rule to remove has been found
			//check if another identical rule exists
			for(list<Rule>::iterator again = rules.begin(); again != rules.end(); again++)
			{
				if(it->getID() != again->getID())
				{
					if(*it == *again)
					{
						//another rule with the same match and action has been found
						retVal = true;
						break;
					}
				}
			}
			rules.erase(it);
			break;
		}
	}

	return retVal;
}

list<Rule> Graph::getRules()
{
	return rules;
}

void Graph::print()
{
	if(LOGGING_LEVEL <= ORCH_DEBUG_INFO)
	{
		cout << "Graph :" << endl << "{" << endl;
		for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
			r->print();
		cout << "}" << endl;
	}
}

void Graph::prettyPrint(LSI *lsi0,map<string,LSI *> lsis)
{
	for(list<Rule>::iterator r = rules.begin(); r != rules.end(); r++)
			r->prettyPrint(lsi0,lsis);
}

}
