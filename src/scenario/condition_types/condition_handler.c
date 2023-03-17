#include "condition_handler.h"

#include "scenario/condition_types/building_count_active.h"
#include "scenario/condition_types/city_population.h"
#include "scenario/condition_types/count_own_troops.h"
#include "scenario/condition_types/difficulty.h"
#include "scenario/condition_types/money.h"
#include "scenario/condition_types/population_unemployed.h"
#include "scenario/condition_types/request_is_ongoing.h"
#include "scenario/condition_types/rome_wages.h"
#include "scenario/condition_types/savings.h"
#include "scenario/condition_types/stats_city_health.h"
#include "scenario/condition_types/stats_culture.h"
#include "scenario/condition_types/stats_favor.h"
#include "scenario/condition_types/stats_peace.h"
#include "scenario/condition_types/stats_prosperity.h"
#include "scenario/condition_types/time.h"
#include "scenario/condition_types/trade_sell_price.h"

void scenario_condition_type_init(scenario_condition_t *condition)
{
    switch (condition->type) {
        case CONDITION_TYPE_TIME_PASSED:
            scenario_condition_type_time_init(condition);
            break;
        default:
            break;
    }
}

int scenario_condition_type_is_met(scenario_condition_t *condition)
{
    switch (condition->type) {
        case CONDITION_TYPE_BUILDING_COUNT_ACTIVE: return scenario_condition_type_building_count_active_met(condition);
        case CONDITION_TYPE_CITY_POPULATION: return scenario_condition_type_city_population_met(condition);
        case CONDITION_TYPE_COUNT_OWN_TROOPS: return scenario_condition_type_count_own_troops_met(condition);
        case CONDITION_TYPE_DIFFICULTY: return scenario_condition_type_difficulty_met(condition);
        case CONDITION_TYPE_MONEY: return scenario_condition_type_money_met(condition);
        case CONDITION_TYPE_POPS_UNEMPLOYMENT: return scenario_condition_type_population_unemployed_met(condition);
        case CONDITION_TYPE_REQUEST_IS_ONGOING: return scenario_condition_type_request_is_ongoing_met(condition);
        case CONDITION_TYPE_ROME_WAGES: return scenario_condition_type_rome_wages_met(condition);
        case CONDITION_TYPE_SAVINGS: return scenario_condition_type_savings_met(condition);
        case CONDITION_TYPE_STATS_CITY_HEALTH: return scenario_condition_type_stats_city_health_met(condition);
        case CONDITION_TYPE_STATS_CULTURE: return scenario_condition_type_stats_culture_met(condition);
        case CONDITION_TYPE_STATS_FAVOR: return scenario_condition_type_stats_favor_met(condition);
        case CONDITION_TYPE_STATS_PEACE: return scenario_condition_type_stats_peace_met(condition);
        case CONDITION_TYPE_STATS_PROSPERITY: return scenario_condition_type_stats_prosperity_met(condition);
        case CONDITION_TYPE_TIME_PASSED: return scenario_condition_type_time_met(condition);
        case CONDITION_TYPE_TRADE_SELL_PRICE: return scenario_condition_type_trade_sell_price_met(condition);
        default:
            return 0;
            break;
    }
}

void scenario_condition_type_delete(scenario_condition_t *condition)
{
    memset(condition, 0, sizeof(scenario_condition_t));
    condition->type = CONDITION_TYPE_UNDEFINED;
}

void scenario_condition_type_save_state(buffer *buf, scenario_condition_t *condition, int link_type, int32_t link_id)
{
    buffer_write_i16(buf, link_type);
    buffer_write_i32(buf, link_id);
    buffer_write_i16(buf, condition->type);
    buffer_write_i32(buf, condition->parameter1);
    buffer_write_i32(buf, condition->parameter2);
    buffer_write_i32(buf, condition->parameter3);
    buffer_write_i32(buf, condition->parameter4);
    buffer_write_i32(buf, condition->parameter5);
}

void scenario_condition_type_load_state(buffer *buf,
    int version, scenario_condition_t *condition, int *link_type, int32_t *link_id)
{
    *link_type = buffer_read_i16(buf);
    *link_id = buffer_read_i32(buf);
    condition->type = buffer_read_i16(buf);
    condition->parameter1 = buffer_read_i32(buf);
    condition->parameter2 = buffer_read_i32(buf);
    condition->parameter3 = buffer_read_i32(buf);
    condition->parameter4 = buffer_read_i32(buf);
    condition->parameter5 = buffer_read_i32(buf);
}
