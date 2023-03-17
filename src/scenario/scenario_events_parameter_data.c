#include "scenario_events_parameter_data.h"
#include "game/resource.h"

scenario_condition_data_t *scenario_events_parameter_data_get_conditions_xml_attributes(condition_types type)
{
    return &scenario_condition_data[type];
}

scenario_action_data_t *scenario_events_parameter_data_get_actions_xml_attributes(action_types type)
{
    return &scenario_action_data[type];
}

int scenario_events_parameter_data_get_default_value_for_parameter(xml_data_attribute_t *attribute_data)
{
    switch (attribute_data->type) {
        case PARAMETER_TYPE_NUMBER:
            if (attribute_data->min_limit > 0) {
                return attribute_data->min_limit;
            } else {
                if (attribute_data->max_limit < 0) {
                    return attribute_data->max_limit;
                }
                return 0;
            }
        case PARAMETER_TYPE_CHECK: return COMPARISON_TYPE_EQUAL;
        case PARAMETER_TYPE_DIFFICULTY: return DIFFICULTY_NORMAL;
        case PARAMETER_TYPE_RESOURCE: return RESOURCE_WHEAT;
        case PARAMETER_TYPE_POP_CLASS: return POP_CLASS_ALL;
        case PARAMETER_TYPE_BUILDING: return BUILDING_WELL;
        case PARAMETER_TYPE_BUILDING_COUNTING: return BUILDING_WELL;
        case PARAMETER_TYPE_ALLOWED_BUILDING: return ALLOWED_BUILDING_FARMS;
        case PARAMETER_TYPE_STANDARD_MESSAGE: return MESSAGE_CAESAR_WRATH;
        default:
            return 0;
            break;
    }
}

special_attribute_mapping_t *scenario_events_parameter_data_get_attribute_mapping_by_type_and_value(parameter_type type, int search_value)
{
    special_attribute_mapping_t *mapping_list;
    int list_size = 0;
    
    switch (type) {
        case PARAMETER_TYPE_CHECK:
            list_size = SPECIAL_ATTRIBUTE_MAPPINGS_CHECK_SIZE;
            mapping_list = special_attribute_mappings_check;
            break;
        case PARAMETER_TYPE_DIFFICULTY:
            list_size = SPECIAL_ATTRIBUTE_MAPPINGS_CHECK_DIFFICULTY;
            mapping_list = special_attribute_mappings_difficulty;
            break;
        case PARAMETER_TYPE_BOOLEAN:
            list_size = SPECIAL_ATTRIBUTE_MAPPINGS_BOOLEAN_SIZE;
            mapping_list = special_attribute_mappings_boolean;
            break;
        case PARAMETER_TYPE_POP_CLASS:
            list_size = SPECIAL_ATTRIBUTE_MAPPINGS_POP_CLASS_SIZE;
            mapping_list = special_attribute_mappings_pop_class;
            break;
        case PARAMETER_TYPE_BUILDING:
        case PARAMETER_TYPE_BUILDING_COUNTING:
            list_size = BUILDING_TYPE_MAX;
            mapping_list = special_attribute_mappings_buildings;
            break;
        case PARAMETER_TYPE_ALLOWED_BUILDING:
            list_size = SPECIAL_ATTRIBUTE_MAPPINGS_ALLOWED_BUILDINGS_SIZE;
            mapping_list = special_attribute_mappings_allowed_buildings;
            break;
        case PARAMETER_TYPE_STANDARD_MESSAGE:
            list_size = SPECIAL_ATTRIBUTE_MAPPINGS_STANDARD_MESSAGE_SIZE;
            mapping_list = special_attribute_mappings_standard_message;
            break;
        default:
            {
                special_attribute_mapping_t nothing = { .type = PARAMETER_TYPE_UNDEFINED, .text = "", .value = 0, .key = 0 };
                return &nothing;
            }
            break;
    }
    for (int i = 0; i < list_size; i++) {
        if (mapping_list[i].value == search_value) {
            return &mapping_list[i];
        }
    }

    special_attribute_mapping_t nothing = { .type = PARAMETER_TYPE_UNDEFINED, .text = "", .value = 0, .key = 0 };
    return &nothing;
}
