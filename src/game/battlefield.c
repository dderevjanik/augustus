#include "battlefield.h"

#include "building/construction.h"
#include "building/menu.h"
#include "building/monument.h"
#include "building/storage.h"
#include "city/data.h"
#include "city/map.h"
#include "city/message.h"
#include "city/military.h"
#include "city/view.h"
#include "city/victory.h"
#include "core/image.h"
#include "core/log.h"
#include "core/terminal.h"
#include "figure/enemy_army.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/name.h"
#include "figure/route.h"
#include "figure/visited_buildings.h"
#include "core/dir.h"
#include "game/animation.h"
#include "game/file.h"
#include "game/state.h"
#include "game/time.h"
#include "map/aqueduct.h"
#include "map/building.h"
#include "map/desirability.h"
#include "map/elevation.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/image_context.h"
#include "map/property.h"
#include "map/random.h"
#include "map/road_network.h"
#include "map/routing_terrain.h"
#include "map/soldier_strength.h"
#include "map/sprite.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "scenario/map.h"
#include "scenario/property.h"
#include "scenario/scenario.h"
#include "scenario/lua/lua_state.h"
#include "sound/city.h"
#include "window/city.h"

#include <string.h>

static int is_active;
static int has_backup;

static const char BACKUP_FILENAME[] = "battlefield-backup.svx";

int battlefield_is_active(void)
{
    return is_active;
}

void battlefield_stop(void)
{
    is_active = 0;
    if (has_backup) {
        has_backup = 0;
        terminal_add_line("[battlefield] Restoring previous scenario...");
        const char *path = dir_append_location(BACKUP_FILENAME, PATH_LOCATION_SAVEGAME);
        if (game_file_load_saved_game(path) == 1) {
            terminal_add_line("[battlefield] Scenario restored.");
        } else {
            terminal_add_line("[error] Failed to restore scenario from backup.");
        }
    }
}

void battlefield_check_completion(void)
{
    if (!is_active) {
        return;
    }
    if (enemy_army_total_enemy_formations() <= 0) {
        terminal_add_line("[battlefield] Victory! All enemies defeated.");
        battlefield_stop();
    }
}

static void clear_battlefield_data(void)
{
    scenario_lua_shutdown();

    city_victory_reset();
    building_construction_clear_type();
    city_data_init();
    city_message_init_scenario();
    game_state_init();
    game_animation_init();
    sound_city_init();
    building_menu_enable_all();
    building_clear_all();
    building_storage_clear_all();
    figure_init_scenario();
    enemy_armies_clear();
    figure_name_init();
    formations_clear();
    building_monument_initialize_deliveries();
    figure_route_clear_all();
    figure_visited_buildings_init();

    game_time_init(2098);

    // Clear all map grids
    map_image_clear();
    map_building_clear();
    map_terrain_clear();
    map_aqueduct_clear();
    map_figure_clear();
    map_property_clear();
    map_sprite_clear();
    map_random_clear();
    map_desirability_clear();
    map_elevation_clear();
    map_soldier_strength_clear();
    map_road_network_clear();

    map_image_context_init();
    map_random_init();
}

static void setup_map(void)
{
    // Set scenario map dimensions (same pattern as scenario_editor_create)
    scenario.map.width = BATTLEFIELD_MAP_WIDTH;
    scenario.map.height = BATTLEFIELD_MAP_HEIGHT;
    scenario.map.grid_border_size = GRID_SIZE - BATTLEFIELD_MAP_WIDTH;
    scenario.map.grid_start =
        (GRID_SIZE - BATTLEFIELD_MAP_HEIGHT) / 2 * GRID_SIZE +
        (GRID_SIZE - BATTLEFIELD_MAP_WIDTH) / 2;

    // Set entry/exit at map edges (needed for enemy retreat paths)
    scenario.entry_point.x = BATTLEFIELD_MAP_WIDTH - 1;
    scenario.entry_point.y = BATTLEFIELD_MAP_HEIGHT / 2;
    scenario.exit_point.x = 0;
    scenario.exit_point.y = BATTLEFIELD_MAP_HEIGHT / 2;

    // Set climate to central
    scenario_change_climate(CLIMATE_CENTRAL);
    scenario.enemy_id = ENEMY_0_BARBARIAN;

    // Initialize map grids and terrain
    scenario_map_init();
    map_tiles_update_all_empty_land();

    // Load graphics
    image_load_climate(CLIMATE_CENTRAL, 0, 0, 0);
    image_load_enemy(ENEMY_0_BARBARIAN);

    // Update routing
    map_routing_update_all();

    // Initialize entry/exit flags
    scenario_map_init_entry_exit();

    map_point entry = scenario_map_entry();
    map_point exit = scenario_map_exit();
    city_map_set_entry_point(entry.x, entry.y);
    city_map_set_exit_point(exit.x, exit.y);
}

