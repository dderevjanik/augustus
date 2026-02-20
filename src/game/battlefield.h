#ifndef GAME_BATTLEFIELD_H
#define GAME_BATTLEFIELD_H

#include "figure/type.h"

// Map
#define BATTLEFIELD_MAP_WIDTH 80
#define BATTLEFIELD_MAP_HEIGHT 80

// Default forces
#define BATTLEFIELD_DEFAULT_LEGION_COUNT 2
#define BATTLEFIELD_DEFAULT_SOLDIERS_PER_LEGION 16
#define BATTLEFIELD_DEFAULT_ENEMY_FORMATION_COUNT 2
#define BATTLEFIELD_DEFAULT_ENEMIES_PER_FORMATION 16

// Default spawn positions (player on left, enemies on right)
#define BATTLEFIELD_DEFAULT_PLAYER_X 20
#define BATTLEFIELD_DEFAULT_PLAYER_Y 36
#define BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING 8

#define BATTLEFIELD_DEFAULT_ENEMY_X 60
#define BATTLEFIELD_DEFAULT_ENEMY_Y 36
#define BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING 8

#define BATTLEFIELD_MAX_ARMIES 10

typedef struct {
    figure_type figure_type;
    int count;
    int soldiers;
    int x;
    int y;
    int y_spacing;
} battlefield_army;

typedef struct {
    int player_army_count;
    battlefield_army player_armies[BATTLEFIELD_MAX_ARMIES];
    int enemy_army_count;
    battlefield_army enemy_armies[BATTLEFIELD_MAX_ARMIES];
    int enemy_id;
} battlefield_config;

void battlefield_start(void);
void battlefield_start_configured(const battlefield_config *config);
void battlefield_start_from_map(const char *filename, const battlefield_config *config);
void battlefield_stop(void);
int battlefield_is_active(void);
int battlefield_should_stop(void);
void battlefield_check_completion(void);
void battlefield_process_pending_lua(void);
int battlefield_spawn_enemies(figure_type type, int count, int soldiers, int x, int y, int y_spacing);

#endif // GAME_BATTLEFIELD_H
