/* SPDX-License-Identifier: MIT
 * Run: cc -std=c99 -Wall -Wextra -Werror tests/display_test.c -o /tmp/temper-display-test
 *      /tmp/temper-display-test
 */
#include <assert.h>
#include <stdio.h>
#include "../display/portrait.h"

int main(void) {
    bool seen[PORTRAIT_WIDTH * PORTRAIT_HEIGHT] = {false};
    for (unsigned int y = 0; y < PORTRAIT_HEIGHT; y++) {
        for (unsigned int x = 0; x < PORTRAIT_WIDTH; x++) {
            size_t index = portrait_native_index(x, y);
            assert(index < sizeof(seen) / sizeof(seen[0]));
            assert(!seen[index]);
            seen[index] = true;
        }
    }
    assert(portrait_native_index(0, 0) == 159);
    assert(portrait_native_index(67, 0) == 10879);
    assert(portrait_native_index(0, 159) == 0);
    assert(portrait_native_index(67, 159) == 10720);

    assert(cat_pose(0, false, 0) == 0); /* Idle does not animate. */
    assert(cat_pose(0, true, 0) == 1);
    assert(cat_pose(1, false, 100) == 1); /* No new presses: hold pose. */
    assert(cat_pose(1, true, 0) == 2);
    assert(cat_pose(2, true, 0) == 1); /* Includes coalesced fast typing. */
    assert(cat_pose(2, false, CAT_REST_MS - 1) == 2);
    assert(cat_pose(2, false, CAT_REST_MS) == 0);
    assert(cat_pose(1, true, CAT_REST_MS + 1) == 0); /* Stale event. */
    assert(cat_pose(0, true, 0) == 1); /* Wake after resting. */
    for (uint8_t index = 0; index < 5; index++) {
        assert(profile_display_number(index) == (unsigned int)index + 1);
    }
    puts("PASS: portrait rotation, typing animation, and one-based profile numbers");
    return 0;
}
