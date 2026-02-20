#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "city/victory.h"
#include "figure/type.h"
#include "game/battlefield.h"
#include "game/settings.h"
#include "game/time.h"
#include "scenario/lua/lua_state.h"

#include <string.h>

static int l_game_year(lua_State *L)
{
    lua_pushinteger(L, game_time_year());
    return 1;
}

static int l_game_month(lua_State *L)
{
    lua_pushinteger(L, game_time_month());
    return 1;
}

static int l_game_day(lua_State *L)
{
    lua_pushinteger(L, game_time_day());
    return 1;
}

static int l_game_tick(lua_State *L)
{
    lua_pushinteger(L, game_time_tick());
    return 1;
}

static int l_game_speed(lua_State *L)
{
    lua_pushinteger(L, setting_game_speed());
    return 1;
}

static int l_game_set_speed(lua_State *L)
{
    int speed = (int) luaL_checkinteger(L, 1);
    setting_set_game_speed(speed);
    return 0;
}

static int l_game_win(lua_State *L)
{
    city_victory_force_win();
    return 0;
}

static int l_game_lose(lua_State *L)
{
    city_victory_force_lose();
    return 0;
}

static int l_game_api_version(lua_State *L)
{
    lua_pushinteger(L, LUA_API_VERSION);
    return 1;
}

// Helper: parse an army table from a Lua array element
static void parse_army_table(lua_State *L, int index, battlefield_army *army,
    int default_x, int default_y, int default_spacing,
    int default_soldiers, figure_type default_figure)
{
    army->figure_type = default_figure;
    army->count = 1;
    army->soldiers = default_soldiers;
    army->x = default_x;
    army->y = default_y;
    army->y_spacing = default_spacing;

    if (!lua_istable(L, index)) {
        return;
    }

    lua_getfield(L, index, "figure_type");
    if (!lua_isnil(L, -1)) {
        army->figure_type = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "count");
    if (!lua_isnil(L, -1)) {
        army->count = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "soldiers");
    if (!lua_isnil(L, -1)) {
        army->soldiers = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "x");
    if (!lua_isnil(L, -1)) {
        army->x = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "y");
    if (!lua_isnil(L, -1)) {
        army->y = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "y_spacing");
    if (!lua_isnil(L, -1)) {
        army->y_spacing = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
}

// game.battlefield_start(config?)
// config is optional. If nil or absent, uses defaults.
static int l_game_battlefield_start(lua_State *L)
{
    if (lua_gettop(L) == 0 || lua_isnil(L, 1)) {
        battlefield_start();
        return 0;
    }

    luaL_checktype(L, 1, LUA_TTABLE);

    battlefield_config config;
    memset(&config, 0, sizeof(config));

    // enemy_id (default ENEMY_0_BARBARIAN = 0)
    lua_getfield(L, 1, "enemy_id");
    config.enemy_id = lua_isnil(L, -1) ? 0 : (int) lua_tointeger(L, -1);
    lua_pop(L, 1);

    // player_armies array
    lua_getfield(L, 1, "player_armies");
    if (lua_istable(L, -1)) {
        int n = (int) lua_rawlen(L, -1);
        if (n > BATTLEFIELD_MAX_ARMIES) {
            n = BATTLEFIELD_MAX_ARMIES;
        }
        config.player_army_count = n;
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            parse_army_table(L, lua_gettop(L), &config.player_armies[i - 1],
                BATTLEFIELD_DEFAULT_PLAYER_X,
                BATTLEFIELD_DEFAULT_PLAYER_Y + (i - 1) * BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING,
                BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING,
                BATTLEFIELD_DEFAULT_SOLDIERS_PER_LEGION,
                FIGURE_FORT_LEGIONARY);
            lua_pop(L, 1);
        }
    } else {
        // No player armies specified — use default
        config.player_army_count = 1;
        config.player_armies[0].figure_type = FIGURE_FORT_LEGIONARY;
        config.player_armies[0].count = BATTLEFIELD_DEFAULT_LEGION_COUNT;
        config.player_armies[0].soldiers = BATTLEFIELD_DEFAULT_SOLDIERS_PER_LEGION;
        config.player_armies[0].x = BATTLEFIELD_DEFAULT_PLAYER_X;
        config.player_armies[0].y = BATTLEFIELD_DEFAULT_PLAYER_Y;
        config.player_armies[0].y_spacing = BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING;
    }
    lua_pop(L, 1);

    // enemy_armies array
    lua_getfield(L, 1, "enemy_armies");
    if (lua_istable(L, -1)) {
        int n = (int) lua_rawlen(L, -1);
        if (n > BATTLEFIELD_MAX_ARMIES) {
            n = BATTLEFIELD_MAX_ARMIES;
        }
        config.enemy_army_count = n;
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            parse_army_table(L, lua_gettop(L), &config.enemy_armies[i - 1],
                BATTLEFIELD_DEFAULT_ENEMY_X,
                BATTLEFIELD_DEFAULT_ENEMY_Y + (i - 1) * BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING,
                BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING,
                BATTLEFIELD_DEFAULT_ENEMIES_PER_FORMATION,
                FIGURE_ENEMY49_FAST_SWORD);
            lua_pop(L, 1);
        }
    } else {
        // No enemy armies specified — use default
        config.enemy_army_count = 1;
        config.enemy_armies[0].figure_type = FIGURE_ENEMY49_FAST_SWORD;
        config.enemy_armies[0].count = BATTLEFIELD_DEFAULT_ENEMY_FORMATION_COUNT;
        config.enemy_armies[0].soldiers = BATTLEFIELD_DEFAULT_ENEMIES_PER_FORMATION;
        config.enemy_armies[0].x = BATTLEFIELD_DEFAULT_ENEMY_X;
        config.enemy_armies[0].y = BATTLEFIELD_DEFAULT_ENEMY_Y;
        config.enemy_armies[0].y_spacing = BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING;
    }
    lua_pop(L, 1);

    battlefield_start_configured(&config);
    return 0;
}

