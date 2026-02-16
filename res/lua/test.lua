-- Test scenario: Battlefield launcher via input dialog

local function start_battle()
    game.battlefield_start({
        enemy_id = game.ENEMY_TYPE.BARBARIAN,
        player_armies = {
            { figure_type = game.FIGURE_TYPE.LEGIONARY, count = 3, soldiers = 16, x = 15, y = 25, y_spacing = 10 },
            { figure_type = game.FIGURE_TYPE.MOUNTED,   count = 2, soldiers = 12, x = 10, y = 55, y_spacing = 8 },
            { figure_type = game.FIGURE_TYPE.JAVELIN,   count = 2, soldiers = 12, x = 12, y = 35, y_spacing = 10 },
        },
        enemy_armies = {
            { figure_type = game.FIGURE_TYPE.ENEMY_FAST_SWORD, count = 2, soldiers = 16, x = 65, y = 35, y_spacing = 10 },
        },
    })
end

function on_load()
    ui.input_dialog({
        title = "Battlefield",
        text = "Would you like to start a battlefield skirmish? Your forces: 3 Legions, 2 Cavalry, 2 Javelin squads vs 2 Barbarian formations.",
        buttons = {
            { label = "To battle!", on_click = start_battle },
            { label = "Not now" },
        },
    })
end
