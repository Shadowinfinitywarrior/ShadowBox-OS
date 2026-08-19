// ═══════════════════════════════════════════════════════════════════════════
//  Widget.hpp  —  Base class for every GUI element in ShadowBox OS
//
//  All coordinates are in screen pixels; origin = top-left corner.
//  The widget tree is a classic parent→children ownership model where
//  children are NOT owned (deleted) by the parent — lifetime is managed
//  externally (e.g. by the Compositor or by stack allocation in userland).
//
//  Constraints:
//    • Freestanding C++17 — no exceptions, no RTTI, no STL headers.
//    • Uses only malloc / free / realloc from <cstdlib>.
//    • Raw function pointers instead of std::function.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstdint>
#include <cstddef>
#include "c_std.h"

// ── Forward declarations ──────────────────────────────────────────────────
class Compositor;
class InputRouter;

#include "Colors.hpp"
#include "Types.hpp"

// ─────────────────────────────────────────────────────────────────────────
//  Widget flags
// ─────────────────────────────────────────────────────────────────────────
enum WidgetFlags : uint32_t {
    WF_NONE      = 0,
    WF_VISIBLE   = 1u << 0,
    WF_ENABLED   = 1u << 1,
    WF_FOCUSABLE = 1u << 2,
    WF_FOCUSED   = 1u << 3,
    WF_HOVERED   = 1u << 4,
    WF_PRESSED   = 1u << 5,
    WF_CLIP      = 1u << 6,   // clip paint to own bounds
    WF_OPAQUE    = 1u << 7,   // does not need bg erase from parent
    WF_DRAGGABLE = 1u << 8,
    WF_ANIMATED  = 1u << 9,   // has active animation
};

// ═══════════════════════════════════════════════════════════════════════════
//  Widget  —  Abstract base class
// ═══════════════════════════════════════════════════════════════════════════
class Widget {
public:
    // ── Callback types (raw function pointers — no std::function) ─────────
    using PaintFn = void(*)(Widget* self, const Rect& dirty,
                             void* fb, uint32_t stride);
    using EventFn = bool(*)(Widget* self, const InputEvent& ev);
    using VoidFn  = void(*)(Widget* self);

    // ── Construction / destruction ─────────────────────────────────────────
    explicit Widget(Widget* parent = nullptr);
    virtual ~Widget();

    // Non-copyable
    Widget(const Widget&)            = delete;
    Widget& operator=(const Widget&) = delete;

    // ── Tree management ────────────────────────────────────────────────────
    void    add_child   (Widget* child);
    void    remove_child(Widget* child);
    Widget* parent      () const { return parent_; }
    Widget* child_at    (int idx) const;
    int     child_count () const { return child_count_; }

    // Deepest visible widget under screen_pt; nullptr if miss.
    Widget* hit_test(Point screen_pt) const;

    // ── Geometry ───────────────────────────────────────────────────────────
    void  set_rect(const Rect& r);
    void  set_pos (int32_t x, int32_t y);
    void  set_size(int32_t w, int32_t h);
    Rect  rect()        const { return rect_; }
    Rect  screen_rect() const;
    Point screen_pos()  const;

    // ── Flags ──────────────────────────────────────────────────────────────
    void set_visible  (bool v);
    void set_enabled  (bool e);
    void set_focusable(bool f);
    bool visible()   const { return (flags_ & WF_VISIBLE)   != 0; }
    bool enabled()   const { return (flags_ & WF_ENABLED)   != 0; }
    bool focused()   const { return (flags_ & WF_FOCUSED)   != 0; }
    bool hovered()   const { return (flags_ & WF_HOVERED)   != 0; }
    bool pressed()   const { return (flags_ & WF_PRESSED)   != 0; }
    uint32_t flags() const { return flags_; }
    uint32_t bg_color() const { return bg_color_; }
    void set_bg_color(uint32_t c) { bg_color_ = c; mark_dirty(); }
    uint32_t fg_color() const { return fg_color_; }
    void set_fg_color(uint32_t c) { fg_color_ = c; mark_dirty(); }

    void set_flag(WidgetFlags f, bool on);

    // ── Paint ──────────────────────────────────────────────────────────────
    void mark_dirty();
    void mark_dirty(const Rect& local_rect);

    virtual void paint         (const Rect& dirty_screen, void* fb, uint32_t stride);
    virtual void paint_self    (const Rect& /*dirty*/, void* /*fb*/, uint32_t /*stride*/) {}
    virtual void paint_children(const Rect& dirty_screen, void* fb, uint32_t stride);

    // ── Input dispatch (return true = event consumed) ──────────────────────
    virtual bool on_event        (const InputEvent& ev);
    virtual bool on_mouse_move   (const InputEvent& /*ev*/) { return false; }
    virtual bool on_mouse_press  (const InputEvent& /*ev*/) { return false; }
    virtual bool on_mouse_release(const InputEvent& /*ev*/) { return false; }
    virtual bool on_mouse_scroll (const InputEvent& /*ev*/) { return false; }
    virtual bool on_key_press    (const InputEvent& /*ev*/) { return false; }
    virtual bool on_key_release  (const InputEvent& /*ev*/) { return false; }
    virtual void on_focus_gained () {}
    virtual void on_focus_lost   () {}
    virtual void on_resize       () {}
    // Tick (frame update) — default no-op for widgets
    virtual void tick(int dt_ms) {}

    // ── Per-instance C-callable overrides (optional) ──────────────────────
    PaintFn  custom_paint = nullptr;
    EventFn  custom_event = nullptr;
    VoidFn   on_clicked   = nullptr;   // shortcut fired by Button
    void*    user_data    = nullptr;

    // ── Rounded rectangle & shadow support ────────────────────────────────
    void draw_rounded_rect(const Rect& r, uint32_t corner_radius,
                           uint32_t fill_color, uint32_t shadow_color,
                           void* fb, uint32_t stride);
    void draw_shadow(const Rect& r, uint32_t shadow_offset,
                     uint32_t shadow_color, void* fb, uint32_t stride);

    // ── Z-order ────────────────────────────────────────────────────────────
    void raise_to_top   ();
    void lower_to_bottom();

    // ── Debug tag ─────────────────────────────────────────────────────────
    char tag[32] = {};
    void set_tag(const char* t) {
        int i = 0;
        while (t[i] && i < 31) { tag[i] = t[i]; ++i; }
        tag[i] = '\0';
    }

    // ── Dirty list (accessible to Compositor) ─────────────────────────────
    DirtyList& dirty() { return dirty_; }

protected:
    Widget*  parent_      = nullptr;
    Widget** children_    = nullptr;
    int      child_count_ = 0;
    int      child_cap_   = 0;
    Rect     rect_        = {};         // relative to parent
    uint32_t flags_       = WF_VISIBLE | WF_ENABLED;
    DirtyList dirty_      = {};

    // Theme colors (derived classes can override)
    uint32_t bg_color_    = LightTheme::WindowBg;
    uint32_t fg_color_    = LightTheme::Text;

    void grow_children();
};
