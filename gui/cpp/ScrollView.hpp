// ScrollView.hpp  —  Clips and scrolls a single content widget
#pragma once
#include "Widget.hpp"

// ═══════════════════════════════════════════════════════════════════════════
//  ScrollView
//
//  A single content widget lives inside; it can be taller/wider than
//  the scroll view's visible area. A vertical scrollbar is drawn on the
//  right edge when needed.
// ═══════════════════════════════════════════════════════════════════════════
class ScrollView : public Widget {
public:
    static constexpr int SCROLLBAR_W = 8;
    static constexpr int MIN_THUMB   = 20;
    static constexpr int SCROLL_STEP = 24;   // pixels per wheel click

    explicit ScrollView(Widget* parent = nullptr);

    // Set the single scrollable content widget (externally owned)
    void set_content(Widget* content);

    int  scroll_x() const { return scroll_x_; }
    int  scroll_y() const { return scroll_y_; }
    void scroll_to(int x, int y);

    Color scrollbar_bg    = 0x22FFFFFFu;
    Color scrollbar_thumb = Colors::MidGray;

protected:
    void paint_self    (const Rect& dirty, void* fb, uint32_t stride) override;
    void paint_children(const Rect& dirty, void* fb, uint32_t stride) override;
    bool on_mouse_scroll(const InputEvent& ev) override;

private:
    Widget* content_  = nullptr;
    int     scroll_x_ = 0;
    int     scroll_y_ = 0;

    void clamp_scroll();
    void draw_scrollbar_v(void* fb, uint32_t stride);
};
