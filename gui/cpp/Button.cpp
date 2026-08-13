// Button.cpp  —  Push-button implementation
#include "Button.hpp"
#include <cstring>

extern "C" {
    void fb_fill_rect       (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_fill_rect_round (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_rect_round (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_text       (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    int  fb_text_width      (const char* s);
}

Button::Button(Widget* parent) : Widget(parent) {
    set_flag(WF_FOCUSABLE, true);
    set_tag("Button");
}

void Button::set_label(const char* text) {
    int i = 0;
    while (text[i] && i < 127) { label_[i] = text[i]; ++i; }
    label_[i] = '\0';
    mark_dirty();
}

void Button::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr = screen_rect();

    // Select colour by state
    Color bg = bg_normal;
    Color fg = fg_normal;
    if (!enabled()) {
        bg = dim(bg_normal, 100);
        fg = dim(fg_normal, 100);
    } else if (pressed()) {
        bg = bg_press;
        fg = fg_hover;
    } else if (hovered()) {
        bg = bg_hover;
        fg = fg_hover;
    }

    // Rounded background fill
    fb_fill_rect_round(fb, stride,
                       sr.x, sr.y, sr.w, sr.h,
                       bg, corner_radius);

    // Focus ring (2px outside border)
    if (focused()) {
        fb_draw_rect_round(fb, stride,
                           sr.x - 2, sr.y - 2, sr.w + 4, sr.h + 4,
                           Colors::AccentHover, corner_radius + 2);
    }

    // Centred label
    int text_w = fb_text_width(label_) * FONT_W;
    int text_x = sr.x + (sr.w - text_w) / 2;
    int text_y = sr.y + (sr.h - FONT_H) / 2;
    fb_draw_text(fb, stride, text_x, text_y, label_, fg, Colors::Transparent);
}

bool Button::on_mouse_move(const InputEvent& ev) {
    bool was = hovered();
    bool now = screen_rect().contains(ev.pos);
    set_flag(WF_HOVERED, now);
    if (was != now) mark_dirty();
    return now;
}

bool Button::on_mouse_press(const InputEvent& ev) {
    if (ev.button != MouseButton::Left) return false;
    if (!screen_rect().contains(ev.pos)) return false;
    set_flag(WF_PRESSED, true);
    mark_dirty();
    return true;
}

bool Button::on_mouse_release(const InputEvent& ev) {
    if (ev.button != MouseButton::Left) return false;
    bool was_pressed = pressed();
    set_flag(WF_PRESSED, false);
    mark_dirty();
    if (was_pressed && screen_rect().contains(ev.pos)) {
        if (on_clicked) on_clicked(this);
    }
    return was_pressed;
}