// game.battlefield_start_from_map(filename, config?)
// Loads an existing map file (.svx, .sav, .map, .mapx) and starts battlefield on it.
// Optional second argument is a config table (same format as battlefield_start).
static int l_game_battlefield_start_from_map(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);

    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        battlefield_config config;
        memset(&config, 0, sizeof(config));

        lua_getfield(L, 2, "enemy_id");
        config.enemy_id = lua_isnil(L, -1) ? 0 : (int) lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, 2, "player_armies");
        if (lua_istable(L, -1)) {
            int n = (int) lua_rawlen(L, -1);
            if (n > BATTLEFIELD_MAX_ARMIES) n = BATTLEFIELD_MAX_ARMIES;
            config.player_army_count = n;
            for (int i = 1; i <= n; i++) {
                lua_rawgeti(L, -1, i);
                parse_army_table(L, lua_gettop(L), &config.player_armies[i - 1],
                    BATTLEFIELD_DEFAULT_PLAYER_X,
                    BATTLEFIELD_DEFAULT_PLAYER_Y + (i - 1) * BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING,
                    BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING,
                    BATTLEFIELD_DEFAULT_SOLDIERS_PER_LEGION,
                    FIGURE_FORT_LEGIONARY);
                lua_pop(L, 1);
            }
        } else {
            config.player_army_count = 1;
            config.player_armies[0].figure_type = FIGURE_FORT_LEGIONARY;
            config.player_armies[0].count = BATTLEFIELD_DEFAULT_LEGION_COUNT;
            config.player_armies[0].soldiers = BATTLEFIELD_DEFAULT_SOLDIERS_PER_LEGION;
            config.player_armies[0].x = BATTLEFIELD_DEFAULT_PLAYER_X;
            config.player_armies[0].y = BATTLEFIELD_DEFAULT_PLAYER_Y;
            config.player_armies[0].y_spacing = BATTLEFIELD_DEFAULT_PLAYER_Y_SPACING;
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "enemy_armies");
        if (lua_istable(L, -1)) {
            int n = (int) lua_rawlen(L, -1);
            if (n > BATTLEFIELD_MAX_ARMIES) n = BATTLEFIELD_MAX_ARMIES;
            config.enemy_army_count = n;
            for (int i = 1; i <= n; i++) {
                lua_rawgeti(L, -1, i);
                parse_army_table(L, lua_gettop(L), &config.enemy_armies[i - 1],
                    BATTLEFIELD_DEFAULT_ENEMY_X,
                    BATTLEFIELD_DEFAULT_ENEMY_Y + (i - 1) * BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING,
                    BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING,
                    BATTLEFIELD_DEFAULT_ENEMIES_PER_FORMATION,
                    FIGURE_ENEMY49_FAST_SWORD);
                lua_pop(L, 1);
            }
        } else {
            config.enemy_army_count = 1;
            config.enemy_armies[0].figure_type = FIGURE_ENEMY49_FAST_SWORD;
            config.enemy_armies[0].count = BATTLEFIELD_DEFAULT_ENEMY_FORMATION_COUNT;
            config.enemy_armies[0].soldiers = BATTLEFIELD_DEFAULT_ENEMIES_PER_FORMATION;
            config.enemy_armies[0].x = BATTLEFIELD_DEFAULT_ENEMY_X;
            config.enemy_armies[0].y = BATTLEFIELD_DEFAULT_ENEMY_Y;
            config.enemy_armies[0].y_spacing = BATTLEFIELD_DEFAULT_ENEMY_Y_SPACING;
        }
        lua_pop(L, 1);

        battlefield_start_from_map(filename, &config);
    } else {
        battlefield_start_from_map(filename, 0);
    }
    return 0;
}

// game.battlefield_stop()
static int l_game_battlefield_stop(lua_State *L)
{
    battlefield_stop();
    return 0;
}

// game.battlefield_is_active() -> boolean
static int l_game_battlefield_is_active(lua_State *L)
{
    lua_pushboolean(L, battlefield_is_active());
    return 1;
}

