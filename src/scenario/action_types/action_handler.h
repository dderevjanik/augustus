#ifndef ACTION_TYPES_H
#define ACTION_TYPES_H

#include "core/buffer.h"
#include "scenario/scenario_event_data.h"

void scenario_action_type_init(scenario_action_t *action);
int scenario_action_type_execute(scenario_action_t *action);

void scenario_action_type_save_state(buffer *buf, scenario_action_t *action, int link_type, int32_t link_id);
void scenario_action_type_load_state(buffer *buf, int versio, scenario_action_t *action, int *link_type, int32_t *link_id);

#endif // ACTION_TYPES_H