static void spawn_player_legions(void)
{
    for (int i = 0; i < BATTLEFIELD_PLAYER_LEGION_COUNT; i++) {
        int x = BATTLEFIELD_PLAYER_X;
        int y = BATTLEFIELD_PLAYER_Y + i * BATTLEFIELD_PLAYER_Y_SPACING;

        formation *m = formation_create_legion_at(FIGURE_FORT_LEGIONARY, x, y);
        if (!m || m->id == 0) {
            log_error("Battlefield: failed to create player legion", 0, 0);
            continue;
        }

        int formation_id = m->id;

        // Create standard (flag) figure
        figure *standard = figure_create(FIGURE_FORT_STANDARD, x, y, DIR_4_BOTTOM);
        if (standard && standard->id) {
            standard->formation_id = formation_id;
            standard->formation_at_rest = 0;
            m->standard_figure_id = standard->id;
        }

        // Create soldiers
        for (int s = 0; s < BATTLEFIELD_SOLDIERS_PER_LEGION; s++) {
            figure *f = figure_create(FIGURE_FORT_LEGIONARY, x, y, DIR_4_BOTTOM);
            if (f && f->id) {
                f->formation_id = formation_id;
                f->action_state = FIGURE_ACTION_84_SOLDIER_AT_STANDARD;
                f->formation_at_rest = 0;
            }
        }
    }
}

static void spawn_enemy_forces(void)
{
    for (int i = 0; i < BATTLEFIELD_ENEMY_FORMATION_COUNT; i++) {
        int x = BATTLEFIELD_ENEMY_X;
        int y = BATTLEFIELD_ENEMY_Y + i * BATTLEFIELD_ENEMY_Y_SPACING;

        int formation_id = formation_create_enemy(
            FIGURE_ENEMY49_FAST_SWORD, x, y,
            FORMATION_ENEMY_MOB, DIR_0_TOP,
            ENEMY_0_BARBARIAN, FORMATION_ATTACK_TROOPS,
            0, 0
        );
        if (formation_id <= 0) {
            log_error("Battlefield: failed to create enemy formation", 0, 0);
            continue;
        }

        for (int s = 0; s < BATTLEFIELD_ENEMIES_PER_FORMATION; s++) {
            figure *f = figure_create(FIGURE_ENEMY49_FAST_SWORD, x, y, DIR_0_TOP);
            if (f && f->id) {
                f->faction_id = 0;
                f->is_friendly = 0;
                f->action_state = FIGURE_ACTION_151_ENEMY_INITIAL;
                f->wait_ticks = 10 * s + 10;
                f->formation_id = formation_id;
                f->name = figure_name_get(FIGURE_ENEMY49_FAST_SWORD, ENEMY_0_BARBARIAN);
                f->is_ghost = 1;
            }
        }
    }
}

void battlefield_start(void)
{
    terminal_add_line("[battlefield] Starting battlefield mode...");

    // 0. Save current game state for later restoration
    const char *path = dir_append_location(BACKUP_FILENAME, PATH_LOCATION_SAVEGAME);
    if (game_file_write_saved_game(path)) {
        has_backup = 1;
        terminal_add_line("[battlefield] Scenario saved for later restoration.");
    } else {
        has_backup = 0;
        terminal_add_line("[warning] Could not save scenario backup.");
    }

    // 1. Clear all game state
    clear_battlefield_data();

    // 2. Set up empty flat map
    setup_map();

    // 3. Spawn player legions on the left
    spawn_player_legions();

    // 4. Spawn enemy formations on the right
    spawn_enemy_forces();

    // 5. Link figures to formations
    formation_calculate_figures();

    // 6. Activate battlefield mode
    is_active = 1;

    // 7. Set up view and unpause
    city_view_init();
    game_state_unpause();
    window_city_show();

    terminal_add_line("[battlefield] Battlefield active: 2 player legions vs 2 enemy formations");
}
