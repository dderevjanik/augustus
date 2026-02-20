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
#include "editor/editor.h"
#include "game/animation.h"
#include "game/file.h"
#include "game/file_io.h"
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
#include "sound/city.h"
#include "window/city.h"

#include <string.h>

static int is_active;
static int has_backup;
static int pending_stop;

static const char BACKUP_FILENAME[] = "battlefield-backup.svx";

static void save_backup(void)
{
    if (editor_is_active()) {
        // Editor state cannot be saved as a regular .svx savegame
        has_backup = 0;
        terminal_add_line("[battlefield] Started from editor — no backup (cannot auto-restore).");
        return;
    }
    const char *path = dir_append_location(BACKUP_FILENAME, PATH_LOCATION_SAVEGAME);
    if (game_file_write_saved_game(path)) {
        has_backup = 1;
        terminal_add_line("[battlefield] Scenario saved for later restoration.");
    } else {
        has_backup = 0;
        terminal_add_line("[warning] Could not save scenario backup.");
    }
}

int battlefield_is_active(void)
{
    return is_active;
}

void battlefield_stop(void)
{
    is_active = 0;
    pending_stop = 0;
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

int battlefield_should_stop(void)
{
    return pending_stop;
}

void battlefield_check_completion(void)
{
    if (!is_active) {
        return;
    }
    if (enemy_army_total_enemy_formations() <= 0) {
        terminal_add_line("[battlefield] Victory! All enemies defeated.");
        pending_stop = 1;
    }
}

static void clear_battlefield_data(void)
{
    // NOTE: Do NOT call scenario_lua_shutdown() here.
    // battlefield_start may be called from a Lua callback (e.g. input dialog button),
    // and destroying the Lua state mid-execution causes a segfault.
    // The Lua state will be properly restored when battlefield_stop() loads the backup save.

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

static void spawn_player_legion(const battlefield_army *army)
{
    for (int i = 0; i < army->count; i++) {
        int x = army->x;
        int y = army->y + i * army->y_spacing;

        formation *m = formation_create_legion_at(army->figure_type, x, y);
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
        for (int s = 0; s < army->soldiers; s++) {
            figure *f = figure_create(army->figure_type, x, y, DIR_4_BOTTOM);
            if (f && f->id) {
                f->formation_id = formation_id;
                f->action_state = FIGURE_ACTION_84_SOLDIER_AT_STANDARD;
                f->formation_at_rest = 0;
            }
        }
    }
}

static void spawn_enemy_formation(const battlefield_army *army, int enemy_id)
{
    for (int i = 0; i < army->count; i++) {
        int x = army->x;
        int y = army->y + i * army->y_spacing;

        int formation_id = formation_create_enemy(
            army->figure_type, x, y,
            FORMATION_ENEMY_MOB, DIR_0_TOP,
            enemy_id, FORMATION_ATTACK_TROOPS,
            0, 0
        );
        if (formation_id <= 0) {
            log_error("Battlefield: failed to create enemy formation", 0, 0);
            continue;
        }

        for (int s = 0; s < army->soldiers; s++) {
            figure *f = figure_create(army->figure_type, x, y, DIR_0_TOP);
            if (f && f->id) {
                f->faction_id = 0;
                f->is_friendly = 0;
                f->action_state = FIGURE_ACTION_151_ENEMY_INITIAL;
                f->wait_ticks = 10 * s + 10;
                f->formation_id = formation_id;
                f->name = figure_name_get(army->figure_type, enemy_id);
                f->is_ghost = 1;
            }
        }
    }
}

static battlefield_config default_config(void)
{
    battlefield_config config;
    config.enemy_id = ENEMY_0_BARBARIAN;

    config.player_army_count = 1;
    config.player_armies[0].figure_type = FIGURE_FORT_LEGIONARY;
    config.player_armies[0].count = BATTLEFIELD_DEFAULT_LEGION_COUNT;
    config.player_armies[0].soldiers = BATTLEFIELD_DEFAULT_SOLDIERS_PER_LEGION;
    config.player_armies[0].x = BATTLEFIELD_DEFAULT_PLAYER_X;
    config.player_armies[0].y = BATTLEFIELD_DEFAULT_PLAYER_Y;
    config.player_armies[0].y_spacing = BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING;

    config.enemy_army_count = 1;
    config.enemy_armies[0].figure_type = FIGURE_ENEMY49_FAST_SWORD;
    config.enemy_armies[0].count = BATTLEFIELD_DEFAULT_ENEMY_FORMATION_COUNT;
    config.enemy_armies[0].soldiers = BATTLEFIELD_DEFAULT_ENEMIES_PER_FORMATION;
    config.enemy_armies[0].x = BATTLEFIELD_DEFAULT_ENEMY_X;
    config.enemy_armies[0].y = BATTLEFIELD_DEFAULT_ENEMY_Y;
    config.enemy_armies[0].y_spacing = BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING;

    return config;
}

void battlefield_start_configured(const battlefield_config *config)
{
    terminal_add_line("[battlefield] Starting battlefield mode...");

    // 0. Save current game state for later restoration
    save_backup();

    // 1. Clear all game state
    clear_battlefield_data();

    // 2. Set up empty flat map — use config enemy_id for graphics
    scenario_change_climate(CLIMATE_CENTRAL);
    scenario.enemy_id = config->enemy_id;

    scenario.map.width = BATTLEFIELD_MAP_WIDTH;
    scenario.map.height = BATTLEFIELD_MAP_HEIGHT;
    scenario.map.grid_border_size = GRID_SIZE - BATTLEFIELD_MAP_WIDTH;
    scenario.map.grid_start =
        (GRID_SIZE - BATTLEFIELD_MAP_HEIGHT) / 2 * GRID_SIZE +
        (GRID_SIZE - BATTLEFIELD_MAP_WIDTH) / 2;

    scenario.entry_point.x = BATTLEFIELD_MAP_WIDTH - 1;
    scenario.entry_point.y = BATTLEFIELD_MAP_HEIGHT / 2;
    scenario.exit_point.x = 0;
    scenario.exit_point.y = BATTLEFIELD_MAP_HEIGHT / 2;

    scenario_map_init();
    map_tiles_update_all_empty_land();
    image_load_climate(CLIMATE_CENTRAL, 0, 0, 0);
    image_load_enemy(config->enemy_id);
    map_routing_update_all();
    scenario_map_init_entry_exit();

    map_point entry = scenario_map_entry();
    map_point exit = scenario_map_exit();
    city_map_set_entry_point(entry.x, entry.y);
    city_map_set_exit_point(exit.x, exit.y);

    // 3. Spawn player armies
    for (int i = 0; i < config->player_army_count && i < BATTLEFIELD_MAX_ARMIES; i++) {
        spawn_player_legion(&config->player_armies[i]);
    }

    // 4. Spawn enemy armies
    for (int i = 0; i < config->enemy_army_count && i < BATTLEFIELD_MAX_ARMIES; i++) {
        spawn_enemy_formation(&config->enemy_armies[i], config->enemy_id);
    }

    // 5. Link figures to formations
    formation_calculate_figures();

    // 6. Activate battlefield mode
    is_active = 1;

    // 7. Set up view and unpause
    city_view_init();
    game_state_unpause();
    window_city_show();

    terminal_add_line("[battlefield] Battlefield active.");
}

void battlefield_start(void)
{
    battlefield_config config = default_config();
    battlefield_start_configured(&config);
}

static int has_extension(const char *filename, const char *ext)
{
    size_t flen = strlen(filename);
    size_t elen = strlen(ext);
    if (flen < elen) {
        return 0;
    }
    for (size_t i = 0; i < elen; i++) {
        char fc = filename[flen - elen + i];
        char ec = ext[i];
        // case-insensitive
        if (fc >= 'A' && fc <= 'Z') fc += 32;
        if (ec >= 'A' && ec <= 'Z') ec += 32;
        if (fc != ec) return 0;
    }
    return 1;
}

static int load_map_terrain_from_scenario(const char *filename)
{
    // Clear everything first (same as loading a fresh scenario)
    clear_battlefield_data();

    // Load scenario file — populates terrain grids, scenario settings (map dimensions, climate, etc.)
    if (!game_file_io_read_scenario(filename)) {
        return 0;
    }

    // Rebuild terrain tile images (same sequence as initialize_scenario_data in file.c)
    scenario_map_init();
    map_tiles_update_all_elevation();
    map_tiles_update_all_water();
    map_tiles_update_all_earthquake();
    map_tiles_update_all_rocks();
    map_tiles_update_all_empty_land();
    map_tiles_update_all_meadow();
    map_tiles_update_all_rubble();
    map_tiles_update_all_roads();
    map_tiles_update_all_highways();
    map_tiles_update_all_plazas();
    map_tiles_update_all_walls();
    map_tiles_update_all_aqueducts(0);

    image_load_climate(scenario_property_climate(), 0, 0, 0);
    map_routing_update_all();

    scenario_map_init_entry_exit();
    map_point entry = scenario_map_entry();
    map_point exit = scenario_map_exit();
    city_map_set_entry_point(entry.x, entry.y);
    city_map_set_exit_point(exit.x, exit.y);

    return 1;
}

static int load_map_terrain_from_savegame(const char *filename)
{
    // Load the full saved game (terrain + everything)
    if (game_file_load_saved_game(filename) != 1) {
        return 0;
    }

    // Strip buildings, figures, formations — keep terrain
    building_clear_all();
    building_storage_clear_all();
    figure_init_scenario();
    enemy_armies_clear();
    figure_name_init();
    formations_clear();
    building_monument_initialize_deliveries();
    figure_route_clear_all();
    figure_visited_buildings_init();

    map_building_clear();
    map_figure_clear();
    map_sprite_clear();
    map_soldier_strength_clear();

    // Rebuild map images over existing terrain
    map_image_clear();
    map_image_update_all();
    map_routing_update_all();

    return 1;
}

void battlefield_start_from_map(const char *filename, const battlefield_config *config)
{
    if (!filename || strlen(filename) == 0) {
        terminal_add_line("[error] No filename provided.");
        return;
    }

    battlefield_config cfg;
    if (config) {
        cfg = *config;
    } else {
        cfg = default_config();
    }

    terminal_add_line("[battlefield] Starting battlefield from map...");

    // Save backup first
    save_backup();

    int loaded = 0;

    if (has_extension(filename, ".map") || has_extension(filename, ".mapx")) {
        loaded = load_map_terrain_from_scenario(filename);
    } else if (has_extension(filename, ".svx") || has_extension(filename, ".sav")) {
        loaded = load_map_terrain_from_savegame(filename);
    } else {
        // Try savegame first, then scenario
        loaded = load_map_terrain_from_savegame(filename);
        if (!loaded) {
            loaded = load_map_terrain_from_scenario(filename);
        }
    }

    if (!loaded) {
        terminal_add_line("[error] Failed to load map file.");
        // Restore backup if we have one
        if (has_backup) {
            has_backup = 0;
            const char *restore_path = dir_append_location(BACKUP_FILENAME, PATH_LOCATION_SAVEGAME);
            game_file_load_saved_game(restore_path);
        }
        return;
    }

    // Set enemy graphics
    scenario.enemy_id = cfg.enemy_id;
    image_load_enemy(cfg.enemy_id);

    // Spawn player armies
    for (int i = 0; i < cfg.player_army_count && i < BATTLEFIELD_MAX_ARMIES; i++) {
        spawn_player_legion(&cfg.player_armies[i]);
    }

    // Spawn enemy armies
    for (int i = 0; i < cfg.enemy_army_count && i < BATTLEFIELD_MAX_ARMIES; i++) {
        spawn_enemy_formation(&cfg.enemy_armies[i], cfg.enemy_id);
    }

    // Link figures to formations
    formation_calculate_figures();

    // Activate
    is_active = 1;
    city_view_init();
    game_state_unpause();
    window_city_show();

    terminal_add_line("[battlefield] Battlefield active (loaded from map).");
}