static const luaL_Reg game_funcs[] = {
    {"year", l_game_year},
    {"month", l_game_month},
    {"day", l_game_day},
    {"tick", l_game_tick},
    {"speed", l_game_speed},
    {"set_speed", l_game_set_speed},
    {"win", l_game_win},
    {"lose", l_game_lose},
    {"api_version", l_game_api_version},
    // Battlefield
    {"battlefield_start",          l_game_battlefield_start},
    {"battlefield_start_from_map",  l_game_battlefield_start_from_map},
    {"battlefield_stop",           l_game_battlefield_stop},
    {"battlefield_is_active",      l_game_battlefield_is_active},
    {NULL, NULL}
};

void lua_api_game_register(lua_State *L)
{
    luaL_newlib(L, game_funcs);

    // game.FIGURE_TYPE.* constants
    lua_newtable(L);
    lua_pushinteger(L, FIGURE_FORT_LEGIONARY);
    lua_setfield(L, -2, "LEGIONARY");
    lua_pushinteger(L, FIGURE_FORT_JAVELIN);
    lua_setfield(L, -2, "JAVELIN");
    lua_pushinteger(L, FIGURE_FORT_MOUNTED);
    lua_setfield(L, -2, "MOUNTED");
    lua_pushinteger(L, FIGURE_FORT_INFANTRY);
    lua_setfield(L, -2, "INFANTRY");
    lua_pushinteger(L, FIGURE_FORT_ARCHER);
    lua_setfield(L, -2, "ARCHER");
    lua_pushinteger(L, FIGURE_ENEMY43_SPEAR);
    lua_setfield(L, -2, "ENEMY_SPEAR");
    lua_pushinteger(L, FIGURE_ENEMY44_SWORD);
    lua_setfield(L, -2, "ENEMY_SWORD");
    lua_pushinteger(L, FIGURE_ENEMY45_SWORD);
    lua_setfield(L, -2, "ENEMY_SWORD_2");
    lua_pushinteger(L, FIGURE_ENEMY46_CAMEL);
    lua_setfield(L, -2, "ENEMY_CAMEL");
    lua_pushinteger(L, FIGURE_ENEMY47_ELEPHANT);
    lua_setfield(L, -2, "ENEMY_ELEPHANT");
    lua_pushinteger(L, FIGURE_ENEMY48_CHARIOT);
    lua_setfield(L, -2, "ENEMY_CHARIOT");
    lua_pushinteger(L, FIGURE_ENEMY49_FAST_SWORD);
    lua_setfield(L, -2, "ENEMY_FAST_SWORD");
    lua_pushinteger(L, FIGURE_ENEMY50_SWORD);
    lua_setfield(L, -2, "ENEMY_SWORD_3");
    lua_pushinteger(L, FIGURE_ENEMY51_SPEAR);
    lua_setfield(L, -2, "ENEMY_SPEAR_2");
    lua_pushinteger(L, FIGURE_ENEMY52_MOUNTED_ARCHER);
    lua_setfield(L, -2, "ENEMY_MOUNTED_ARCHER");
    lua_pushinteger(L, FIGURE_ENEMY53_AXE);
    lua_setfield(L, -2, "ENEMY_AXE");
    lua_pushinteger(L, FIGURE_ENEMY54_GLADIATOR);
    lua_setfield(L, -2, "ENEMY_GLADIATOR");
    lua_setfield(L, -2, "FIGURE_TYPE");

    // game.ENEMY_TYPE.* constants
    lua_newtable(L);
    lua_pushinteger(L, ENEMY_0_BARBARIAN);
    lua_setfield(L, -2, "BARBARIAN");
    lua_pushinteger(L, ENEMY_1_NUMIDIAN);
    lua_setfield(L, -2, "NUMIDIAN");
    lua_pushinteger(L, ENEMY_2_GAUL);
    lua_setfield(L, -2, "GAUL");
    lua_pushinteger(L, ENEMY_3_CELT);
    lua_setfield(L, -2, "CELT");
    lua_pushinteger(L, ENEMY_4_GOTH);
    lua_setfield(L, -2, "GOTH");
    lua_pushinteger(L, ENEMY_5_PERGAMUM);
    lua_setfield(L, -2, "PERGAMUM");
    lua_pushinteger(L, ENEMY_6_SELEUCID);
    lua_setfield(L, -2, "SELEUCID");
    lua_pushinteger(L, ENEMY_7_ETRUSCAN);
    lua_setfield(L, -2, "ETRUSCAN");
    lua_pushinteger(L, ENEMY_8_GREEK);
    lua_setfield(L, -2, "GREEK");
    lua_pushinteger(L, ENEMY_9_EGYPTIAN);
    lua_setfield(L, -2, "EGYPTIAN");
    lua_pushinteger(L, ENEMY_10_CARTHAGINIAN);
    lua_setfield(L, -2, "CARTHAGINIAN");
    lua_pushinteger(L, ENEMY_11_CAESAR);
    lua_setfield(L, -2, "CAESAR");
    lua_setfield(L, -2, "ENEMY_TYPE");

    lua_setglobal(L, "game");
}
