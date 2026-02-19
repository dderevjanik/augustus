#include "military_status_bar.h"

#include "assets/assets.h"
#include "city/view.h"
#include "core/image_group.h"
#include "figure/formation.h"
#include "figure/formation_legion.h"
#include "game/state.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "core/string.h"
#include "widget/sidebar/military.h"
#include <stdio.h>
#include "window/city.h"

#define SLOT_WIDTH 60
#define SLOT_SPACING 4
#define MAX_SLOTS 20
#define BORDER_WIDTH 16
#define CONTENT_PADDING 8
#define CONTENT_TOP (BORDER_WIDTH + CONTENT_PADDING)
#define CONTENT_BOTTOM (BORDER_WIDTH + CONTENT_PADDING)
#define CONTENT_LEFT (BORDER_WIDTH + CONTENT_PADDING)
#define CONTENT_RIGHT (BORDER_WIDTH + CONTENT_PADDING)
#define SLOT_HEIGHT 125
#define BAR_HEIGHT (CONTENT_TOP + SLOT_HEIGHT + CONTENT_BOTTOM)

static void button_select_legion(const generic_button *button);

static generic_button legion_buttons[MAX_SLOTS];

static struct {
    int num_legions;
    int formation_ids[MAX_SLOTS];
    unsigned int focus_button_id;
} data;

static void collect_legions(void)
{
    data.num_legions = 0;
    for (int i = 1; i < formation_count(); i++) {
        const formation *m = formation_get(i);
        if (m->in_use && !m->is_herd && m->is_legion) {
            data.formation_ids[data.num_legions] = i;
            data.num_legions++;
            if (data.num_legions >= MAX_SLOTS) {
                break;
            }
        }
    }
}

static int get_flag_image_id(const formation *m)
{
    int flag_image_id = image_group(GROUP_FIGURE_FORT_FLAGS);
    if (m->figure_type == FIGURE_FORT_JAVELIN) {
        flag_image_id += 9;
    } else if (m->figure_type == FIGURE_FORT_MOUNTED) {
        flag_image_id += 18;
    }
    if (m->is_halted) {
        flag_image_id += 8;
    }
    if (m->figure_type == FIGURE_FORT_INFANTRY) {
        flag_image_id = assets_get_image_id("UI",
            m->is_halted ? "auxinf_banner_0" : "auxinf_banner_01");
    }
    if (m->figure_type == FIGURE_FORT_ARCHER) {
        flag_image_id = assets_get_image_id("UI",
            m->is_halted ? "auxarch_banner_0" : "auxarch_banner_01");
    }
    return flag_image_id;
}

static void draw_standard(const formation *m, int center_x, int top_y)
{
    // Icon at the top
    int icon_image_id = m->legion_flag_id;
    const image *icon_img = image_get(icon_image_id);
    int icon_x = center_x - (icon_img->width + icon_img->x_offset) / 2;
    image_draw(icon_image_id, icon_x, top_y, COLOR_MASK_NONE, SCALE_NONE);
    int y_cursor = top_y + icon_img->height;

    // Flag below icon
    int flag_image_id = get_flag_image_id(m);
    const image *flag_img = image_get(flag_image_id);
    int flag_x = center_x - (flag_img->width + flag_img->x_offset) / 2;
    image_draw(flag_image_id, flag_x, y_cursor, COLOR_MASK_NONE, SCALE_NONE);
    y_cursor += flag_img->height;

    // Pole with morale ball below flag
    // int morale_offset = m->morale / 5;
    // if (morale_offset > 20) {
    //     morale_offset = 20;
    // }
    // int pole_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_POLE) + 20 - morale_offset;
    // const image *pole_img = image_get(pole_image_id);
    // int pole_x = center_x - (pole_img->width + pole_img->x_offset) / 2;
    // image_draw(pole_image_id, pole_x, y_cursor, COLOR_MASK_NONE, SCALE_NONE);
}

