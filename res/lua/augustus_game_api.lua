---@meta augustus
-- Augustus Lua Scripting API - Game API
-- Place this file in your workspace for IDE autocomplete. Do NOT load it in scripts.

----------------------------------------------------------------
-- game.*  –  Game time and flow control
----------------------------------------------------------------

---@class game
game = {}

--- Get the current in-game year
---@return integer year
function game.year() end

--- Get the current in-game month (0-11)
---@return integer month
function game.month() end

--- Get the current in-game day (0-15)
---@return integer day
function game.day() end

--- Get the current tick within the day
---@return integer tick
function game.tick() end

--- Get the current game speed setting
---@return integer speed
function game.speed() end

--- Set the game speed
---@param speed integer
function game.set_speed(speed) end

--- Force a scenario victory
function game.win() end

--- Force a scenario defeat
function game.lose() end

--- Get the Lua API version supported by this build of Augustus.
--- Use this to check compatibility and guard against unsupported features.
---@return integer version
function game.api_version() end

----------------------------------------------------------------
-- Battlefield
----------------------------------------------------------------

--- Army configuration for battlefield_start.
---@class battlefield_army
---@field figure_type? integer Figure type constant (default: legionary for player, barbarian sword for enemy)
---@field count? integer Number of formations to create (default: 2)
---@field soldiers? integer Soldiers per formation (default: 16)
---@field x? integer Spawn X position on the map (default: 20 for player, 60 for enemy)
---@field y? integer Spawn Y position of the first formation (default: 36)
---@field y_spacing? integer Vertical spacing between formations (default: 8)

--- Battlefield configuration table.
---@class battlefield_config
---@field enemy_id? integer Enemy graphics set (default: 0 = barbarian). See game.ENEMY_TYPE.*
---@field player_armies? battlefield_army[] Array of player army groups
---@field enemy_armies? battlefield_army[] Array of enemy army groups

--- Start a battlefield battle. Saves the current scenario, creates a flat 80x80 map,
--- and spawns the configured armies. The previous scenario is restored when the
--- battle ends (victory or manual stop).
---
--- When called with no arguments, uses default armies (2 legions vs 2 barbarian formations).
--- Pass a config table to customize armies:
---
--- ```lua
--- game.battlefield_start({
---     enemy_id = game.ENEMY_TYPE.GAUL,
---     player_armies = {
---         { figure_type = game.FIGURE_TYPE.LEGIONARY, count = 3, soldiers = 16, x = 15, y = 30, y_spacing = 10 },
---         { figure_type = game.FIGURE_TYPE.JAVELIN,   count = 1, soldiers = 12, x = 10, y = 50 },
---     },
---     enemy_armies = {
---         { figure_type = game.FIGURE_TYPE.ENEMY_FAST_SWORD, count = 2, soldiers = 20, x = 65, y = 30 },
---         { figure_type = game.FIGURE_TYPE.ENEMY_SPEAR,      count = 2, soldiers = 16, x = 70, y = 50 },
---     },
--- })
--- ```
---@param config? battlefield_config
function game.battlefield_start(config) end

--- Stop an active battlefield and restore the previously saved scenario.
function game.battlefield_stop() end

--- Check whether a battlefield battle is currently active.
---@return boolean is_active
function game.battlefield_is_active() end

----------------------------------------------------------------
-- Battlefield constants
----------------------------------------------------------------

--- Player figure type constants for battlefield armies
---@class game_FIGURE_TYPE
game.FIGURE_TYPE = {
    LEGIONARY  = 13,  -- FIGURE_FORT_LEGIONARY
    JAVELIN    = 11,  -- FIGURE_FORT_JAVELIN
    MOUNTED    = 12,  -- FIGURE_FORT_MOUNTED
    INFANTRY   = 92,  -- FIGURE_FORT_INFANTRY
    ARCHER     = 94,  -- FIGURE_FORT_ARCHER
    -- Enemy figure types
    ENEMY_SPEAR           = 43,  -- FIGURE_ENEMY43_SPEAR
    ENEMY_SWORD           = 44,  -- FIGURE_ENEMY44_SWORD
    ENEMY_SWORD_2         = 45,  -- FIGURE_ENEMY45_SWORD
    ENEMY_CAMEL           = 46,  -- FIGURE_ENEMY46_CAMEL
    ENEMY_ELEPHANT        = 47,  -- FIGURE_ENEMY47_ELEPHANT
    ENEMY_CHARIOT         = 48,  -- FIGURE_ENEMY48_CHARIOT
    ENEMY_FAST_SWORD      = 49,  -- FIGURE_ENEMY49_FAST_SWORD
    ENEMY_SWORD_3         = 50,  -- FIGURE_ENEMY50_SWORD
    ENEMY_SPEAR_2         = 51,  -- FIGURE_ENEMY51_SPEAR
    ENEMY_MOUNTED_ARCHER  = 52,  -- FIGURE_ENEMY52_MOUNTED_ARCHER
    ENEMY_AXE             = 53,  -- FIGURE_ENEMY53_AXE
    ENEMY_GLADIATOR       = 54,  -- FIGURE_ENEMY54_GLADIATOR
}

--- Enemy graphics set constants (controls which enemy sprite sheet is loaded)
---@class game_ENEMY_TYPE
game.ENEMY_TYPE = {
    BARBARIAN     = 0,
    NUMIDIAN      = 1,
    GAUL          = 2,
    CELT          = 3,
    GOTH          = 4,
    PERGAMUM      = 5,
    SELEUCID      = 6,
    ETRUSCAN      = 7,
    GREEK         = 8,
    EGYPTIAN      = 9,
    CARTHAGINIAN  = 10,
    CAESAR        = 11,
}
