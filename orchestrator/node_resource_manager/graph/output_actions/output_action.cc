#include "output_action.h"

OutputAction::OutputAction(temp_action_t type)
{
    this->type=type;
}

OutputAction::~OutputAction()
{

}

temp_action_t OutputAction::getType()
{
    return type;
}