#include "random_map.h"

#include "game/file_editor.h"
#include "graphics/arrow_button.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "mapgen/mapgen.h"
#include "mapgen/mapgen_config.h"
#include "translation/translation.h"
#include "window/editor/map.h"

#include <stdlib.h>
#include <time.h>

static void button_generate(const generic_button *button);
static void button_randomize_seed(const generic_button *button);
static void button_cancel(const generic_button *button);
static void arrow_map_size(int is_down, int param2);
static void arrow_tree_density(int is_down, int param2);
static void arrow_rock_density(int is_down, int param2);
static void arrow_shrub_density(int is_down, int param2);
static void arrow_meadow_density(int is_down, int param2);
static void arrow_seed(int is_down, int param2);
static void arrow_lake_count(int is_down, int param2);
static void arrow_lake_size(int is_down, int param2);
static void arrow_river_count(int is_down, int param2);
static void arrow_coastal(int is_down, int param2);
static void arrow_elevation_enabled(int is_down, int param2);
static void arrow_max_elevation(int is_down, int param2);
static void arrow_flatness(int is_down, int param2);

static generic_button buttons[] = {
    {130, 430, 200, 30, button_generate},
    {350, 430, 200, 30, button_cancel},
    {400, 48, 100, 24, button_randomize_seed},
};

static arrow_button arrow_buttons[] = {
    {370, 46, 17, 24, arrow_seed, 0, 0},
    {394, 46, 15, 24, arrow_seed, 1, 0},
    {370, 76, 17, 24, arrow_map_size, 0, 0},
    {394, 76, 15, 24, arrow_map_size, 1, 0},
    {370, 106, 17, 24, arrow_tree_density, 0, 0},
    {394, 106, 15, 24, arrow_tree_density, 1, 0},
    {370, 136, 17, 24, arrow_rock_density, 0, 0},
    {394, 136, 15, 24, arrow_rock_density, 1, 0},
    {370, 166, 17, 24, arrow_shrub_density, 0, 0},
    {394, 166, 15, 24, arrow_shrub_density, 1, 0},
    {370, 196, 17, 24, arrow_meadow_density, 0, 0},
    {394, 196, 15, 24, arrow_meadow_density, 1, 0},
    {370, 236, 17, 24, arrow_lake_count, 0, 0},
    {394, 236, 15, 24, arrow_lake_count, 1, 0},
    {370, 266, 17, 24, arrow_lake_size, 0, 0},
    {394, 266, 15, 24, arrow_lake_size, 1, 0},
    {370, 296, 17, 24, arrow_river_count, 0, 0},
    {394, 296, 15, 24, arrow_river_count, 1, 0},
    {370, 326, 17, 24, arrow_coastal, 0, 0},
    {394, 326, 15, 24, arrow_coastal, 1, 0},
    {370, 356, 17, 24, arrow_elevation_enabled, 0, 0},
    {394, 356, 15, 24, arrow_elevation_enabled, 1, 0},
    {370, 386, 17, 24, arrow_max_elevation, 0, 0},
    {394, 386, 15, 24, arrow_max_elevation, 1, 0},
    {370, 406, 17, 24, arrow_flatness, 0, 0},
    {394, 406, 15, 24, arrow_flatness, 1, 0},
};

static const int MAP_SIZES[] = {40, 60, 80, 100, 120, 160};
static const char *COASTAL_LABELS[] = {"None", "One side", "Corner"};

static struct {
    mapgen_config config;
    unsigned int focus_button_id;
    unsigned int focus_arrow_id;
} data;

static int initialized = 0;

