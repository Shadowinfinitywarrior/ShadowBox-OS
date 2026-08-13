#include "fb_double.h"
#include "fb.h"
#include "malloc.h"
#include <string.h>

static uint8_t *backbuf = NULL;
static uint8_t *fb_addr = NULL;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;

/* Initialise a back‑buffer of the same size as the framebuffer.
 * This function should be called after the framebuffer is mapped
 * (e.g. after fb_console_init()).
 */
void fb_double_init(void) {
    /* Retrieve current framebuffer parameters. */
    fb_width = fb_get_width();
    fb_height = fb_get_height();
    fb_pitch = fb_get_pitch();
    fb_addr = fb_get_addr();

    if (!fb_addr || fb_width == 0 || fb_height == 0 || fb_pitch == 0) {
        // Framebuffer not ready – nothing to do.
        return;
    }

    /* Allocate a back‑buffer large enough for the whole screen. */
    uint64_t size = (uint64_t)fb_height * (uint64_t)fb_pitch; // bytes
    backbuf = (uint8_t *)kmalloc(size);
    if (!backbuf) {
        // Allocation failed; keep backbuf NULL.
        return;
    }

    // Clear the back‑buffer.
    memset(backbuf, 0, (size_t)size);
}

/* Copy the back‑buffer to the visible framebuffer.
 * This performs a row‑by‑row memcpy respecting the pitch.
 */
void fb_double_swap(void) {
    if (!backbuf || !fb_addr) return;

    for (uint32_t y = 0; y < fb_height; ++y) {
        uint8_t *dst = fb_addr + (uint64_t)y * fb_pitch;
        const uint8_t *src = backbuf + (uint64_t)y * fb_pitch;
        memcpy(dst, src, fb_pitch);
    }
}

/* Free the allocated back‑buffer. */
void fb_double_free(void) {
    if (backbuf) {
        kfree(backbuf);
        backbuf = NULL;
    }
}
