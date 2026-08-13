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
}

// ─────────────────────────────────────────────────────────────────────────
//  Construction / destruction
// ─────────────────────────────────────────────────────────────────────────

Widget::Widget(Widget* parent)
    : parent_(nullptr)
{
    if (parent) parent->add_child(this);
}

Widget::~Widget() {
    // Remove ourselves from our parent silently (without recursion)
    if (parent_) {
        for (int i = 0; i < parent_->child_count_; ++i) {
            if (parent_->children_[i] == this) {
                parent_->child_count_--;
                ::memmove(&parent_->children_[i],
                          &parent_->children_[i + 1],
                          (size_t)(parent_->child_count_ - i) * sizeof(Widget*));
                break;
            }
        }
        parent_ = nullptr;
    }

    // Detach children (do NOT delete them — external lifetime management)
    for (int i = 0; i < child_count_; ++i)
        children_[i]->parent_ = nullptr;
    child_count_ = 0;

    if (children_) {
        ::free(children_);
        children_ = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────
//  Tree management
// ─────────────────────────────────────────────────────────────────────────

void Widget::grow_children() {
    int new_cap = (child_cap_ == 0) ? 8 : child_cap_ * 2;
    children_ = static_cast<Widget**>(
        ::realloc(children_, (size_t)new_cap * sizeof(Widget*)));
    child_cap_ = new_cap;
}

void Widget::add_child(Widget* child) {
    if (!child || child->parent_ == this) return;
    if (child->parent_) child->parent_->remove_child(child);
    if (child_count_ >= child_cap_) grow_children();
    children_[child_count_++] = child;
    child->parent_ = this;
    if (child->rect_.w > 0 && child->rect_.h > 0)
        mark_dirty(child->rect_);
}

void Widget::remove_child(Widget* child) {
    for (int i = 0; i < child_count_; ++i) {
        if (children_[i] == child) {
            mark_dirty(child->rect_);
            child->parent_ = nullptr;
            child_count_--;
            ::memmove(&children_[i], &children_[i + 1],
                      (size_t)(child_count_ - i) * sizeof(Widget*));
            return;
        }
    }
}

Widget* Widget::child_at(int idx) const {
    if (idx < 0 || idx >= child_count_) return nullptr;
    return children_[idx];
}

// ─────────────────────────────────────────────────────────────────────────
//  Geometry
// ─────────────────────────────────────────────────────────────────────────

void Widget::set_rect(const Rect& r) {
    if (parent_) parent_->mark_dirty(rect_);  // invalidate old region
    rect_ = r;
    if (parent_) parent_->mark_dirty(r);       // invalidate new region
    on_resize();
}

void Widget::set_pos(int32_t x, int32_t y) {
    set_rect({ x, y, rect_.w, rect_.h });
}

void Widget::set_size(int32_t w, int32_t h) {
    set_rect({ rect_.x, rect_.y, w, h });
}

Point Widget::screen_pos() const {
    Point p { rect_.x, rect_.y };
    const Widget* par = parent_;
    while (par) {
        p.x += par->rect_.x;
        p.y += par->rect_.y;
        par = par->parent_;
    }
    return p;
}

Rect Widget::screen_rect() const {
    Point p = screen_pos();
    return { p.x, p.y, rect_.w, rect_.h };
}

// ─────────────────────────────────────────────────────────────────────────
//  Flags
// ─────────────────────────────────────────────────────────────────────────

void Widget::set_flag(WidgetFlags f, bool on) {
    if (on) flags_ |=  (uint32_t)f;
    else    flags_ &= ~(uint32_t)f;
}

void Widget::set_visible(bool v) {
    bool was = visible();
    set_flag(WF_VISIBLE, v);
    if (was != v) mark_dirty();
}

void Widget::set_enabled(bool e) {
    set_flag(WF_ENABLED, e);
    mark_dirty();
}

void Widget::set_focusable(bool f) {
    set_flag(WF_FOCUSABLE, f);
}

// ─────────────────────────────────────────────────────────────────────────
//  Dirty tracking
// ─────────────────────────────────────────────────────────────────────────

void Widget::mark_dirty() {
    dirty_.add({ 0, 0, rect_.w, rect_.h });
    if (parent_) parent_->mark_dirty(rect_);
}

void Widget::mark_dirty(const Rect& local_rect) {
    dirty_.add(local_rect);
    if (parent_) parent_->mark_dirty(local_rect.translated(rect_.x, rect_.y));
}

// ─────────────────────────────────────────────────────────────────────────
//  Z-order
// ─────────────────────────────────────────────────────────────────────────

void Widget::raise_to_top() {
    if (!parent_) return;
    for (int i = 0; i < parent_->child_count_; ++i) {
        if (parent_->children_[i] == this) {
            ::memmove(&parent_->children_[i],
                      &parent_->children_[i + 1],
                      (size_t)(parent_->child_count_ - i - 1) * sizeof(Widget*));
            parent_->children_[parent_->child_count_ - 1] = this;
            mark_dirty();
            return;
        }
    }
}

void Widget::lower_to_bottom() {
    if (!parent_) return;
    for (int i = 0; i < parent_->child_count_; ++i) {
        if (parent_->children_[i] == this) {
            ::memmove(&parent_->children_[1],
                      &parent_->children_[0],
                      (size_t)i * sizeof(Widget*));
            parent_->children_[0] = this;
            mark_dirty();
            return;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────
//  Hit testing
// ─────────────────────────────────────────────────────────────────────────

Widget* Widget::hit_test(Point screen_pt) const {
    if (!visible()) return nullptr;
    Rect sr = screen_rect();
    if (!sr.contains(screen_pt)) return nullptr;
    // Test children in reverse order (topmost drawn last = first to receive input)
    for (int i = child_count_ - 1; i >= 0; --i) {
        Widget* hit = children_[i]->hit_test(screen_pt);
        if (hit) return hit;
    }
    return const_cast<Widget*>(this);
}

// ─────────────────────────────────────────────────────────────────────────
//  Paint
// ─────────────────────────────────────────────────────────────────────────

void Widget::paint(const Rect& dirty_screen, void* fb, uint32_t stride) {
    if (!visible()) return;
    Rect sr   = screen_rect();
    Rect clip = sr.intersection(dirty_screen);
    if (clip.empty()) return;

    paint_self(clip, fb, stride);

    if (custom_paint)
        custom_paint(this, clip, fb, stride);

    paint_children(dirty_screen, fb, stride);
}

void Widget::paint_children(const Rect& dirty_screen, void* fb, uint32_t stride) {
    for (int i = 0; i < child_count_; ++i)
        children_[i]->paint(dirty_screen, fb, stride);
}

// ─────────────────────────────────────────────────────────────────────────
//  Event dispatch
// ─────────────────────────────────────────────────────────────────────────

bool Widget::on_event(const InputEvent& ev) {
    if (!visible() || !enabled()) return false;

    if (custom_event && custom_event(this, ev)) return true;

    switch (ev.type) {
        case EventType::MouseMove:    return on_mouse_move(ev);
        case EventType::MousePress:   return on_mouse_press(ev);
        case EventType::MouseRelease: return on_mouse_release(ev);
        case EventType::MouseScroll:  return on_mouse_scroll(ev);
        case EventType::KeyPress:     return on_key_press(ev);
        case EventType::KeyRelease:   return on_key_release(ev);
        case EventType::FocusGained:  on_focus_gained(); return true;
        case EventType::FocusLost:    on_focus_lost();   return true;
        default: return false;
    }
}