static void ensure_initialized(void)
{
    if (!initialized) {
        mapgen_config_init_defaults(&data.config);
        data.config.seed = (uint32_t)time(NULL);
        initialized = 1;
    }
}

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    ensure_initialized();
    graphics_in_dialog();

    outer_panel_draw(48, 16, 28, 30);

    // Title
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_TITLE, 80, 28, FONT_LARGE_BLACK);

    int label_x = 80;
    int value_x = 280;
    int value_w = 90;
    int y = 56;
    int row_h = 30;

    // Seed
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_SEED, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.seed % 100000, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Map size
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_MAP_SIZE, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(MAP_SIZES[data.config.map_size], value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Tree density
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_TREE_DENSITY, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.tree_density, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Rock density
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_ROCK_DENSITY, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.rock_density, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Shrub density
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_SHRUB_DENSITY, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.shrub_density, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Meadow density
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_MEADOW_DENSITY, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.meadow_density, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += 10; // spacer
    y += row_h;
    // Lake count
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_LAKE_COUNT, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.lake_count, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Lake size
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_LAKE_SIZE, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.lake_size, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // River count
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_RIVER_COUNT, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.river_count, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Coastal
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_COASTAL, label_x, y, FONT_NORMAL_BLACK);
    text_draw_centered((const uint8_t *)COASTAL_LABELS[data.config.coastal],
        value_x, y, value_w, FONT_NORMAL_BLACK, 0);

    y += row_h;
    // Elevation enabled
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_ELEVATION, label_x, y, FONT_NORMAL_BLACK);
    lang_text_draw_centered(18, data.config.elevation_enabled, value_x, y, value_w, FONT_NORMAL_BLACK);

    y += row_h;
    // Max elevation
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_MAX_ELEVATION, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.max_elevation, value_x, y, value_w, FONT_NORMAL_BLACK);

    // Flatness (below max elevation, sharing arrow row)
    y += 20;
    lang_text_draw(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_FLATNESS, label_x, y, FONT_NORMAL_BLACK);
    text_draw_number_centered(data.config.flatness, value_x, y, value_w, FONT_NORMAL_BLACK);

    // Buttons
    button_border_draw(130, 430, 200, 30, data.focus_button_id == 1);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_GENERATE, 130, 438, 200, FONT_NORMAL_BLACK);

    button_border_draw(350, 430, 200, 30, data.focus_button_id == 2);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_EDITOR_RANDOM_MAP_CANCEL, 350, 438, 200, FONT_NORMAL_BLACK);

    arrow_buttons_draw(0, 0, arrow_buttons, 26);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons, 3, &data.focus_button_id)) {
        return;
    }
    if (arrow_buttons_handle_mouse(m_dialog, 0, 0, arrow_buttons, 26, &data.focus_arrow_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_editor_map_show();
    }
}

static void clamp_int(int *value, int min, int max)
{
    if (*value < min) *value = min;
    if (*value > max) *value = max;
}

static void arrow_seed(int is_down, int param2)
{
    if (is_down) {
        data.config.seed -= 1;
    } else {
        data.config.seed += 1;
    }
}

static void arrow_map_size(int is_down, int param2)
{
    if (is_down) {
        data.config.map_size--;
    } else {
        data.config.map_size++;
    }
    clamp_int(&data.config.map_size, 0, 5);
}

static void arrow_tree_density(int is_down, int param2)
{
    data.config.tree_density += is_down ? -5 : 5;
    clamp_int(&data.config.tree_density, 0, 100);
}

static void arrow_rock_density(int is_down, int param2)
{
    data.config.rock_density += is_down ? -5 : 5;
    clamp_int(&data.config.rock_density, 0, 100);
}

static void arrow_shrub_density(int is_down, int param2)
{
    data.config.shrub_density += is_down ? -5 : 5;
    clamp_int(&data.config.shrub_density, 0, 100);
}

static void arrow_meadow_density(int is_down, int param2)
{
    data.config.meadow_density += is_down ? -5 : 5;
    clamp_int(&data.config.meadow_density, 0, 100);
}

static void arrow_lake_count(int is_down, int param2)
{
    data.config.lake_count += is_down ? -1 : 1;
    clamp_int(&data.config.lake_count, 0, 4);
}

static void arrow_lake_size(int is_down, int param2)
{
    data.config.lake_size += is_down ? -1 : 1;
    clamp_int(&data.config.lake_size, 1, 5);
}

static void arrow_river_count(int is_down, int param2)
{
    data.config.river_count += is_down ? -1 : 1;
    clamp_int(&data.config.river_count, 0, 2);
}

static void arrow_coastal(int is_down, int param2)
{
    data.config.coastal += is_down ? -1 : 1;
    clamp_int(&data.config.coastal, 0, 2);
}

static void arrow_elevation_enabled(int is_down, int param2)
{
    data.config.elevation_enabled = !data.config.elevation_enabled;
}

static void arrow_max_elevation(int is_down, int param2)
{
    data.config.max_elevation += is_down ? -1 : 1;
    clamp_int(&data.config.max_elevation, 1, 5);
}

static void arrow_flatness(int is_down, int param2)
{
    data.config.flatness += is_down ? -5 : 5;
    clamp_int(&data.config.flatness, 0, 100);
}

static void button_generate(const generic_button *button)
{
    game_file_editor_create_scenario(data.config.map_size);
    mapgen_generate(&data.config);
    window_editor_map_show();
}

static void button_randomize_seed(const generic_button *button)
{
    data.config.seed = (uint32_t)time(NULL) ^ (data.config.seed * 2654435761u);
}

static void button_cancel(const generic_button *button)
{
    window_editor_map_show();
}

void window_editor_random_map_show(void)
{
    window_type window = {
        WINDOW_EDITOR_RANDOM_MAP,
        draw_background,
        draw_foreground,
        handle_input
    };
    window_show(&window);
}
