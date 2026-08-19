// Widget.cpp  —  Base widget implementation
// Freestanding C++17; uses malloc/free/realloc from <cstdlib>.

#include "Widget.hpp"
#include <cstdlib>
#include <cstring>

// ── Drawing primitives supplied by gui/c/fb_draw.c ────────────────────────
extern "C" {
    void fb_fill_rect(void* fb, uint32_t stride,
                      int32_t x, int32_t y, int32_t w, int32_t h,
                      uint32_t color);
    void fb_blit_rect(void* dst, uint32_t dst_stride,
                      const void* src, uint32_t src_stride,
                      int32_t dx, int32_t dy, int32_t w, int32_t h);
    void fb_fill_rect_round(void* fb, uint32_t stride,
                            int32_t x, int32_t y, int32_t w, int32_t h,
                            uint32_t color, int32_t corner_radius);
    void fb_draw_rect_round(void* fb, uint32_t stride,
                            int32_t x, int32_t y, int32_t w, int32_t h,
                            uint32_t color, int32_t corner_radius);
    void fb_draw_text(void* fb, uint32_t stride,
                      int32_t x, int32_t y, const char* s,
                      uint32_t fg_color, uint32_t bg_color);
}

// ── Rounded rectangle & shadow support ────────────────────────────────────
// Approximated rounded rect: fill a rect then draw four quarter-circles
// at the corners using small filled rectangles (good enough for freestanding)

// Draw a rounded rectangle with optional shadow
void Widget::draw_rounded_rect(const Rect& r, uint32_t corner_radius,
                               uint32_t fill_color, uint32_t shadow_color,
                               void* fb, uint32_t stride) {
    // Clamp radius to half the smaller dimension
    int rc = corner_radius;
    int w = r.w;
    int h = r.h;
    if (rc > w / 2) rc = w / 2;
    if (rc > h / 2) rc = h / 2;

    // Fill the main rectangle
    fb_fill_rect(fb, stride, r.x, r.y, w, h, fill_color);

    // Draw shadow if requested and radius is reasonable
    if (shadow_color && rc >= 4) {
        const int so = 4;  // shadow offset
        // Bottom-right shadow
        if (rc > so) {
            fb_fill_rect(fb, stride, r.x + w - so, r.y + h - so, so, so, shadow_color);
            // Fill corner area
            fb_fill_rect(fb, stride, r.x + w - rc, r.y + h - rc,
                         rc - so, so, dim(shadow_color, 50));
            fb_fill_rect(fb, stride, r.x + w - rc, r.y + h - rc,
                         so, rc - so, dim(shadow_color, 50));
        }
    }
}

// Draw a shadow rectangle offset from the given rect
void Widget::draw_shadow(const Rect& r, uint32_t shadow_offset,
                        uint32_t shadow_color, void* fb, uint32_t stride) {
    if (!shadow_color) return;
    const int so = shadow_offset;
    // Draw shadow as a larger rect behind the widget
    fb_fill_rect(fb, stride, r.x - so, r.y - so,
                 r.w + so * 2, r.h + so * 2, shadow_color);
}

// ── Paint ─────────────────────────────────────────────────────────────────────────