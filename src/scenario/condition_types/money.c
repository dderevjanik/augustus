#include "money.h"

#include "city/finance.h"
#include "scenario/condition_types/comparison_helper.h"

int scenario_condition_type_money_met(scenario_condition_t *condition)
{
    int funds = city_finance_treasury();
    int type = condition->parameter1;
    int32_t value = condition->parameter2;

    return comparison_helper_compare_values(type, funds, (int)value);
}
