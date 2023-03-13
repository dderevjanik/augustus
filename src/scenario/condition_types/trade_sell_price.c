#include "trade_sell_price.h"

#include "empire/trade_prices.h"
#include "scenario/condition_types/comparison_helper.h"

int scenario_condition_type_trade_sell_price_met(scenario_condition_t *condition)
{
    int32_t resource = condition->parameter1;
    int type = condition->parameter2;
    int32_t value = condition->parameter3;

    int trade_sell_price = trade_price_base_sell(resource);
    return comparison_helper_compare_values(type, trade_sell_price, (int)value);
}
