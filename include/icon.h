#ifndef ICON_H
#define ICON_H

#include <stdint.h>

// Blit a square ARGB icon onto a framebuffer.
// `size` is both width and height (pixels).
// `fb` points to framebuffer pixel data (32‑bit ARGB, no alpha stored).
// `stride` is the number of pixels per row in the framebuffer.
void icon_blit(const uint32_t *data, int size, int x, int y, uint32_t *fb, int stride);

// Icon data arrays – extern declarations
extern uint32_t icon_terminal_32[32*32];
extern uint32_t icon_files_32[32*32];
extern uint32_t icon_sysmon_32[32*32];
extern uint32_t icon_calc_32[32*32];
extern uint32_t icon_editor_32[32*32];
extern uint32_t icon_paint_32[32*32];
extern uint32_t icon_close_16[16*16];
extern uint32_t icon_max_16[16*16];
extern uint32_t icon_min_16[16*16];

#endif // ICON_H
