#include "stats_city_health.h"

#include "city/health.h"
#include "scenario/condition_types/comparison_helper.h"

int scenario_condition_type_stats_city_health_met(scenario_condition_t *condition)
{
    int stat_value = city_health();
    int type = condition->parameter1;
    int32_t value = condition->parameter2;

    return comparison_helper_compare_values(type, stat_value, (int)value);
}
