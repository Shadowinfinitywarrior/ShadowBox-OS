// kernel/stubs.c — internal dummy implementations so the kernel links
// when optional subsystems are not yet complete.
// Keep every symbol static / internal to avoid polluting the ABI.

#include "kernel.h"

// ---------------------------------------------------------------------------
// Calendar / time
// ---------------------------------------------------------------------------
#include "time.h"

int clock_gettime(int clk_id, struct timespec *tp) {
 (void)clk_id;
 if (tp) {
  tp->tv_sec = 0;
  tp->tv_nsec = 0;
 }
 return 0;
}

// ---------------------------------------------------------------------------
// Block layer
// ---------------------------------------------------------------------------
int block_flush(void) { return 0; }

// ---------------------------------------------------------------------------
// Framebuffer / GUI helpers used by userland GUI code that ended up linked
// into the kernel image.
// ---------------------------------------------------------------------------
int sys_fb_flip(void) { return 0; }

void draw_pixel(int x, int y, int color) { (void)x; (void)y; (void)color; }
void draw_hline(int x, int y, int w, int color) { (void)x; (void)y; (void)w; (void)color; }
void draw_vline(int x, int y, int h, int color) { (void)x; (void)y; (void)h; (void)color; }
void draw_line(int x0, int y0, int x1, int y1, int color)
{(void)x0;(void)y0;(void)x1;(void)y1;(void)color;}
void clear_screen(void) {}

// Font used by taskbar.c
const uint8_t font8x8_basic[128][8] = {{0}};

// Shared UI state / backbuffer
uint8_t *backbuffer = NULL;
int menu_open = 0;
int mouse_btn_down = 0;
int num_windows = 0;
void *windows[32] = {0};
int mouse_x = 0;
int mouse_y = 0;
void *settings_daemon = NULL;
