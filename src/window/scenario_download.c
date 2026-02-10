#include "scenario_download.h"

#include "core/encoding.h"
#include "core/image_group.h"
#include "core/string.h"
#include "game/scenario_download.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/list_box.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "translation/translation.h"

#include <stdio.h>
#include <string.h>

#define COL_NAME_X 0
#define COL_NAME_W 200
#define COL_RATING_X 200
#define COL_RATING_W 50
#define COL_AUTHOR_X 250
#define COL_AUTHOR_W 130
#define COL_UPDATED_X 380
#define COL_UPDATED_W 100
#define COL_RELEASED_X 480
#define COL_RELEASED_W 100

static void draw_scenario_item(const list_box_item *item);
static void select_scenario(unsigned int index, int is_double_click);
static void button_back(int param1, int param2);

static image_button back_button =
    { 16, 440, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 4, button_back, button_none, 1, 0, 1 };

static list_box_type list_box = {
    .x = 16,
    .y = 75,
    .width_blocks = 37,
    .height_blocks = 22,
    .item_height = 16,
    .draw_inner_panel = 1,
    .extend_to_hidden_scrollbar = 1,
    .decorate_scrollbar = 1,
    .draw_item = draw_scenario_item,
    .on_select = select_scenario,
};

static struct {
    int initialized;
} data;

static void init(void)
{
    data.initialized = 0;

    if (scenario_download_fetch()) {
        const scenario_download_list *list = scenario_download_get_list();
        list_box_init(&list_box, list->count);
        if (list->count > 0) {
            list_box_select_index(&list_box, 0);
        }
        data.initialized = 1;
    }
}

static void draw_scenario_item(const list_box_item *item)
{
    const scenario_download_list *list = scenario_download_get_list();
    if ((int) item->index >= list->count) {
        return;
    }

    const scenario_download_entry *entry = &list->entries[item->index];
    font_t font = item->is_selected ? FONT_NORMAL_WHITE : FONT_NORMAL_GREEN;
    int base_x = item->x;
    int y = item->y;

    // Name column - show "*" prefix if already downloaded
    uint8_t name_buf[SCENARIO_DL_FIELD_MAX];
    if (entry->is_downloaded) {
        char tmp[SCENARIO_DL_FIELD_MAX];
        snprintf(tmp, sizeof(tmp), "* %s", entry->name);
        encoding_from_utf8(tmp, name_buf, SCENARIO_DL_FIELD_MAX);
    } else {
        encoding_from_utf8(entry->name, name_buf, SCENARIO_DL_FIELD_MAX);
    }
    text_ellipsize(name_buf, font, COL_NAME_W - 5);
    text_draw(name_buf, base_x + COL_NAME_X, y, font, 0);

    // Rating column
    uint8_t rating_buf[32];
    encoding_from_utf8(entry->rating, rating_buf, 32);
    text_draw_centered(rating_buf, base_x + COL_RATING_X, y, COL_RATING_W, font, 0);

    // Author column
    uint8_t author_buf[SCENARIO_DL_FIELD_MAX];
    encoding_from_utf8(entry->author, author_buf, SCENARIO_DL_FIELD_MAX);
    text_ellipsize(author_buf, font, COL_AUTHOR_W - 5);
    text_draw(author_buf, base_x + COL_AUTHOR_X, y, font, 0);

    // Updated At column
    uint8_t updated_buf[32];
    encoding_from_utf8(entry->updated_at, updated_buf, 32);
    text_draw_centered(updated_buf, base_x + COL_UPDATED_X, y, COL_UPDATED_W, font, 0);

    // Released At column
    uint8_t released_buf[32];
    encoding_from_utf8(entry->released_at, released_buf, 32);
    text_draw_centered(released_buf, base_x + COL_RELEASED_X, y, COL_RELEASED_W, font, 0);

    if (item->is_focused) {
        button_border_draw(item->x - 4, item->y - 4, item->width + 6, item->height + 4, 1);
    }
}

static void draw_column_headers(int base_x, int y)
{
    font_t font = FONT_NORMAL_WHITE;

    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_COL_NAME,
        base_x + COL_NAME_X, y, COL_NAME_W, font);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_COL_RATING,
        base_x + COL_RATING_X, y, COL_RATING_W, font);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_COL_AUTHOR,
        base_x + COL_AUTHOR_X, y, COL_AUTHOR_W, font);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_COL_UPDATED,
        base_x + COL_UPDATED_X, y, COL_UPDATED_W, font);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_COL_RELEASED,
        base_x + COL_RELEASED_X, y, COL_RELEASED_W, font);
}

static void draw_background(void)
{
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND));
    graphics_in_dialog();

    outer_panel_draw(0, 0, 40, 30);

    // Title
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_TITLE,
        0, 16, 640, FONT_LARGE_BLACK);

    // Column headers
    draw_column_headers(24, 58);

    // Status messages
    const scenario_download_list *list = scenario_download_get_list();
    if (list->fetched == -1) {
        lang_text_draw_centered_colored(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_FETCH_ERROR,
            0, 200, 640, FONT_NORMAL_RED, 0);
    } else if (list->fetched == 0) {
        lang_text_draw_centered(CUSTOM_TRANSLATION, TR_WINDOW_SCENARIO_DOWNLOAD_FETCHING,
            0, 200, 640, FONT_NORMAL_WHITE);
    }

    list_box_request_refresh(&list_box);
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    if (data.initialized) {
        list_box_draw(&list_box);
    }

    image_buttons_draw(0, 0, &back_button, 1);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);

    if (image_buttons_handle_mouse(m_dialog, 0, 0, &back_button, 1, 0)) {
        return;
    }
    if (data.initialized && list_box_handle_input(&list_box, m_dialog, 1)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_go_back();
    }
}

static void select_scenario(unsigned int index, int is_double_click)
{
    if (!is_double_click) {
        return;
    }

    const scenario_download_list *list = scenario_download_get_list();
    if ((int) index >= list->count) {
        return;
    }

    if (list->entries[index].is_downloaded) {
        return;
    }

    if (scenario_download_file(index)) {
        window_invalidate();
    }
}

static void button_back(int param1, int param2)
{
    window_go_back();
}

void window_scenario_download_show(void)
{
    window_type window = {
        WINDOW_SCENARIO_DOWNLOAD,
        draw_background,
        draw_foreground,
        handle_input
    };
    init();
    window_show(&window);
}
