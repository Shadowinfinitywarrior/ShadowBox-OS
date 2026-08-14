#include "wm.h"
#include "compositor.h"
#include "../ui/theme.h"
#include "icon.h"

// Forward declarations for low‑level drawing primitives from gui/c/fb_draw.c
extern void fb_fill_rect(void *fb, uint32_t stride,
                         int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t color);
extern void fb_draw_rect(void *fb, uint32_t stride,
                         int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t color);
extern void fb_fill_rect_round(void *fb, uint32_t stride,
                               int32_t x, int32_t y, int32_t w, int32_t h,
                               uint32_t color, int32_t r);
extern void fb_draw_rect_round(void *fb, uint32_t stride,
                               int32_t x, int32_t y, int32_t w, int32_t h,
                               uint32_t color, int32_t r);
extern void fb_draw_text(void *fb, uint32_t stride,
                        int32_t x, int32_t y,
                        const char *s, uint32_t fg, uint32_t bg);

// Visual parameters
#define TITLEBAR_HEIGHT 28
#define BORDER_WIDTH 2
#define CORNER_RADIUS 8

// Button layout
#define BUTTON_SIZE 16
#define BUTTON_PADDING 4
#define BUTTON_SPACING 4

// Helper to combine alpha with an RGB colour
static inline uint32_t argb(uint8_t a, uint32_t rgb) {
    return ((uint32_t)a << 24) | (rgb & 0xFFFFFF);
}

void wm_draw_window_decorations(const xdg_toplevel_t *win, void *fb, uint32_t stride)
{
    if (!win || !fb) return;
    const wl_surface_t *surf = win->surface;
    if (!surf) return;

    int32_t x = surf->x;
    int32_t y = surf->y;
    int32_t w = surf->current_buffer ? surf->current_buffer->width : 0;
    int32_t h = surf->current_buffer ? surf->current_buffer->height : 0;
    if (w <= 0 || h <= 0) return;

    /* Theme‑based colours */
    uint32_t bg_color   = argb(0xCC, ui_theme_get_background_color());   // 80 % opacity
    uint32_t title_color= argb(0xDD, ui_theme_get_accent_color());      // slightly brighter
    uint32_t border_color = argb(0xFF, ui_theme_get_secondary_color());
    uint32_t text_color = argb(0xFF, ui_theme_get_foreground_color());

    /* Shadow – simple offset, semi‑transparent */
    const uint32_t SHADOW_COLOR = 0x44000000u;
    const int SHADOW_OFFSET = 4;
    fb_fill_rect(fb, stride,
                 x + SHADOW_OFFSET, y + SHADOW_OFFSET,
                 w, h,
                 SHADOW_COLOR);

    /* Window background with rounded corners */
    fb_fill_rect_round(fb, stride, x, y, w, h, bg_color, CORNER_RADIUS);

    /* Title bar (full width, sits inside the rounded background) */
    fb_fill_rect(fb, stride, x, y, w, TITLEBAR_HEIGHT, title_color);

    /* Border – rounded */
    fb_draw_rect_round(fb, stride, x, y, w, h, border_color, CORNER_RADIUS);

    /* Separator line between title bar and client area */
    fb_fill_rect(fb, stride, x, y + TITLEBAR_HEIGHT, w, 1, border_color);
    const uint32_t SEPARATOR_SHADOW = 0x66000000u;
    fb_fill_rect(fb, stride, x, y + TITLEBAR_HEIGHT + 1, w, 1, SEPARATOR_SHADOW);

    /* Title text */
    if (win->title && win->title[0]) {
        int32_t text_x = x + BORDER_WIDTH + 4;
        int32_t text_y = y + (TITLEBAR_HEIGHT - 16) / 2; // 16px font height
        fb_draw_text(fb, stride, text_x, text_y, win->title,
                    text_color, 0x00000000u);
    }

    /* Window control buttons */
    int btn_y = y + (TITLEBAR_HEIGHT - BUTTON_SIZE) / 2;
    int btn_x_close = x + w - BUTTON_PADDING - BUTTON_SIZE;
    int btn_x_max   = btn_x_close - BUTTON_SPACING - BUTTON_SIZE;
    int btn_x_min   = btn_x_max   - BUTTON_SPACING - BUTTON_SIZE;

    // Button background – semi‑transparent dark
    uint32_t button_bg = argb(0xAA, 0x404040);
    fb_fill_rect_round(fb, stride, btn_x_close, btn_y, BUTTON_SIZE, BUTTON_SIZE,
                       button_bg, CORNER_RADIUS/2);
    fb_fill_rect_round(fb, stride, btn_x_max, btn_y, BUTTON_SIZE, BUTTON_SIZE,
                       button_bg, CORNER_RADIUS/2);
    fb_fill_rect_round(fb, stride, btn_x_min, btn_y, BUTTON_SIZE, BUTTON_SIZE,
                       button_bg, CORNER_RADIUS/2);

    // Iconic buttons – draw 16x16 icons
    int stride_px = stride / 4;
    icon_blit(icon_close_16, 16, btn_x_close, btn_y, (uint32_t*)fb, stride_px);
    icon_blit(icon_max_16, 16, btn_x_max, btn_y, (uint32_t*)fb, stride_px);
    icon_blit(icon_min_16, 16, btn_x_min, btn_y, (uint32_t*)fb, stride_px);
}
