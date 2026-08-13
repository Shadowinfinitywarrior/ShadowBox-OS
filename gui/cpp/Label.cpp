// Label.cpp  —  Static text widget implementation
#include "Label.hpp"

extern "C" {
    void fb_draw_text     (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    void fb_draw_text_wrap(void*, uint32_t, int32_t, int32_t, int32_t, int32_t,
                           const char*, uint32_t, uint32_t);
    int  fb_text_width    (const char* s);
}

Label::Label(Widget* parent) : Widget(parent) {
    set_tag("Label");
    set_flag(WF_FOCUSABLE, false);   // labels are not interactive by default
}

void Label::set_text(const char* text) {
    int i = 0;
    while (text[i] && i < MAX_TEXT - 1) { text_[i] = text[i]; ++i; }
    text_[i] = '\0';
    mark_dirty();
}

void Label::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    if (!text_[0]) return;
    Rect sr = screen_rect();

    if (wrap_ == WrapMode::WordWrap) {
        fb_draw_text_wrap(fb, stride,
                          sr.x, sr.y, sr.w, sr.h,
                          text_, fg_, Colors::Transparent);
        return;
    }

    // Single-line with alignment
    int text_w = fb_text_width(text_) * FONT_W_ * scale_;
    int tx = sr.x;
    if (align_ == TextAlign::Center)
        tx = sr.x + (sr.w - text_w) / 2;
    else if (align_ == TextAlign::Right)
        tx = sr.x + sr.w - text_w;

    int ty = sr.y + (sr.h - FONT_H_ * scale_) / 2;
    fb_draw_text(fb, stride, tx, ty, text_, fg_, Colors::Transparent);
}
