// SPDX-License-Identifier: MIT

#include "wm.h"

// Minimal stub implementations for window animation hooks.
// These functions are intentionally left empty; they simply
// suppress unused-parameter warnings and allow the kernel to
// compile without linking errors.

static void wm_animate_window_open(xdg_toplevel_t *win) {
    (void)win; // placeholder to avoid unused variable warning
}

static void wm_animate_window_close(xdg_toplevel_t *win) {
    (void)win; // placeholder to avoid unused variable warning
}