static int get_soldier_image_for_type(int figure_type)
{
    switch (figure_type) {
        case FIGURE_FORT_LEGIONARY:
            return image_group(GROUP_BIG_PEOPLE) + 40;
        case FIGURE_FORT_JAVELIN:
            return image_group(GROUP_BIG_PEOPLE) + 41;
        case FIGURE_FORT_MOUNTED:
            return image_group(GROUP_BIG_PEOPLE) + 25;
        case FIGURE_FORT_INFANTRY:
            return assets_get_image_id("Warriors", "auxinf_portrait");
        case FIGURE_FORT_ARCHER:
            return assets_get_image_id("Warriors", "auxarch_portrait");
        default:
            return image_group(GROUP_BIG_PEOPLE) + 40;
    }
}

void widget_military_status_bar_draw(void)
{
    if (game_state_overlay() != OVERLAY_MILITARY) {
        return;
    }

    collect_legions();
    if (data.num_legions == 0) {
        return;
    }

    int vp_x, vp_y, vp_width, vp_height;
    city_view_get_viewport(&vp_x, &vp_y, &vp_width, &vp_height);

    int bar_x = vp_x;
    int bar_y = vp_y + vp_height - BAR_HEIGHT;
    int bar_width = CONTENT_LEFT + data.num_legions * (SLOT_WIDTH + SLOT_SPACING) - SLOT_SPACING + CONTENT_RIGHT;
    if (bar_width > vp_width) {
        bar_width = vp_width;
    }

    int content_x = bar_x + CONTENT_LEFT;
    int content_y = bar_y + CONTENT_TOP;
    int image_base = image_group(GROUP_EMPIRE_PANELS);

    // Solid background fill to cover any gaps from panel tiling
    int inner_x = bar_x + BORDER_WIDTH;
    int inner_y = bar_y + BORDER_WIDTH;
    int inner_w = bar_width - 2 * BORDER_WIDTH + 10;
    int inner_h = BAR_HEIGHT - 2 * BORDER_WIDTH + 30;
    graphics_fill_rect(inner_x, inner_y, inner_w, inner_h, COLOR_BLACK);
    inner_panel_draw(inner_x, inner_y, inner_w / BLOCK_SIZE, inner_h / BLOCK_SIZE);

    // Empire-style border frame on top
    graphics_set_clip_rectangle(bar_x, bar_y, bar_width, BAR_HEIGHT);

    // Horizontal border bars (top and bottom)
    for (int x = bar_x; x < bar_x + bar_width; x += 86) {
        image_draw(image_base + 1, x, bar_y, COLOR_MASK_NONE, SCALE_NONE);
        image_draw(image_base + 1, x, bar_y + BAR_HEIGHT - BORDER_WIDTH, COLOR_MASK_NONE, SCALE_NONE);
    }

    // Vertical border bars (left and right)
    for (int y = bar_y + BORDER_WIDTH; y < bar_y + BAR_HEIGHT - BORDER_WIDTH; y += 86) {
        image_draw(image_base, bar_x, y, COLOR_MASK_NONE, SCALE_NONE);
        image_draw(image_base, bar_x + bar_width - BORDER_WIDTH, y, COLOR_MASK_NONE, SCALE_NONE);
    }

    // Corner crossbars
    image_draw(image_base + 2, bar_x, bar_y, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, bar_x + bar_width - BORDER_WIDTH, bar_y, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, bar_x, bar_y + BAR_HEIGHT - BORDER_WIDTH, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, bar_x + bar_width - BORDER_WIDTH, bar_y + BAR_HEIGHT - BORDER_WIDTH, COLOR_MASK_NONE, SCALE_NONE);
    graphics_reset_clip_rectangle();

    for (int i = 0; i < data.num_legions; i++) {
        const formation *m = formation_get(data.formation_ids[i]);
        int slot_x = content_x + i * (SLOT_WIDTH + SLOT_SPACING);
        int slot_center_x = slot_x + SLOT_WIDTH / 2;

        legion_buttons[i].x = CONTENT_LEFT + i * (SLOT_WIDTH + SLOT_SPACING);
        legion_buttons[i].y = CONTENT_TOP;
        legion_buttons[i].width = SLOT_WIDTH;
        legion_buttons[i].height = SLOT_HEIGHT;
        legion_buttons[i].left_click_handler = button_select_legion;
        legion_buttons[i].parameter1 = i;

        // Draw full standard (icon + flag + pole) at top of slot
        draw_standard(m, slot_center_x, content_y - 45);

        // Draw soldier figure below the flag
        int soldier_image = get_soldier_image_for_type(m->figure_type);
        if (soldier_image) {
            const image *soldier_img = image_get(soldier_image);
            int fig_x = slot_center_x - soldier_img->width / 2;
            image_draw(soldier_image, fig_x - 5, content_y, COLOR_MASK_NONE, SCALE_NONE);
        }

        // Draw troop count bar below figure
        int bar_x = slot_x + 2;
        int bar_w = SLOT_WIDTH - 4;
        int bar_h = 9;
        int bar_y = content_y + 85;
        int troop_pct = m->max_figures > 0 ? m->num_figures * 100 / m->max_figures : 0;
        int troop_fill = bar_w * troop_pct / 100;
        color_t troop_bar_color = troop_pct < 30 ? COLOR_RED : COLOR_FONT_GREEN;
        graphics_fill_rect(bar_x, bar_y, bar_w, bar_h, COLOR_BLACK);
        if (troop_fill > 0) {
            graphics_fill_rect(bar_x, bar_y, troop_fill, bar_h, troop_bar_color);
        }
        char soldiers_buf[20];
        snprintf(soldiers_buf, sizeof(soldiers_buf), "%d/%d", m->num_figures, m->max_figures);
        text_draw_centered(string_from_ascii(soldiers_buf), slot_x, bar_y, SLOT_WIDTH, FONT_SMALL_PLAIN, COLOR_WHITE);

        // Draw morale bar below troop bar
        int morale_y = bar_y + bar_h + 1;
        int morale_pct = m->morale > 100 ? 100 : m->morale;
        int morale_fill = bar_w * morale_pct / 100;
        graphics_fill_rect(bar_x, morale_y, bar_w, bar_h, COLOR_BLACK);
        if (morale_fill > 0 && m->num_figures > 0) {
            graphics_fill_rect(bar_x, morale_y, morale_fill, bar_h, COLOR_FONT_BLUE);
        }

        // Draw highlight border
        int is_selected = ((int) formation_get_selected() == data.formation_ids[i]);
        int is_focused = (data.focus_button_id == (unsigned int) (i + 1));
        button_border_draw(slot_x, content_y, SLOT_WIDTH, SLOT_HEIGHT, is_focused || is_selected);

        // Draw fort/outside status indicator in top-right corner
        const uint8_t *status_text = m->is_at_fort ? string_from_ascii("F") : string_from_ascii("O");
        color_t status_color = m->is_at_fort ? COLOR_FONT_YELLOW : COLOR_FONT_RED;
        text_draw(status_text, slot_x + SLOT_WIDTH - 14, content_y + 8, FONT_SMALL_PLAIN, status_color);
    }
}

int widget_military_status_bar_handle_mouse(const mouse *m)
{
    if (game_state_overlay() != OVERLAY_MILITARY) {
        return 0;
    }
    if (data.num_legions == 0) {
        return 0;
    }

    int vp_x, vp_y, vp_width, vp_height;
    city_view_get_viewport(&vp_x, &vp_y, &vp_width, &vp_height);
    int bar_y = vp_y + vp_height - BAR_HEIGHT;

    return generic_buttons_handle_mouse(m, vp_x, bar_y,
        legion_buttons, data.num_legions, &data.focus_button_id);
}

static void button_select_legion(const generic_button *button)
{
    int index = button->parameter1;
    if (index >= 0 && index < data.num_legions) {
        int formation_id = data.formation_ids[index];
        window_city_military_show(formation_id);
    }
}
