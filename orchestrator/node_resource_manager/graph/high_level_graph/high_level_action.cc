#include "high_level_action.h"

namespace highlevel
{

Action::Action()
{

}

Action::~Action()
{

}

void Action::addGenericAction(GenericAction *ga)
{
	genericActions.push_back(ga);
}

list<GenericAction*> Action::getGenericActions()
{
	return genericActions;
}

void Action::addOutputAction(OutputAction *oa)
{
	outputActions.push_back(oa);
}

list<OutputAction*> Action::getOutputActions()
{
	return outputActions;
}

Array Action::toJSON()
{
	Array action;
	for(list<OutputAction*>::iterator oa = outputActions.begin(); oa != outputActions.end(); oa++)
	{
		action.push_back((*oa)->toJSON());
	}
	for(list<GenericAction*>::iterator ga = genericActions.begin(); ga != genericActions.end(); ga++)
	{
		(*ga)->toJSON(action);
	}
	return action;
}



}