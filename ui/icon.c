#include "icon.h"

/* Simple blend helper – same logic as in desktop.c */
static inline uint32_t blend_color(uint32_t bg, uint32_t fg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    uint32_t rb = bg & 0xFF00FF;
    uint32_t g  = bg & 0x00FF00;
    uint32_t rf = fg & 0xFF00FF;
    uint32_t gf = fg & 0x00FF00;
    rb += ((rf - rb) * alpha) >> 8;
    g  += ((gf - g ) * alpha) >> 8;
    return (rb & 0xFF00FF) | (g & 0x00FF00);
}

void icon_blit(const uint32_t *data, int size, int x, int y, uint32_t *fb, int stride) {
    for (int row = 0; row < size; ++row) {
        int dst_y = y + row;
        if (dst_y < 0) continue;
        for (int col = 0; col < size; ++col) {
            int dst_x = x + col;
            if (dst_x < 0) continue;
            uint32_t src = data[row * size + col];
            uint8_t alpha = src >> 24;
            if (alpha == 0) continue; // fully transparent
            uint32_t *pixel = &fb[dst_y * stride + dst_x];
            if (alpha == 255) {
                *pixel = src & 0x00FFFFFF; // ignore alpha channel
            } else {
                *pixel = blend_color(*pixel, src, alpha);
            }
        }
    }
}

/* Placeholder icons – solid colours, fully opaque */
#define ICON_FILL(name, color) \
    uint32_t name##_32[32*32] = { [0 ... (32*32-1)] = (0xFF000000u | (color)) };

#define ICON_FILL16(name, color) \
    uint32_t name##_16[16*16] = { [0 ... (16*16-1)] = (0xFF000000u | (color)) };

/* Desktop icons (32×32) */
ICON_FILL(icon_terminal, 0x3498DB)   /* teal */
ICON_FILL(icon_files,    0xF39C12)   /* orange */
ICON_FILL(icon_sysmon,   0x2ECC71)   /* green */
ICON_FILL(icon_calc,    0xE67E22)    /* orange */
ICON_FILL(icon_editor,   0x9B59B6)   /* purple */
ICON_FILL(icon_paint,    0xE74C3C)   /* red */

/* Window control button icons (16×16) */
ICON_FILL16(icon_close, 0xFF0000)   /* red */
ICON_FILL16(icon_max,   0x00FF00)   /* green */
ICON_FILL16(icon_min,   0xFFFF00)   /* yellow */
