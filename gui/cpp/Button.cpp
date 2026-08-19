// Button.cpp  —  Push-button implementation with modern visual styling
#include "Button.hpp"
#include <cstring>

// External C drawing functions from fb_draw.c
extern "C" {
    void fb_fill_rect       (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_fill_rect_round (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_rect_round (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_text       (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    int  fb_text_width      (const char* s);
}

// ── System colors (use theme if available) ──────────────────────────────────
// Fallback colors if no theme is active
static constexpr uint32_t BG_NORMAL     = 0xFFE5E5E5;
static constexpr uint32_t BG_HOVER      = 0xFFD0D0D0;
static constexpr uint32_t BG_PRESSED    = 0xFFA0A0A0;
static constexpr uint32_t FG_NORMAL     = 0xFF000000;
static constexpr uint32_t FG_HOVER      = 0xFFFFFFFF;
static constexpr uint32_t FG_PRESSED    = 0xFFFFFFFF;

// ── Button state colors from theme ──────────────────────────────────────────
static uint32_t button_bg_normal() {
    extern uint32_t bg_color();
    uint32_t c = bg_color();
    return c ? c : BG_NORMAL;
}

static uint32_t button_bg_hover() {
    extern uint32_t bg_color();
    uint32_t c = bg_color();
    return c ? dim(c, 150) : dim(BG_HOVER, 150);
}

static uint32_t button_bg_pressed() {
    extern uint32_t bg_color();
    uint32_t c = bg_color();
    return c ? dim(c, 200) : dim(BG_PRESSED, 200);
}

// Removed button_fg_pressed - using button_fg_hover instead
static uint32_t button_fg_normal() {
    extern uint32_t fg_color();
    uint32_t c = fg_color();
    return c ? c : FG_NORMAL;
}

static uint32_t button_fg_hover() {
    extern uint32_t fg_color();
    uint32_t c = fg_color();
    return c ? c : FG_HOVER;
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void Button::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr = screen_rect();

    // Select colours by state using theme-aware functions
    uint32_t bg = button_bg_normal();
    uint32_t fg = button_fg_normal();

    if (!enabled()) {
        bg = dim(bg, 100);
        fg = dim(fg, 100);
    } else if (pressed()) {
        bg = button_bg_pressed();
        fg = button_fg_normal();  // Use normal fg for pressed state
    } else if (hovered()) {
        bg = button_bg_hover();
        fg = button_fg_hover();
    }

    // Rounded background fill with corner radius
    int corner_radius = 6;  // Modern rounded corners
    fb_fill_rect_round(fb, stride,
                       sr.x, sr.y, sr.w, sr.h,
                       bg, corner_radius);

    // Focus ring (2px outside border) when focused
    if (focused()) {
        fb_draw_rect_round(fb, stride,
                           sr.x - 2, sr.y - 2, sr.w + 4, sr.h + 4,
                           accent_color(), corner_radius + 2);
    }

    // Centred label
    int text_w = fb_text_width(label_) * FONT_W;
    int text_x = sr.x + (sr.w - text_w) / 2;
    int text_y = sr.y + (sr.h - FONT_H) / 2;
    fb_draw_text(fb, stride, text_x, text_y, label_, fg, bg);
}