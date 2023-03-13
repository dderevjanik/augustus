#include "stats_culture.h"

#include "city/ratings.h"
#include "scenario/condition_types/comparison_helper.h"

int scenario_condition_type_stats_culture_met(scenario_condition_t *condition)
{
    int stat_value = city_rating_culture();
    int type = condition->parameter1;
    int32_t value = condition->parameter2;

    return comparison_helper_compare_values(type, stat_value, (int)value);
}
