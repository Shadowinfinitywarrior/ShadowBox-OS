// dirty_region.c – Implementation of dirty_region.h
//
// This file implements a tiny dirty‑region subsystem that can be linked
// into user‑land binaries (e.g. desktop.elf). It does not depend on any
// C++ code and uses only the low‑level fb_blit_rect helper for copying
// rectangular pixel blocks from a back‑buffer to the screen framebuffer.
// The API is deliberately simple: initialise a region, add rectangles,
// and flush them.

#include "dirty_region.h"
#include "kstring.h"

static inline int rect_is_valid(int32_t w, int32_t h) {
    return w > 0 && h > 0;
}

void dirty_region_add(DirtyRegion *dr, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!dr || !rect_is_valid(w, h)) return;

    if (dr->count < DIRTY_MAX) {
        dr->rects[dr->count].x = x;
        dr->rects[dr->count].y = y;
        dr->rects[dr->count].w = w;
        dr->rects[dr->count].h = h;
        dr->count++;
    } else {
        // Full‑screen fallback – push a rectangle covering the whole screen.
        // The exact dimensions will be filled in by dirty_region_flush.
        dr->rects[0].x = 0;
        dr->rects[0].y = 0;
        dr->rects[0].w = 0; // sentinel for full‑screen
        dr->rects[0].h = 0;
        dr->count = 1;
    }
}

void dirty_region_flush(DirtyRegion *dr,
                        void *fb, uint32_t stride,
                        const void *backbuf,
                        int32_t screen_w, int32_t screen_h) {
    if (!dr || !fb || !backbuf) return;

    for (int i = 0; i < dr->count; ++i) {
        DirtyRect *r = &dr->rects[i];
        // If the stored rectangle is marked as full‑screen (w == 0 && h == 0),
        // replace it with the real screen dimensions.
        int32_t w = r->w ? r->w : screen_w;
        int32_t h = r->h ? r->h : screen_h;
        int32_t x = r->x;
        int32_t y = r->y;
        // Simple bounds clipping – ensure we never write outside the framebuffer.
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > screen_w) w = screen_w - x;
        if (y + h > screen_h) h = screen_h - y;
        if (w <= 0 || h <= 0) continue;

        // Manual copy row by row from backbuf to framebuffer
        uint8_t *dst_row = (uint8_t *)fb + (uint32_t)y * stride + (uint32_t)x * sizeof(uint32_t);
        const uint8_t *src_row = (const uint8_t *)backbuf + (uint32_t)y * stride + (uint32_t)x * sizeof(uint32_t);
        for (int32_t row = 0; row < h; ++row) {
            // Copy w pixels (each 4 bytes) for this row
            memcpy(dst_row, src_row, (size_t)w * sizeof(uint32_t));
            dst_row += stride;
            src_row += stride;
        }
    }

    // Reset for next frame.
    dr->count = 0;
}
