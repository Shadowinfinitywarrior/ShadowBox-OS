#include "wm.h"
#include "compositor.h" // provides wl_surface_t and xdg_toplevel_t definitions

// Forward declarations of low‑level drawing primitives from gui/c/fb_draw.c
extern void fb_fill_rect(void *fb, uint32_t stride,
                         int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t color);
extern void fb_draw_rect(void *fb, uint32_t stride,
                         int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t color);
extern void fb_draw_text(void *fb, uint32_t stride,
                         int32_t x, int32_t y,
                         const char *s, uint32_t fg, uint32_t bg);

// Simple colour palette – mirrors the C++ Colors namespace used elsewhere.
#define COLOR_WINDOW_BG  0xFF12121Au   // dark window background
#define COLOR_BAR_BG     0xFF0E0E16u   // title bar background
#define COLOR_BORDER     0xFF2A2A3Cu   // window border
#define COLOR_TEXT       0xFFE8E8F0u   // title text (light)

#define TITLEBAR_HEIGHT  32
#define BORDER_WIDTH      2

void wm_draw_window_decorations(const xdg_toplevel_t *win, void *fb, uint32_t stride)
{
    if (!win || !fb) return;
    const wl_surface_t *surf = win->surface;
    if (!surf) return;

    // Geometry of the window – stored in the surface structure.
    int32_t x = surf->x;
    int32_t y = surf->y;
    int32_t w = surf->current_buffer ? surf->current_buffer->width : 0;
    int32_t h = surf->current_buffer ? surf->current_buffer->height : 0;
    if (w <= 0 || h <= 0) return;

    // Background (including border area).
    fb_fill_rect(fb, stride, x, y, w, h, COLOR_WINDOW_BG);

    // Title bar – full width, fixed height.
    fb_fill_rect(fb, stride, x, y, w, TITLEBAR_HEIGHT, COLOR_BAR_BG);

    // Border – 1‑pixel rectangle around the window.
    fb_draw_rect(fb, stride, x, y, w, h, COLOR_BORDER);

    // Optional separator line between title bar and client area.
    fb_fill_rect(fb, stride, x, y + TITLEBAR_HEIGHT, w, 1, COLOR_BORDER);

    // Draw the window title if present.
    if (win->title && win->title[0]) {
        // Leave a small left padding and centre vertically within the title bar.
        int32_t text_x = x + BORDER_WIDTH + 4;
        int32_t text_y = y + (TITLEBAR_HEIGHT - 16) / 2; // 16px font height
        fb_draw_text(fb, stride, text_x, text_y, win->title, COLOR_TEXT, 0x00000000u);
    }
}
