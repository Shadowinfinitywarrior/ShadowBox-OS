// ScrollView.cpp  —  Scrollable viewport implementation
#include "ScrollView.hpp"

extern "C" {
    void fb_fill_rect      (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_fill_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
}

// ─────────────────────────────────────────────────────────────────────────

ScrollView::ScrollView(Widget* parent) : Widget(parent) {
    set_tag("ScrollView");
    set_flag(WF_CLIP, true);
}

void ScrollView::set_content(Widget* c) {
    if (content_) remove_child(content_);
    content_ = c;
    if (c) {
        add_child(c);
        clamp_scroll();
    }
}

void ScrollView::scroll_to(int x, int y) {
    scroll_x_ = x;
    scroll_y_ = y;
    clamp_scroll();
    mark_dirty();
}

// ─── Clamping ─────────────────────────────────────────────────────────────

void ScrollView::clamp_scroll() {
    if (!content_) { scroll_x_ = scroll_y_ = 0; return; }

    int max_x = content_->rect().w - rect_.w + SCROLLBAR_W;
    int max_y = content_->rect().h - rect_.h;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;
    if (scroll_x_ < 0) scroll_x_ = 0;
    if (scroll_x_ > max_x) scroll_x_ = max_x;
    if (scroll_y_ < 0) scroll_y_ = 0;
    if (scroll_y_ > max_y) scroll_y_ = max_y;

    // Reposition content widget so it scrolls correctly
    if (content_)
        content_->set_pos(-scroll_x_, -scroll_y_);
}

// ─── Paint ────────────────────────────────────────────────────────────────

void ScrollView::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    // Background (let the content cover it; just fill any uncovered area)
    Rect sr = screen_rect();
    fb_fill_rect(fb, stride, sr.x, sr.y, sr.w, sr.h, Colors::WindowBg);
    draw_scrollbar_v(fb, stride);
}

void ScrollView::draw_scrollbar_v(void* fb, uint32_t stride) {
    if (!content_) return;
    int content_h = content_->rect().h;
    if (content_h <= rect_.h) return;   // no overflow, no scrollbar needed

    Rect sr  = screen_rect();
    int  sbx = sr.x + sr.w - SCROLLBAR_W;
    int  sby = sr.y;
    int  sbh = rect_.h;

    // Track
    fb_fill_rect(fb, stride, sbx, sby, SCROLLBAR_W, sbh, scrollbar_bg);

    // Thumb
    int thumb_h = sbh * sbh / content_h;
    if (thumb_h < MIN_THUMB) thumb_h = MIN_THUMB;
    int thumb_y = sby + scroll_y_ * (sbh - thumb_h) / (content_h - rect_.h);

    fb_fill_rect_round(fb, stride,
                       sbx + 1, thumb_y,
                       SCROLLBAR_W - 2, thumb_h,
                       scrollbar_thumb, 4);
}

void ScrollView::paint_children(const Rect& dirty_screen, void* fb, uint32_t stride) {
    // Paint content only — children_ contains exactly content_
    if (content_ && content_->visible())
        content_->paint(dirty_screen, fb, stride);
}

// ─── Input ────────────────────────────────────────────────────────────────

bool ScrollView::on_mouse_scroll(const InputEvent& ev) {
    scroll_y_ += ev.scroll_delta * SCROLL_STEP;
    clamp_scroll();
    mark_dirty();
    return true;
}
