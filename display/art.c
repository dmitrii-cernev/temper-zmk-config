/* SPDX-License-Identifier: MIT
 * Original vector cat and split-keyboard artwork; no external image assets.
 */
#include "art.h"

static void stroke(lv_obj_t *canvas, const lv_point_t *points, uint32_t count) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = lv_color_white();
    dsc.width = 2;
    lv_canvas_draw_line(canvas, points, count, &dsc);
}

static void box(lv_obj_t *canvas, int x, int y, int w, int h, int radius) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_black();
    dsc.border_color = lv_color_white();
    dsc.border_width = 1;
    dsc.radius = radius;
    lv_canvas_draw_rect(canvas, x, y, w, h, &dsc);
}

void temper_draw_cat(lv_obj_t *canvas, uint8_t pose) {
    const lv_point_t outline[] = {{9, 88}, {10, 66}, {14, 48}, {25, 57}, {43, 57},
                                  {54, 48}, {58, 66}, {59, 88}};
    const lv_point_t eyes_left[] = {{23, 70}, {23, 73}};
    const lv_point_t eyes_right[] = {{45, 70}, {45, 73}};
    const lv_point_t mouth[] = {{29, 77}, {32, 80}, {34, 77}, {37, 80}, {40, 77}};
    const lv_point_t whisker_left[] = {{8, 76}, {17, 78}};
    const lv_point_t whisker_right[] = {{51, 78}, {60, 76}};
    stroke(canvas, outline, sizeof(outline) / sizeof(outline[0]));
    stroke(canvas, eyes_left, 2);
    stroke(canvas, eyes_right, 2);
    stroke(canvas, mouth, 5);
    stroke(canvas, whisker_left, 2);
    stroke(canvas, whisker_right, 2);

    /* Bongo bodies and rims. Draw paws last, over the rims. */
    box(canvas, 6, 100, 26, 23, 4);
    box(canvas, 36, 100, 26, 23, 4);
    box(canvas, 4, 96, 30, 8, 4);
    box(canvas, 34, 96, 30, 8, 4);
    box(canvas, 13, pose == 1 ? 97 : 86, 13, 7, 3);
    box(canvas, 42, pose == 2 ? 97 : 86, 13, 7, 3);

    if (pose != 0) {
        int x = pose == 1 ? 12 : 43;
        const lv_point_t tap[] = {{x, 91}, {x - 3, 87}};
        stroke(canvas, tap, 2);
    }
}

void temper_draw_logo(lv_obj_t *canvas) {
    /* Resting cat above two three-by-three keyboard halves. */
    const lv_point_t cat[] = {{14, 73}, {15, 51}, {25, 60}, {43, 60}, {53, 51},
                              {54, 73}, {49, 81}, {19, 81}, {14, 73}};
    const lv_point_t eyes[] = {{24, 70}, {29, 70}};
    const lv_point_t eyes_right[] = {{39, 70}, {44, 70}};
    stroke(canvas, cat, 9);
    stroke(canvas, eyes, 2);
    stroke(canvas, eyes_right, 2);
    for (int side = 0; side < 2; side++) {
        int x = side == 0 ? 4 : 38;
        box(canvas, x, 89, 26, 32, 3);
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                box(canvas, x + 4 + col * 6, 94 + row * 7, 4, 5, 0);
            }
        }
    }
    lv_draw_label_dsc_t text;
    lv_draw_label_dsc_init(&text);
    text.color = lv_color_white();
    text.font = &lv_font_unscii_8;
    text.align = LV_TEXT_ALIGN_CENTER;
    lv_canvas_draw_text(canvas, 0, 133, 68, &text, "TEMPER");
}
