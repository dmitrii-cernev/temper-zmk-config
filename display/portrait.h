/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PORTRAIT_WIDTH 68
#define PORTRAIT_HEIGHT 160
#define CAT_FRAME_MS 100
#define CAT_REST_MS 220

/* Rotate portrait content clockwise onto the native 160 x 68 panel. */
static inline size_t portrait_native_index(unsigned int x, unsigned int y) {
    return x * PORTRAIT_HEIGHT + (PORTRAIT_HEIGHT - 1 - y);
}

/* Keep ZMK's zero-based profile indices internal; show profiles 1-5 to the user. */
static inline unsigned int profile_display_number(uint8_t index) { return index + 1; }

/* 0 = resting, 1 = left paw, 2 = right paw. */
static inline uint8_t cat_pose(uint8_t current, bool new_press, int64_t elapsed_ms) {
    if (elapsed_ms >= CAT_REST_MS) {
        return 0;
    }
    /* Alternate displayed paws even if several keypresses coalesce into one frame. */
    return new_press ? (current == 1 ? 2 : 1) : current;
}
