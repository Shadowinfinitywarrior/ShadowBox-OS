// Button.hpp  —  Clickable, stateful push-button widget
#pragma once
#include "Widget.hpp"

// Font metrics (8×16 bitmap font defined in fb_draw.c)
static constexpr int FONT_W = 8;
static constexpr int FONT_H = 16;

// ═══════════════════════════════════════════════════════════════════════════
//  Button
// ═══════════════════════════════════════════════════════════════════════════
class Button : public Widget {
public:
    explicit Button(Widget* parent = nullptr);

    void        set_label(const char* text);
    const char* label() const { return label_; }

    // Colours — all public so callers can theme freely
    Color bg_normal   = Colors::MidGray;
    Color bg_hover    = Colors::Accent;
    Color bg_press    = Colors::AccentActive;
    Color fg_normal   = Colors::Text;
    Color fg_hover    = Colors::White;
    Color border_col  = Colors::Border;
    int   corner_radius = 6;

protected:
    void paint_self      (const Rect& dirty, void* fb, uint32_t stride) override;
    bool on_mouse_move   (const InputEvent& ev) override;
    bool on_mouse_press  (const InputEvent& ev) override;
    bool on_mouse_release(const InputEvent& ev) override;
    void on_focus_gained () override { mark_dirty(); }
    void on_focus_lost   () override { mark_dirty(); }

private:
    char label_[128] = {};
};
