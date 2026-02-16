#include "battle_scenarios.h"

#include "core/image_group.h"
#include "core/string.h"
#include "game/battlefield.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/list_box.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "translation/translation.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define BACKGROUND_WIDTH 1024
#define BACKGROUND_HEIGHT 768

typedef struct {
    const char *name;
    const char *description;
    const char *map_size;
    const char *forces_player;
    const char *forces_enemy;
} battle_scenario_entry;

static const battle_scenario_entry scenarios[] = {
    {
        "Test Battlefield",
        "A test battle scenario on a flat plain. Command your legions against barbarian forces!",
        "80 x 80",
        "2 Legions (16 soldiers each)",
        "2 Formations (16 barbarians each)"
    },
};

#define NUM_SCENARIOS (sizeof(scenarios) / sizeof(scenarios[0]))

static void select_scenario(unsigned int index, int is_double_click);
static void button_start_scenario(int param1, int param2);
static void button_back(int param1, int param2);
static void draw_scenario_item(const list_box_item *item);
static void handle_tooltip(tooltip_context *c);
static void scenario_tooltip(const list_box_item *item, tooltip_context *c);

static image_button start_button =
{ 600, 440, 27, 27, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 56, button_start_scenario, button_none, 1, 0, 1 };

static image_button back_button =
{ 330, 440, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 4, button_back, button_none, 1, 0, 1 };

static list_box_type list_box = {
    .x = 16,
    .y = 210,
    .width_blocks = 18,
    .height_blocks = 16,
    .item_height = 16,
    .draw_inner_panel = 1,
    .extend_to_hidden_scrollbar = 1,
    .decorate_scrollbar = 1,
    .draw_item = draw_scenario_item,
    .on_select = select_scenario,
    .handle_tooltip = scenario_tooltip
};

static struct {
    int selected_index;
} data;

static void init(void)
{
    data.selected_index = 0;
    list_box_init(&list_box, NUM_SCENARIOS);
    list_box_select_index(&list_box, 0);
}

static void draw_scenario_item(const list_box_item *item)
{
    font_t font = item->is_selected ? FONT_NORMAL_WHITE : FONT_NORMAL_GREEN;
    const uint8_t *name = string_from_ascii(scenarios[item->index].name);
    text_draw(name, item->x, item->y, font, 0);
    if (item->is_focused) {
        button_border_draw(item->x - 4, item->y - 4, item->width + 6, item->height + 4, 1);
    }
}

static void draw_scenario_info(void)
{
    if (data.selected_index < 0 || data.selected_index >= (int) NUM_SCENARIOS) {
        return;
    }

    const battle_scenario_entry *s = &scenarios[data.selected_index];
    const int info_x = 335;
    const int info_width = 280;

    // Scenario name
    const uint8_t *name = string_from_ascii(s->name);
    text_draw_centered(name, info_x, 25, info_width + 10, FONT_LARGE_BLACK, 0);

    // Description
    const uint8_t *desc = string_from_ascii(s->description);
    text_draw_centered(desc, info_x, 60, info_width, FONT_NORMAL_WHITE, 0);

    // Scenario details section
    const uint8_t *label;
    const uint8_t *value;
    int y = 150;

    // Map size
    label = string_from_ascii("Map size:");
    value = string_from_ascii(s->map_size);
    text_draw(label, info_x + 10, y, FONT_NORMAL_BLACK, 0);
    text_draw(value, info_x + 120, y, FONT_NORMAL_BLACK, 0);
    y += 20;

    // Player forces
    label = string_from_ascii("Your forces:");
    value = string_from_ascii(s->forces_player);
    text_draw(label, info_x + 10, y, FONT_NORMAL_BLACK, 0);
    text_draw(value, info_x + 120, y, FONT_NORMAL_BLACK, 0);
    y += 20;

    // Enemy forces
    label = string_from_ascii("Enemy forces:");
    value = string_from_ascii(s->forces_enemy);
    text_draw(label, info_x + 10, y, FONT_NORMAL_BLACK, 0);
    text_draw(value, info_x + 120, y, FONT_NORMAL_BLACK, 0);
    y += 20;

    // Objective
    const uint8_t *objective = string_from_ascii("Defeat all enemy formations to win.");
    text_draw_multiline(objective, info_x + 10, y + 20, info_width - 20, 0, FONT_NORMAL_BLACK, 0);

    // Bottom info line (like CCK's "click the start button" text)
    const uint8_t *start_info = string_from_ascii("Select a scenario and press the start button");
    text_draw_centered(start_info, info_x, 446, info_width, FONT_NORMAL_BLACK, 0);
}

static void draw_background(void)
{
    image_draw_fullscreen_background(image_group(GROUP_CCK_BACKGROUND) + 25);

    graphics_set_clip_rectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    graphics_in_dialog();
    image_draw(image_group(GROUP_CCK_BACKGROUND), (WINDOW_WIDTH - BACKGROUND_WIDTH) / 2,
        (WINDOW_HEIGHT - BACKGROUND_HEIGHT) / 2, COLOR_MASK_NONE, SCALE_NONE);
    graphics_reset_clip_rectangle();
    list_box_request_refresh(&list_box);
    draw_scenario_info();
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, 0, &start_button, 1);
    image_buttons_draw(0, 0, &back_button, 1);
    list_box_draw(&list_box);
    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);
    if (image_buttons_handle_mouse(m_dialog, 0, 0, &start_button, 1, 0) ||
        image_buttons_handle_mouse(m_dialog, 0, 0, &back_button, 1, 0) ||
        list_box_handle_input(&list_box, m_dialog, 1)) {
        return;
    }
    if (h->enter_pressed) {
        button_start_scenario(0, 0);
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_go_back();
    }
}

static void scenario_tooltip(const list_box_item *item, tooltip_context *c)
{
    font_t font = item->is_selected ? FONT_NORMAL_WHITE : FONT_NORMAL_GREEN;
    const uint8_t *name = string_from_ascii(scenarios[item->index].name);
    if (text_get_width(name, font) > item->width) {
        c->precomposed_text = name;
        c->type = TOOLTIP_BUTTON;
    }
}

static void handle_tooltip(tooltip_context *c)
{
    list_box_handle_tooltip(&list_box, c);
}

static void select_scenario(unsigned int index, int is_double_click)
{
    if (data.selected_index != (int) index) {
        data.selected_index = index;
        window_invalidate();
    } else if (is_double_click) {
        button_start_scenario(0, 0);
    }
}

static void button_start_scenario(int param1, int param2)
{
    if (data.selected_index < 0 || data.selected_index >= (int) NUM_SCENARIOS) {
        return;
    }
    // For now all scenarios launch the battlefield
    battlefield_start();
}

static void button_back(int param1, int param2)
{
    window_go_back();
}

void window_battle_scenarios_show(void)
{
    window_type window = {
        WINDOW_BATTLE_SCENARIOS,
        draw_background,
        draw_foreground,
        handle_input,
        handle_tooltip
    };
    init();
    window_show(&window);
}
