#include "output_action.h"

OutputAction::OutputAction(output_action_t type)
{
    this->type=type;
}

OutputAction::~OutputAction()
{

}

output_action_t OutputAction::getType()
{
    return type;
}