// Toggle.cpp  —  Checkbox/Toggle widget implementation with modern styling
#include "Toggle.hpp"
#include "Widget.hpp"

// External C drawing functions from fb_draw.c
extern "C" {
    void fb_fill_rect      (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_draw_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_text      (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    int  fb_text_width     (const char* s);
}

// ── System colors for Toggle ──────────────────────────────────────────────
static constexpr uint32_t TOGGLE_BG_UNCHECKED = 0xFFFFFFFF;  // White
static constexpr uint32_t TOGGLE_BG_CHECKED   = 0xFF0066FF;   // Blue
static constexpr uint32_t TOGGLE_INDICATOR    = 0xFFFFFFFF;   // White checkmark
static constexpr uint32_t TOGGLE_TEXT_UNCHECKED = 0xFF808080; // Gray label
static constexpr uint32_t TOGGLE_TEXT_CHECKED   = 0xFFFFFFFF; // White label

// ── Paint ─────────────────────────────────────────────────────────────────────

void Toggle::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr = screen_rect();

    // Draw background - checked/unchecked
    if (state_ == TOGGLE_ON) {
        fb_fill_rect(fb, stride, sr.x, sr.y, sr.w, sr.h, TOGGLE_BG_CHECKED);
        // Draw checkmark (simple "V" shape)
        int cx = sr.x + sr.w / 2;
        int cy = sr.y + sr.h / 2;
        int size = sr.h / 3;
        // Draw a V checkmark
        fb_draw_rect(fb, stride,
                     cx - size/4, cy - size/4, size/2, 2, TOGGLE_INDICATOR);  // horizontal
        fb_draw_rect(fb, stride,
                     cx - size/4, cy - size/4, 2, size/2, TOGGLE_INDICATOR);  // vertical
    } else {
        fb_fill_rect(fb, stride, sr.x, sr.y, sr.w, sr.h, TOGGLE_BG_UNCHECKED);
        // Draw border
        fb_draw_rect_round(fb, stride, sr.x, sr.y, sr.w, sr.h, TOGGLE_BG_UNCHECKED, 4);
    }
}

// ── Input ────────────────────────────────────────────────────────────────────

bool Toggle::on_mouse_press(const InputEvent& ev) {
    if (!screen_rect().contains(ev.pos)) return false;
    // Toggle state
    state_ = (state_ == TOGGLE_ON) ? TOGGLE_OFF : TOGGLE_ON;
    mark_dirty();
    if (on_toggled_) on_toggled_(this, state_);
    return true;
}

bool Toggle::on_mouse_move(const InputEvent& ev) {
    // No special hover effect needed for simple toggle
    return false;
}

bool Toggle::on_mouse_release(const InputEvent& ev) {
    // Toggle already handled in press - could add visual feedback here
    return false;
}

// ── Constructor / Destructor ────────────────────────────────────────────────

Toggle::Toggle(Widget* parent) : Widget(parent) {
    set_tag("Toggle");
    set_flag(WF_FOCUSABLE, true);
    set_state(TOGGLE_OFF);
}

Toggle::~Toggle() {}