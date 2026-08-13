// Window Decorations component – provides simple drawing of window frames.
// This header declares the API for drawing window decorations using the
// low‑level framebuffer primitives defined in gui/c/fb_draw.c.
//
// The API operates on the compositor's xdg_toplevel_t objects, which contain
// the surface geometry and window title.

#ifndef SHADOWBOX_WM_DECORATIONS_H
#define SHADOWBOX_WM_DECORATIONS_H

#include <stdint.h>
#include "wm.h"

// Draw window decorations (title bar, border, background) for the given toplevel.
// fb   – framebuffer base address.
// stride – number of bytes per row (pixel stride).
void wm_draw_window_decorations(const xdg_toplevel_t *win, void *fb, uint32_t stride);

#endif // SHADOWBOX_WM_DECORATIONS_H
