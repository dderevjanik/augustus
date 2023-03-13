#include "scenario_events_parameter_data.h"

scenario_condition_data_t *scenario_events_parameter_data_get_conditions_xml_attributes(condition_types type)
{
    return &scenario_condition_data[type];
}

scenario_action_data_t *scenario_events_parameter_data_get_actions_xml_attributes(action_types type)
{
    return &scenario_action_data[type];
}
