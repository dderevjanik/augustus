#include "gladiator_revolt.h"

#include "scenario/gladiator_revolt.h"

int scenario_action_type_gladiator_revolt_execute(scenario_action_t *action)
{
    scenario_gladiator_revolt_start_new();

    return 1;
}
