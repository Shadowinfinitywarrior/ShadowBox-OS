// ═══════════════════════════════════════════════════════════════════════════
// Compositor.hpp — Root widget manager and frame renderer
//
// All coordinates are in screen pixels; origin = top-left corner.
// The widget tree is a classic parent→children ownership model where
// children are NOT owned (deleted) by the parent — lifetime is managed
// externally (e.g. by the Compositor or by stack allocation in userland).
//
// Constraints:
// • Freestanding C++17 — no exceptions, no RTTI, no STL headers.
// • Uses only malloc / free / realloc from <cstdlib>.
// • Raw function pointers instead of std::function.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

#include "Widget.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// Compositor
//
// Manages an ordered list of top-level widgets (windows) and drives
// the incremental repaint cycle each frame.
//
// Usage (from desktop.c via the C bridge):
// Compositor* comp = compositor_create(fb, stride, width, height);
// compositor_add_root(comp, my_window);
// // each frame:
// compositor_frame(comp);
// ═══════════════════════════════════════════════════════════════════════════
class Compositor {
public:
 static constexpr int MAX_ROOTS = 32;
 static constexpr int MAX_DIRTY = 64;

 // fb : pointer to the mapped framebuffer (screen pixels)
 // stride : bytes per row (pitch)
 // width / height : screen dimensions in pixels
 Compositor(void* fb, uint32_t stride,
     uint32_t width, uint32_t height);

 // Add / remove a top-level widget (Compositor does NOT own it)
 void add_root (Widget* w, bool raise = true);
 void remove_root(Widget* w);

 // Render one frame: collect dirty regions, repaint, clear dirty lists
 void frame(int dt_ms = 16);

 // Force-invalidate the full screen on next frame
 void invalidate_all();

 // Focused widget management
 Widget* focused() const { return focused_; }
 void set_focus(Widget* w);

 // Screen dimensions
 uint32_t width() const { return width_; }
 uint32_t height() const { return height_; }

 // The back-buffer (same size as screen) for double-buffering
 // Set by the caller after allocating via sbrk / mmap.
 void* backbuf = nullptr;

private:
 void* fb_ = nullptr;
 uint32_t stride_ = 0;
 uint32_t width_ = 0;
 uint32_t height_ = 0;

 Widget* roots_[MAX_ROOTS] = {};
 int root_count_ = 0;

 Widget* focused_ = nullptr;

 // Accumulated dirty rects from all roots this frame
 Rect dirty_[MAX_DIRTY];
 int dirty_count_ = 0;
 bool full_repaint_ = true;

 void collect_dirty();
 void repaint();
 void blit_to_screen();
 void add_dirty(const Rect& r);
 // Advance animations on all root widgets
 void animate_roots(int dt_ms);
};
// ─── C-callable bridge (for desktop.c) ────────────────────────────────────
extern "C" {
 Compositor* compositor_create(void* fb, uint32_t stride,
     uint32_t w, uint32_t h);
 void compositor_destroy (Compositor* c);
 void compositor_add_root (Compositor* c, Widget* w, int raise);
 void compositor_remove_root (Compositor* c, Widget* w);
 void compositor_frame (Compositor* c);
 void compositor_invalidate (Compositor* c);
 void compositor_set_focus (Compositor* c, Widget* w);
 Widget* compositor_focused (Compositor* c);
 void compositor_set_backbuf (Compositor* c, void* buf);
}
