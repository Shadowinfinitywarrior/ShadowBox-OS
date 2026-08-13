// MouseCursor.cpp — Implements a simple mouse cursor widget using the bitmap defined in cursor.hpp

#include "MouseCursor.hpp"
#include <cstdint>

MouseCursor::MouseCursor(InputRouter* router)
    : Widget(nullptr), router_(router)
{
    // Initialize widget as visible and set its size to the cursor bitmap dimensions.
    set_flag(WF_VISIBLE, true);
    set_tag("MouseCursor");
    // Position at the current mouse coordinates.
    int32_t mx = router_ ? router_->mouse_x() : 0;
    int32_t my = router_ ? router_->mouse_y() : 0;
    set_rect({ mx, my, CursorBitmap::W, CursorBitmap::H });
}

void MouseCursor::paint_self(const Rect& dirty, void* fb, uint32_t stride)
{
    if (!router_) return;
    // Determine the cursor's screen rectangle.
    int32_t cx = router_->mouse_x();
    int32_t cy = router_->mouse_y();
    Rect cursor_rect { cx, cy, CursorBitmap::W, CursorBitmap::H };
    // Intersect with the dirty region supplied by the compositor.
    Rect clip = cursor_rect.intersection(dirty);
    if (clip.empty()) return;

    // Direct pixel write – assumes 32‑bit ARGB framebuffer.
    uint8_t* base = static_cast<uint8_t*>(fb);
    for (int32_t y = clip.y; y < clip.y + clip.h; ++y) {
        for (int32_t x = clip.x; x < clip.x + clip.w; ++x) {
            int rel_x = x - cx;
            int rel_y = y - cy;
            if (CursorBitmap::pixel(rel_x, rel_y)) {
                // Black foreground pixel.
                uint32_t* p = reinterpret_cast<uint32_t*>(base + y * stride + x * 4);
                *p = CursorBitmap::FG;
            }
        }
    }
}
