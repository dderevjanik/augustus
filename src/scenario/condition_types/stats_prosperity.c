#include "stats_prosperity.h"

#include "city/ratings.h"
#include "scenario/condition_types/comparison_helper.h"

int scenario_condition_type_stats_prosperity_met(scenario_condition_t *condition)
{
    int stat_value = city_rating_prosperity();
    int type = condition->parameter1;
    int32_t value = condition->parameter2;

    return comparison_helper_compare_values(type, stat_value, (int)value);
}
