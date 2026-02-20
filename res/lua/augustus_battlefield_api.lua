---@meta augustus_battlefield
-- Augustus Lua Scripting API - Battlefield API
-- Place this file in your workspace for IDE autocomplete. Do NOT load it in scripts.

----------------------------------------------------------------
-- battlefield.*  –  Battlefield mode control
----------------------------------------------------------------

---@class battlefield
battlefield = {}

--- Army configuration for battlefield.start.
---@class battlefield_army
---@field figure_type? integer Figure type constant (default: legionary for player, barbarian sword for enemy)
---@field count? integer Number of formations to create (default: 2)
---@field soldiers? integer Soldiers per formation (default: 16)
---@field x? integer Spawn X position on the map (default: 20 for player, 60 for enemy)
---@field y? integer Spawn Y position of the first formation (default: 36)
---@field y_spacing? integer Vertical spacing between formations (default: 8)

--- Battlefield configuration table.
---@class battlefield_config
---@field enemy_id? integer Enemy graphics set (default: 0 = barbarian). See battlefield.ENEMY_TYPE.*
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
--- battlefield.start({
---     enemy_id = battlefield.ENEMY_TYPE.GAUL,
---     player_armies = {
---         { figure_type = battlefield.FIGURE_TYPE.LEGIONARY, count = 3, soldiers = 16, x = 15, y = 30, y_spacing = 10 },
---         { figure_type = battlefield.FIGURE_TYPE.JAVELIN,   count = 1, soldiers = 12, x = 10, y = 50 },
---     },
---     enemy_armies = {
---         { figure_type = battlefield.FIGURE_TYPE.ENEMY_FAST_SWORD, count = 2, soldiers = 20, x = 65, y = 30 },
---         { figure_type = battlefield.FIGURE_TYPE.ENEMY_SPEAR,      count = 2, soldiers = 16, x = 70, y = 50 },
---     },
--- })
--- ```
---@param config? battlefield_config
function battlefield.start(config) end

--- Start a battlefield battle using an existing map file as terrain.
--- Loads the specified file (.svx, .sav, .map, or .mapx), strips all buildings
--- and figures, keeps the terrain, and spawns battlefield armies.
--- The previous scenario is saved and will be restored when the battle ends.
---
--- Pass an optional config table (same format as battlefield.start) to customize armies.
--- If omitted, default armies are used.
---
--- ```lua
--- -- With defaults:
--- battlefield.start_from_map("path/to/map.svx")
---
--- -- With custom config:
--- battlefield.start_from_map("path/to/map.svx", {
---     enemy_id = battlefield.ENEMY_TYPE.GAUL,
---     player_armies = {
---         { figure_type = battlefield.FIGURE_TYPE.LEGIONARY, count = 2, soldiers = 16, x = 20, y = 30 },
---     },
---     enemy_armies = {
---         { figure_type = battlefield.FIGURE_TYPE.ENEMY_FAST_SWORD, count = 3, soldiers = 16, x = 60, y = 30 },
---     },
--- })
--- ```
---@param filename string Path to the map file to load
---@param config? battlefield_config Optional army configuration
function battlefield.start_from_map(filename, config) end

--- Stop an active battlefield and restore the previously saved scenario.
function battlefield.stop() end

--- Check whether a battlefield battle is currently active.
---@return boolean is_active
function battlefield.is_active() end

--- Spawn additional enemy formations on an active battlefield.
--- The battlefield must be active (started via battlefield.start or battlefield.start_from_map).
--- Uses the same army table format as the armies in battlefield_config.
---
--- ```lua
--- battlefield.spawn_enemies({
---     figure_type = battlefield.FIGURE_TYPE.ENEMY_FAST_SWORD,
---     count = 2,       -- number of formations
---     soldiers = 12,   -- soldiers per formation
---     x = 55,          -- map x position
---     y = 20,          -- map y position
---     y_spacing = 6,   -- vertical spacing between formations
--- })
--- ```
---@param army battlefield_army Army group to spawn
---@return boolean success Whether the enemies were spawned successfully
function battlefield.spawn_enemies(army) end

--- End the battlefield with a victory.
--- Fires the `on_victory()` hook on the battlefield Lua script (if defined),
--- then destroys the battlefield script and restores the previous scenario.
--- Deferred to the next tick so it is safe to call from inside a Lua callback.
function battlefield.win() end

--- End the battlefield with a defeat.
--- Fires the `on_defeat()` hook on the battlefield Lua script (if defined),
--- then destroys the battlefield script and restores the previous scenario.
--- Deferred to the next tick so it is safe to call from inside a Lua callback.
function battlefield.lose() end

----------------------------------------------------------------
-- Battlefield queries
----------------------------------------------------------------

--- Count alive enemy figures on the map.
--- If figure_type is given, only counts enemies of that specific type.
--- If omitted or nil, counts all enemy figures.
---
--- ```lua
--- local total = battlefield.enemy_count()
--- local swords = battlefield.enemy_count(battlefield.FIGURE_TYPE.ENEMY_FAST_SWORD)
--- if total == 0 then battlefield.win() end
--- ```
---@param figure_type? integer Optional figure type filter (see battlefield.FIGURE_TYPE.*)
---@return integer count Number of alive enemy figures
function battlefield.enemy_count(figure_type) end

--- Count alive player (legion) figures on the map.
--- If figure_type is given, only counts soldiers of that specific type.
--- If omitted or nil, counts all player soldiers.
---
--- ```lua
--- local total = battlefield.player_count()
--- local legionaries = battlefield.player_count(battlefield.FIGURE_TYPE.LEGIONARY)
--- if total == 0 then battlefield.lose() end
--- ```
---@param figure_type? integer Optional figure type filter (see battlefield.FIGURE_TYPE.*)
---@return integer count Number of alive player figures
function battlefield.player_count(figure_type) end

--- Count active enemy formations (groups with at least one alive figure).
--- If figure_type is given, only counts formations of that specific type.
---
--- ```lua
--- local formations = battlefield.enemy_formation_count()
--- ```
---@param figure_type? integer Optional figure type filter
---@return integer count Number of active enemy formations
function battlefield.enemy_formation_count(figure_type) end

--- Count active player (legion) formations (groups with at least one alive figure).
--- If figure_type is given, only counts formations of that specific type.
---
--- ```lua
--- local formations = battlefield.player_formation_count()
--- ```
---@param figure_type? integer Optional figure type filter
---@return integer count Number of active player formations
function battlefield.player_formation_count(figure_type) end

----------------------------------------------------------------
-- Battlefield army iteration
----------------------------------------------------------------

--- Formation info table returned by enemy_armies() and player_armies().
---@class formation_info
---@field id integer Formation ID (use with formation_figures)
---@field figure_type integer Figure type constant (see battlefield.FIGURE_TYPE.*)
---@field num_figures integer Current number of alive figures in the formation
---@field max_figures integer Maximum number of figures the formation can hold
---@field x integer Current formation X position on the map
---@field y integer Current formation Y position on the map
---@field morale integer Formation morale (0-100+)
---@field direction integer Formation facing direction
---@field is_halted boolean Whether the formation is currently halted
---@field layout integer Formation layout type

--- Figure info table returned by formation_figures().
---@class figure_info
---@field id integer Figure ID
---@field figure_type integer Figure type constant
---@field x integer Figure X position on the map
---@field y integer Figure Y position on the map
---@field damage integer Current damage taken
---@field action_state integer Current action state

--- Get a list of all active enemy formations on the battlefield.
--- Returns an array of formation info tables (only formations with alive figures).
---
--- ```lua
--- local enemies = battlefield.enemy_armies()
--- for _, army in ipairs(enemies) do
---     print("Formation " .. army.id .. " at (" .. army.x .. "," .. army.y .. ") with " .. army.num_figures .. " soldiers")
--- end
--- ```
---@return formation_info[] formations Array of enemy formation info tables
function battlefield.enemy_armies() end

--- Get a list of all active player (legion) formations on the battlefield.
--- Returns an array of formation info tables (only formations with alive figures).
---
--- ```lua
--- local legions = battlefield.player_armies()
--- for _, army in ipairs(legions) do
---     if army.num_figures < 5 then
---         print("Legion " .. army.id .. " is running low on soldiers!")
---     end
--- end
--- ```
---@return formation_info[] formations Array of player formation info tables
function battlefield.player_armies() end

--- Get a list of all alive figures in a specific formation.
--- Returns an array of figure info tables for each alive figure.
---
--- ```lua
--- local enemies = battlefield.enemy_armies()
--- for _, army in ipairs(enemies) do
---     local figures = battlefield.formation_figures(army.id)
---     for _, fig in ipairs(figures) do
---         print("Figure " .. fig.id .. " at (" .. fig.x .. "," .. fig.y .. ") damage=" .. fig.damage)
---     end
--- end
--- ```
---@param formation_id integer The formation ID (from formation_info.id)
---@return figure_info[] figures Array of figure info tables
function battlefield.formation_figures(formation_id) end

----------------------------------------------------------------
-- Battlefield death counts
----------------------------------------------------------------

--- Get the number of enemy figures killed since the battlefield started.
--- If figure_type is given, only counts kills of that specific type.
---
--- ```lua
--- local total_kills = battlefield.enemy_death_count()
--- local sword_kills = battlefield.enemy_death_count(battlefield.FIGURE_TYPE.ENEMY_FAST_SWORD)
--- ```
---@param figure_type? integer Optional figure type filter (see battlefield.FIGURE_TYPE.*)
---@return integer count Number of enemy figures killed
function battlefield.enemy_death_count(figure_type) end

--- Get the number of player (legion) figures killed since the battlefield started.
--- If figure_type is given, only counts deaths of that specific type.
---
--- ```lua
--- local total_losses = battlefield.player_death_count()
--- local legionary_losses = battlefield.player_death_count(battlefield.FIGURE_TYPE.LEGIONARY)
--- ```
---@param figure_type? integer Optional figure type filter (see battlefield.FIGURE_TYPE.*)
---@return integer count Number of player figures killed
function battlefield.player_death_count(figure_type) end

--- Get the number of enemy formations destroyed since the battlefield started.
--- If figure_type is given, only counts formations of that specific type.
---@param figure_type? integer Optional figure type filter
---@return integer count Number of enemy formations destroyed
function battlefield.enemy_formation_death_count(figure_type) end

--- Get the number of player formations destroyed since the battlefield started.
--- If figure_type is given, only counts formations of that specific type.
---@param figure_type? integer Optional figure type filter
---@return integer count Number of player formations destroyed
function battlefield.player_formation_death_count(figure_type) end

----------------------------------------------------------------
-- Battlefield hooks (callbacks)
----------------------------------------------------------------

--- Called when an enemy figure is killed in combat.
--- Only fires during an active battlefield.
---@param figure_type integer The type of enemy figure that was killed
function on_enemy_killed(figure_type) end

--- Called when a player (legion) figure is killed in combat.
--- Only fires during an active battlefield.
---@param figure_type integer The type of player figure that was killed
function on_player_killed(figure_type) end

--- Called when an enemy formation is destroyed (all its figures are dead).
--- Only fires during an active battlefield.
---@param figure_type integer The figure type of the destroyed formation
function on_enemy_formation_destroyed(figure_type) end

--- Called when a player formation is destroyed (all its figures are dead).
--- Only fires during an active battlefield.
---@param figure_type integer The figure type of the destroyed formation
function on_player_formation_destroyed(figure_type) end

----------------------------------------------------------------
-- Battlefield constants
----------------------------------------------------------------

--- Player figure type constants for battlefield armies
---@class battlefield_FIGURE_TYPE
battlefield.FIGURE_TYPE = {
    LEGIONARY            = 13, -- FIGURE_FORT_LEGIONARY
    JAVELIN              = 11, -- FIGURE_FORT_JAVELIN
    MOUNTED              = 12, -- FIGURE_FORT_MOUNTED
    INFANTRY             = 92, -- FIGURE_FORT_INFANTRY
    ARCHER               = 94, -- FIGURE_FORT_ARCHER
    -- Enemy figure types
    ENEMY_SPEAR          = 43, -- FIGURE_ENEMY43_SPEAR
    ENEMY_SWORD          = 44, -- FIGURE_ENEMY44_SWORD
    ENEMY_SWORD_2        = 45, -- FIGURE_ENEMY45_SWORD
    ENEMY_CAMEL          = 46, -- FIGURE_ENEMY46_CAMEL
    ENEMY_ELEPHANT       = 47, -- FIGURE_ENEMY47_ELEPHANT
    ENEMY_CHARIOT        = 48, -- FIGURE_ENEMY48_CHARIOT
    ENEMY_FAST_SWORD     = 49, -- FIGURE_ENEMY49_FAST_SWORD
    ENEMY_SWORD_3        = 50, -- FIGURE_ENEMY50_SWORD
    ENEMY_SPEAR_2        = 51, -- FIGURE_ENEMY51_SPEAR
    ENEMY_MOUNTED_ARCHER = 52, -- FIGURE_ENEMY52_MOUNTED_ARCHER
    ENEMY_AXE            = 53, -- FIGURE_ENEMY53_AXE
    ENEMY_GLADIATOR      = 54, -- FIGURE_ENEMY54_GLADIATOR
}

--- Enemy graphics set constants (controls which enemy sprite sheet is loaded)
---@class battlefield_ENEMY_TYPE
battlefield.ENEMY_TYPE = {
    BARBARIAN    = 0,
    NUMIDIAN     = 1,
    GAUL         = 2,
    CELT         = 3,
    GOTH         = 4,
    PERGAMUM     = 5,
    SELEUCID     = 6,
    ETRUSCAN     = 7,
    GREEK        = 8,
    EGYPTIAN     = 9,
    CARTHAGINIAN = 10,
    CAESAR       = 11,
}
