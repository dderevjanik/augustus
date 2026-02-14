#ifndef GAME_BATTLEFIELD_H
#define GAME_BATTLEFIELD_H

// Map
#define BATTLEFIELD_MAP_WIDTH 80
#define BATTLEFIELD_MAP_HEIGHT 80

// Player forces
#define BATTLEFIELD_PLAYER_LEGION_COUNT 2
#define BATTLEFIELD_SOLDIERS_PER_LEGION 16

// Enemy forces
#define BATTLEFIELD_ENEMY_FORMATION_COUNT 2
#define BATTLEFIELD_ENEMIES_PER_FORMATION 16

// Spawn positions (player on left, enemies on right)
#define BATTLEFIELD_PLAYER_X 20
#define BATTLEFIELD_PLAYER_Y 36
#define BATTLEFIELD_PLAYER_Y_SPACING 8

#define BATTLEFIELD_ENEMY_X 60
#define BATTLEFIELD_ENEMY_Y 36
#define BATTLEFIELD_ENEMY_Y_SPACING 8

void battlefield_start(void);
void battlefield_stop(void);
int battlefield_is_active(void);
void battlefield_check_completion(void);

#endif // GAME_BATTLEFIELD_H
