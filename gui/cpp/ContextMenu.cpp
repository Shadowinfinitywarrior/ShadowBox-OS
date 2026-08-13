// ContextMenu.cpp — Implementation of a simple popup context menu widget
#include "ContextMenu.hpp"

extern "C" {
    // Drawing helpers from the framebuffer driver.
    void fb_fill_rect(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_draw_rect(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_draw_text(void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    int  fb_text_width(const char*);
}

ContextMenu::ContextMenu(Widget* parent) : Widget(parent) {
    set_tag("ContextMenu");
    // Context menus are not focusable by default; they react to mouse only.
    set_flag(WF_FOCUSABLE, false);
    // Default background colour.
    // Size will be adjusted automatically when items are added.
}

void ContextMenu::add_item(const char* label, void (*cb)(void*), void* user_data) {
    if (item_count_ >= MAX_ITEMS) return;
    Item &it = items_[item_count_];
    int i = 0;
    while (label[i] && i < 127) { it.label[i] = label[i]; ++i; }
    it.label[i] = '\0';
    it.callback = cb;
    it.data = user_data;
    ++item_count_;
    // Update widget size: width based on longest label, height based on count.
    // Simple heuristic: fixed width 200px, height = item_count * ITEM_HEIGHT.
    set_size(200, item_count_ * ITEM_HEIGHT);
    mark_dirty();
}

void ContextMenu::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr = screen_rect();
    // Background rectangle
    fb_fill_rect(fb, stride, sr.x, sr.y, sr.w, sr.h, Colors::BarBg);

    // Draw each item
    for (int i = 0; i < item_count_; ++i) {
        int item_y = sr.y + i * ITEM_HEIGHT;
        // Highlight hovered item
        if (i == hovered_index_) {
            fb_fill_rect(fb, stride, sr.x, item_y, sr.w, ITEM_HEIGHT, Colors::Accent);
        }
        // Text colour: white on hover, normal text otherwise
        uint32_t txt_col = (i == hovered_index_) ? Colors::White : Colors::Text;
        // Simple left padding
        int tx = sr.x + 8;
        int ty = item_y + (ITEM_HEIGHT - 16) / 2; // vertical centre assuming 16px font
        fb_draw_text(fb, stride, tx, ty, items_[i].label, txt_col, Colors::Transparent);
    }
    // Optional border
    fb_draw_rect(fb, stride, sr.x, sr.y, sr.w, sr.h, Colors::Border);
}

bool ContextMenu::on_mouse_move(const InputEvent& ev) {
    // Determine which item the cursor is over.
    if (!screen_rect().contains(ev.pos)) {
        if (hovered_index_ != -1) { hovered_index_ = -1; mark_dirty(); }
        return false;
    }
    int local_y = ev.pos.y - screen_pos().y;
    int idx = local_y / ITEM_HEIGHT;
    if (idx < 0 || idx >= item_count_) idx = -1;
    if (idx != hovered_index_) {
        hovered_index_ = idx;
        mark_dirty();
    }
    return true;
}

bool ContextMenu::on_mouse_press(const InputEvent& ev) {
    if (ev.button != MouseButton::Left) return false;
    if (!screen_rect().contains(ev.pos)) return false;
    int local_y = ev.pos.y - screen_pos().y;
    int idx = local_y / ITEM_HEIGHT;
    if (idx >= 0 && idx < item_count_) {
        if (items_[idx].callback) {
            items_[idx].callback(items_[idx].data);
        }
        // Hide after selection.
        set_visible(false);
        mark_dirty();
        return true;
    }
    return false;
}
