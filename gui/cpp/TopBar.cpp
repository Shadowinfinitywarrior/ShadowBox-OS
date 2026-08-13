#include "TopBar.hpp"

extern "C" {
    void fb_fill_rect(void* fb, uint32_t stride, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
}

TopBar::TopBar(Widget* parent, int screen_width) : Widget(parent) {
    set_pos(0, 0);
    set_size(screen_width, 24);
    set_flag(WF_FOCUSABLE, false);

    title_label_ = new Label(this);
    title_label_->set_pos(10, 0);
    title_label_->set_size(200, 24);
    title_label_->set_text("ShadowBox OS");
    title_label_->set_color(0xFFFFFF); // White text
    title_label_->set_align(TextAlign::Left);
}

void TopBar::paint_self(const Rect& clip, void* fb, uint32_t stride) {
    Rect abs_rect = screen_rect();
    Rect r = abs_rect.intersection(clip);
    if (r.w <= 0 || r.h <= 0) return;

    // Draw solid top bar background
    fb_fill_rect(fb, stride, r.x, r.y, r.w, r.h, 0x2C3E50); // Dark blue/gray
}
