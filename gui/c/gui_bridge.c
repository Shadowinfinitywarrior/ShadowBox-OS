#include "fb_draw.h"

/*
 * Simple wrapper to copy the backbuffer to the screen framebuffer.
 * Parameters match the original `gui_blit_screen` expected by desktop_interactive.cpp.
 *   dst_fb: destination framebuffer pointer (usually the primary screen fb)
 *   src_fb: source framebuffer pointer (backbuffer)
 *   stride: bytes per row (same for both buffers)
 *   w: width in pixels
 *   h: height in pixels
 */
void gui_blit_screen(void *dst_fb, const void *src_fb, uint32_t stride, int w, int h) {
    // Use the FB blit helper to copy the full rectangle at (0,0).
    fb_blit_rect(dst_fb, stride, src_fb, stride, 0, 0, w, h);
}

#include "../../userland/sys.h"
void gui_fb_flip(uint32_t y_offset) {
    sys_fb_flip(y_offset);
}
