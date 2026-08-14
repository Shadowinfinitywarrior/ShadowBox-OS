/* Font handling with grayscale anti‑aliasing */

#include <stddef.h>
#include <stdint.h>
#include "font.h"

/* Font descriptor */
/* font_t defined in include/font.h */

/* Load a font – for now we load the built‑in bitmap font only */
int font_load(const char *path, font_t *out) {
    if (!out) return -1;
    /* The bitmap font is built‑in; the path argument is ignored */
    out->name = path ? path : "builtin";
    out->size = 0; /* size not used in bitmap rendering */
    return 0;
}

/* Internal helpers – copied from fb_draw.c */
#define FONT_W 8
#define FONT_H 16

static inline uint32_t *pixel_at(void *fb, uint32_t stride,
                                  int32_t x, int32_t y)
{
    return (uint32_t *)((uint8_t *)fb + (uint32_t)y * stride
                        + (uint32_t)x * sizeof(uint32_t));
}

/* Porter‑Duff src‑over alpha blend */
static inline uint32_t blend(uint32_t src, uint32_t dst)
{
    uint32_t a  = (src >> 24) & 0xFF;
    if (a == 0xFF) return src;
    if (a == 0x00) return dst;
    uint32_t ia = 255 - a;
    uint32_t r  = ((src >> 16 & 0xFF) * a + (dst >> 16 & 0xFF) * ia) / 255;
    uint32_t g  = ((src >>  8 & 0xFF) * a + (dst >>  8 & 0xFF) * ia) / 255;
    uint32_t b  = ((src       & 0xFF) * a + (dst       & 0xFF) * ia) / 255;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

/* Render text with simple grayscale anti‑aliasing.
 *  - framebuffer is a 32‑bit ARGB buffer of size width×height.
 *  - stride is width * sizeof(uint32_t) (no padding).
 *  - foreground colour is opaque white (0xFFFFFFFF).
 *  - background is left transparent.
 */
int font_render(const font_t *font, const char *text, void *framebuffer,
                int width, int height)
{
    if (!font || !text || !framebuffer) return -1;
    if (width <= 0 || height <= 0) return -1;

    uint32_t stride = (uint32_t)width * sizeof(uint32_t);
    const uint32_t fg = 0xFFFFFFFFu; /* opaque white */
    int cx = 0; /* current x position in pixels */

    for (; *text; ++text) {
        unsigned char c = (unsigned char)*text;
        if (c >= 256) continue; /* out of range */
        const uint8_t *glyph = &font8x16[c * FONT_H];
        for (int row = 0; row < FONT_H; ++row) {
            if (row >= height) break;
            for (int col = 0; col < FONT_W; ++col) {
                if (cx + col >= width) break;
                /* Compute coverage using a 3×3 neighbourhood of source bits */
                int sum = 0;
                for (int dr = -1; dr <= 1; ++dr) {
                    int r = row + dr;
                    if (r < 0 || r >= FONT_H) continue;
                    uint8_t bits = glyph[r];
                    for (int dc = -1; dc <= 1; ++dc) {
                        int cc = col + dc;
                        if (cc < 0 || cc >= FONT_W) continue;
                        if (bits & (0x80 >> cc)) sum++;
                    }
                }
                if (sum == 0) continue; /* fully transparent */
                uint8_t alpha = (uint8_t)((sum * 255 + 4) / 9); /* round */
                uint32_t src = ((uint32_t)alpha << 24) | (fg & 0x00FFFFFF);
                uint32_t *p = pixel_at(framebuffer, stride, cx + col, row);
                *p = blend(src, *p);
            }
        }
        cx += FONT_W;
        if (cx + FONT_W > width) break; /* no more horizontal space */
    }
    return 0;
}

