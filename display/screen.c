/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#include "art.h"
#include "portrait.h"

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#define TEMPER_CENTRAL 1
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#else
#define TEMPER_CENTRAL 0
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#endif

static lv_obj_t *portrait_canvas;
static lv_obj_t *panel_canvas;
static lv_color_t portrait_pixels[PORTRAIT_WIDTH * PORTRAIT_HEIGHT];
static lv_color_t panel_pixels[PORTRAIT_WIDTH * PORTRAIT_HEIGHT];
static uint8_t battery;

#if TEMPER_CENTRAL
static uint8_t pose;
static struct {
    bool usb;
    uint8_t profile;
    const char *layer;
} status;
#else
static bool connected;
#endif

static void text(int x, int y, int width, const char *value, lv_text_align_t align) {
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.font = &lv_font_unscii_8;
    dsc.color = lv_color_white();
    dsc.align = align;
    lv_canvas_draw_text(portrait_canvas, x, y, width, &dsc, value);
}

static void separator(int y) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = lv_color_white();
    const lv_point_t points[] = {{2, y}, {65, y}};
    lv_canvas_draw_line(portrait_canvas, points, 2, &dsc);
}

/* All LVGL access stays on ZMK's display work queue, including the animation timer. */
static void render(void) {
    if (!panel_canvas) {
        return;
    }
    lv_canvas_fill_bg(portrait_canvas, lv_color_black(), LV_OPA_COVER);
    char level[6];
    snprintf(level, sizeof(level), "%u%%", battery);
    text(2, 4, 33, level, LV_TEXT_ALIGN_LEFT);
#if TEMPER_CENTRAL
    text(37, 4, 29, status.usb ? "USB" : "BT", LV_TEXT_ALIGN_RIGHT);
    separator(18);
    temper_draw_cat(portrait_canvas, pose);
    separator(140);
    char profile[4];
    snprintf(profile, sizeof(profile), "%u", profile_display_number(status.profile));
    text(2, 148, 10, profile, LV_TEXT_ALIGN_LEFT);
    /* Montserrat 12 leaves room for all current layer names, including System. */
    lv_draw_label_dsc_t layer;
    lv_draw_label_dsc_init(&layer);
    layer.font = &lv_font_montserrat_12;
    layer.color = lv_color_white();
    layer.align = LV_TEXT_ALIGN_RIGHT;
    lv_canvas_draw_text(portrait_canvas, 15, 146, 51, &layer,
                        status.layer && status.layer[0] ? status.layer : "Base");
#else
    text(34, 4, 32, connected ? "LINK" : "----", LV_TEXT_ALIGN_RIGHT);
    separator(18);
    temper_draw_logo(portrait_canvas);
#endif
    for (unsigned int y = 0; y < PORTRAIT_HEIGHT; y++) {
        for (unsigned int x = 0; x < PORTRAIT_WIDTH; x++) {
            panel_pixels[portrait_native_index(x, y)] =
                portrait_pixels[y * PORTRAIT_WIDTH + x];
        }
    }
    lv_obj_invalidate(panel_canvas);
}

static uint8_t battery_get(const zmk_event_t *eh) {
    return zmk_battery_state_of_charge();
}

static void battery_update(uint8_t level) {
    battery = level;
    render();
}

ZMK_DISPLAY_WIDGET_LISTENER(temper_battery, uint8_t, battery_update, battery_get)
ZMK_SUBSCRIPTION(temper_battery, zmk_battery_state_changed);

#if TEMPER_CENTRAL
struct output_state {
    bool usb;
    uint8_t profile;
};

static struct output_state output_get(const zmk_event_t *eh) {
    return (struct output_state){
        .usb = zmk_endpoints_selected().transport == ZMK_TRANSPORT_USB,
        .profile = zmk_ble_active_profile_index(),
    };
}

static void output_update(struct output_state state) {
    status.usb = state.usb;
    status.profile = state.profile;
    render();
}

ZMK_DISPLAY_WIDGET_LISTENER(temper_output, struct output_state, output_update, output_get)
ZMK_SUBSCRIPTION(temper_output, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(temper_output, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(temper_output, zmk_usb_conn_state_changed);

static const char *layer_get(const zmk_event_t *eh) {
    return zmk_keymap_layer_name(
        zmk_keymap_layer_index_to_id(zmk_keymap_highest_layer_active()));
}

static void layer_update(const char *name) {
    status.layer = name;
    render();
}

ZMK_DISPLAY_WIDGET_LISTENER(temper_layer, const char *, layer_update, layer_get)
ZMK_SUBSCRIPTION(temper_layer, zmk_layer_state_changed);

struct typing_state {
    uint32_t presses;
    int64_t last_press;
};
static struct typing_state typing;
static lv_timer_t *cat_timer;

static struct typing_state typing_get(const zmk_event_t *eh) {
    /* Called under the listener mutex, not from the LVGL thread. */
    static struct typing_state state;
    const struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);
    if (event && event->state) {
        state.presses++;
        state.last_press = k_uptime_get();
    }
    return state;
}

static void animation_tick(lv_timer_t *timer) {
    static uint32_t rendered_presses;
    uint8_t next = cat_pose(pose, typing.presses != rendered_presses,
                            k_uptime_get() - typing.last_press);
    rendered_presses = typing.presses;
    if (next != pose) {
        pose = next;
        render();
    }
    if (next == 0) {
        lv_timer_pause(timer);
    }
}

static void typing_update(struct typing_state state) {
    if (state.presses == typing.presses) {
        return; /* Releases neither animate nor extend the animation. */
    }
    typing = state;
    lv_timer_resume(cat_timer);
}

ZMK_DISPLAY_WIDGET_LISTENER(temper_typing, struct typing_state, typing_update, typing_get)
ZMK_SUBSCRIPTION(temper_typing, zmk_position_state_changed);
#else
static bool connection_get(const zmk_event_t *eh) {
    return zmk_split_bt_peripheral_is_connected();
}

static void connection_update(bool state) {
    connected = state;
    render();
}

ZMK_DISPLAY_WIDGET_LISTENER(temper_connection, bool, connection_update, connection_get)
ZMK_SUBSCRIPTION(temper_connection, zmk_split_peripheral_status_changed);
#endif

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Hidden logical canvas; visible canvas uses the panel's native dimensions. */
    portrait_canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(portrait_canvas, portrait_pixels, PORTRAIT_WIDTH, PORTRAIT_HEIGHT,
                         LV_IMG_CF_TRUE_COLOR);
    lv_obj_add_flag(portrait_canvas, LV_OBJ_FLAG_HIDDEN);
    panel_canvas = lv_canvas_create(screen);
    lv_obj_set_pos(panel_canvas, 0, 0);
    lv_canvas_set_buffer(panel_canvas, panel_pixels, PORTRAIT_HEIGHT, PORTRAIT_WIDTH,
                         LV_IMG_CF_TRUE_COLOR);

#if TEMPER_CENTRAL
    cat_timer = lv_timer_create(animation_tick, CAT_FRAME_MS, NULL);
    lv_timer_pause(cat_timer);
    temper_output_init();
    temper_layer_init();
    temper_typing_init();
#else
    temper_connection_init();
#endif
    temper_battery_init();
    return screen;
}
