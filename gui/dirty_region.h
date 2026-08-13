// dirty_region.h – Simple dirty‑region tracking for the GUI
//
// Provides a minimal C API for collecting rectangular dirty regions and
// copying those regions from a back‑buffer to the screen framebuffer.
// The implementation lives in dirty_region.c and uses the low‑level
// fb_blit_rect helper defined in gui/c/fb_draw.c.
//
// This component is deliberately lightweight and does not depend on C++
// or the widget hierarchy; it can be used from any user‑land program that
// draws into a back‑buffer.

#ifndef DIRTY_REGION_H
#define DIRTY_REGION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of dirty rectangles stored per frame.
 * The limit mirrors the MAX_DIRTY constant used by the C++ compositor.
 */
#define DIRTY_MAX 64

/* One dirty rectangle – integer pixel coordinates. */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} DirtyRect;

/* Container that holds a small fixed‑size array of DirtyRect.
 * The count field tracks how many entries are valid.
 */
typedef struct {
    DirtyRect rects[DIRTY_MAX];
    int count;
} DirtyRegion;

/* Initialise a DirtyRegion – set count to zero. */
static inline void dirty_region_init(DirtyRegion *dr) {
    dr->count = 0;
}

/* Add a rectangle to the region.
 * If the region is full, the first entry is replaced with a full‑screen
 * rectangle (0,0,width,height) to force a complete repaint.
 */
void dirty_region_add(DirtyRegion *dr, int32_t x, int32_t y, int32_t w, int32_t h);

/* Flush all recorded dirty rectangles.
 * For each rectangle the function copies the pixels from the back‑buffer
 * to the screen framebuffer using fb_blit_rect.
 * After flushing the region is cleared (count = 0).
 */
void dirty_region_flush(DirtyRegion *dr,
                        void *fb, uint32_t stride,
                        const void *backbuf,
                        int32_t screen_w, int32_t screen_h);

#ifdef __cplusplus
}
#endif

#endif // DIRTY_REGION_H
