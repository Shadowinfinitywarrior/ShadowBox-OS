/*
 * Minimal stub implementations of framebuffer accessor functions for userland
 * programs. This avoids linking the full kernel framebuffer driver, which depends
 * on many kernel symbols. The desktop code uses a fixed framebuffer address
 * (0x78000000) and dimensions 1024x768 with a 32‑bit (4‑byte) pixel stride.
 */

#include "fb.h"

/* Fixed framebuffer configuration matching the desktop's assumptions */
static uint8_t *user_fb_addr = (uint8_t *)0x78000000ULL;
static const uint32_t user_fb_width = 1024;
static const uint32_t user_fb_height = 768;
static const uint32_t user_fb_pitch = 1024 * 4; // bytes per row (32‑bit pixels)
static const uint8_t user_fb_bpp = 32;

void fb_set_info(uint64_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp) {
    // No-op stub – userland does not need dynamic configuration.
    (void)addr; (void)width; (void)height; (void)pitch; (void)bpp;
}

void fb_init(void) {
    // No-op stub – framebuffer is assumed ready.
}

void fb_get_info(uint32_t *width, uint32_t *height, uint32_t *pitch, uint8_t *bpp) {
    if (width) *width = user_fb_width;
    if (height) *height = user_fb_height;
    if (pitch) *pitch = user_fb_pitch;
    if (bpp) *bpp = user_fb_bpp;
}

uint8_t *fb_get_addr(void) { return user_fb_addr; }
uint32_t fb_get_width(void) { return user_fb_width; }
uint32_t fb_get_height(void) { return user_fb_height; }
uint32_t fb_get_pitch(void) { return user_fb_pitch; }
uint8_t fb_get_bpp(void) { return user_fb_bpp; }

void fb_console_init(void) {
    // No-op stub: console functionality not required in userland.
}

void fb_console_putchar(char c) {
    // No-op stub: ignore console output.
    (void)c;
}

/* Minimal pixel write used by assembly draw primitives */
void fb_putpixel(int x, int y, uint32_t color) {
    if (x < 0 || y < 0) return;
    if ((uint32_t)x >= user_fb_width || (uint32_t)y >= user_fb_height) return;
    uint32_t *pixel = (uint32_t *)(user_fb_addr + y * user_fb_pitch + x * 4);
    *pixel = color;
}
