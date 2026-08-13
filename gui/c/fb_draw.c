/*
 * fb_draw.c  —  Framebuffer drawing primitives
 *
 * All functions operate on a caller-supplied framebuffer:
 *   void *fb      : pointer to the start of pixel data
 *   uint32_t stride : bytes per row (pitch)
 * Pixels are 32-bit ARGB (0xAARRGGBB).
 *
 * No stdlib, no exceptions — freestanding C99.
 */

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* ─── Internal helpers ────────────────────────────────────────────────── */
#define font8x16 fb_font8x16

static inline uint32_t *pixel_at(void *fb, uint32_t stride,
                                  int32_t x, int32_t y)
{
    return (uint32_t *)((uint8_t *)fb + (uint32_t)y * stride
                        + (uint32_t)x * sizeof(uint32_t));
}

/* Porter-Duff src-over alpha blend of src onto dst */
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

/* ─── 8×16 bitmap font (ASCII 0x20–0x7E) ─────────────────────────────── */
/*
 * Each character is 8 pixels wide, 16 pixels tall.
 * Each glyph is stored as 16 bytes; each byte = one row, MSB = left pixel.
 * Minimal embedded font covering printable ASCII.
 */
#define FONT_FIRST 0x20
#define FONT_LAST  0x7E
#define FONT_W     8
#define FONT_H     16

/* Compact glyphs: generated from a standard 8x16 VGA ROM font.
 * Only printable ASCII (0x20–0x7E) is stored here.
 * 95 characters × 16 bytes = 1520 bytes.
 */
static const uint8_t fb_font8x16[95][16] = {
    /* 0x20 ' ' */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    /* 0x21 '!' */ {0,0,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0,0x18,0x18,0,0,0,0},
    /* 0x22 '"' */ {0,0,0x6C,0x6C,0x6C,0,0,0,0,0,0,0,0,0,0,0},
    /* 0x23 '#' */ {0,0,0x36,0x36,0x7F,0x36,0x36,0x7F,0x36,0x36,0,0,0,0,0,0},
    /* 0x24 '$' */ {0,0x08,0x3E,0x6B,0x6B,0x68,0x3E,0x0B,0x6B,0x6B,0x3E,0x08,0,0,0,0},
    /* 0x25 '%' */ {0,0,0x63,0x63,0x06,0x0C,0x18,0x30,0x63,0x63,0,0,0,0,0,0},
    /* 0x26 '&' */ {0,0,0x1C,0x36,0x36,0x1C,0x3B,0x6E,0x66,0x66,0x3B,0,0,0,0,0},
    /* 0x27 '\''*/ {0,0,0x18,0x18,0x18,0,0,0,0,0,0,0,0,0,0,0},
    /* 0x28 '(' */ {0,0,0x0C,0x18,0x18,0x30,0x30,0x30,0x18,0x18,0x0C,0,0,0,0,0},
    /* 0x29 ')' */ {0,0,0x30,0x18,0x18,0x0C,0x0C,0x0C,0x18,0x18,0x30,0,0,0,0,0},
    /* 0x2A '*' */ {0,0,0,0x66,0x3C,0xFF,0x3C,0x66,0,0,0,0,0,0,0,0},
    /* 0x2B '+' */ {0,0,0,0x18,0x18,0x7E,0x18,0x18,0,0,0,0,0,0,0,0},
    /* 0x2C ',' */ {0,0,0,0,0,0,0,0,0,0x18,0x18,0x30,0,0,0,0},
    /* 0x2D '-' */ {0,0,0,0,0,0x7E,0,0,0,0,0,0,0,0,0,0},
    /* 0x2E '.' */ {0,0,0,0,0,0,0,0,0,0,0x18,0x18,0,0,0,0},
    /* 0x2F '/' */ {0,0,0x03,0x06,0x0C,0x18,0x30,0x60,0,0,0,0,0,0,0,0},
    /* 0x30 '0' */ {0,0,0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x31 '1' */ {0,0,0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0,0,0,0,0,0,0},
    /* 0x32 '2' */ {0,0,0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0,0,0,0,0,0,0},
    /* 0x33 '3' */ {0,0,0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x34 '4' */ {0,0,0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0,0,0,0,0,0,0},
    /* 0x35 '5' */ {0,0,0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x36 '6' */ {0,0,0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x37 '7' */ {0,0,0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0,0,0,0,0,0,0},
    /* 0x38 '8' */ {0,0,0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x39 '9' */ {0,0,0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0,0,0,0,0,0,0},
    /* 0x3A ':' */ {0,0,0,0x18,0x18,0,0,0x18,0x18,0,0,0,0,0,0,0},
    /* 0x3B ';' */ {0,0,0,0x18,0x18,0,0,0x18,0x18,0x30,0,0,0,0,0,0},
    /* 0x3C '<' */ {0,0,0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0,0,0,0,0,0,0},
    /* 0x3D '=' */ {0,0,0,0,0x7E,0,0x7E,0,0,0,0,0,0,0,0,0},
    /* 0x3E '>' */ {0,0,0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0,0,0,0,0,0,0},
    /* 0x3F '?' */ {0,0,0x3C,0x66,0x06,0x0C,0x18,0,0x18,0,0,0,0,0,0,0},
    /* 0x40 '@' */ {0,0,0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0,0,0,0,0,0,0},
    /* 0x41 'A' */ {0,0,0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0,0,0,0,0,0,0},
    /* 0x42 'B' */ {0,0,0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0,0,0,0,0,0,0},
    /* 0x43 'C' */ {0,0,0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x44 'D' */ {0,0,0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0,0,0,0,0,0,0},
    /* 0x45 'E' */ {0,0,0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0,0,0,0,0,0,0},
    /* 0x46 'F' */ {0,0,0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0,0,0,0,0,0,0},
    /* 0x47 'G' */ {0,0,0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x48 'H' */ {0,0,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0,0,0,0,0,0,0},
    /* 0x49 'I' */ {0,0,0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0,0,0},
    /* 0x4A 'J' */ {0,0,0x0E,0x06,0x06,0x06,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x4B 'K' */ {0,0,0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0,0,0,0,0,0,0},
    /* 0x4C 'L' */ {0,0,0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0,0,0,0,0,0,0},
    /* 0x4D 'M' */ {0,0,0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0,0,0,0,0,0,0},
    /* 0x4E 'N' */ {0,0,0x63,0x73,0x7B,0x6F,0x67,0x63,0x63,0,0,0,0,0,0,0},
    /* 0x4F 'O' */ {0,0,0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x50 'P' */ {0,0,0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0,0,0,0,0,0,0},
    /* 0x51 'Q' */ {0,0,0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0,0,0,0,0,0,0},
    /* 0x52 'R' */ {0,0,0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0,0,0,0,0,0,0},
    /* 0x53 'S' */ {0,0,0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x54 'T' */ {0,0,0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0,0,0,0,0,0,0},
    /* 0x55 'U' */ {0,0,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x56 'V' */ {0,0,0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0,0,0,0,0,0,0},
    /* 0x57 'W' */ {0,0,0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0,0,0,0,0,0,0},
    /* 0x58 'X' */ {0,0,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0,0,0,0,0,0,0},
    /* 0x59 'Y' */ {0,0,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0,0,0,0,0,0,0},
    /* 0x5A 'Z' */ {0,0,0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0,0,0,0,0,0,0},
    /* 0x5B '[' */ {0,0,0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0,0,0,0,0,0,0},
    /* 0x5C '\\'*/ {0,0,0x60,0x30,0x18,0x0C,0x06,0x03,0,0,0,0,0,0,0,0},
    /* 0x5D ']' */ {0,0,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0,0,0,0,0,0,0},
    /* 0x5E '^' */ {0,0,0x18,0x3C,0x66,0,0,0,0,0,0,0,0,0,0,0},
    /* 0x5F '_' */ {0,0,0,0,0,0,0,0,0,0,0x7E,0,0,0,0,0},
    /* 0x60 '`' */ {0,0,0x18,0x18,0x0C,0,0,0,0,0,0,0,0,0,0,0},
    /* 0x61 'a' */ {0,0,0,0,0x3C,0x06,0x3E,0x66,0x3B,0,0,0,0,0,0,0},
    /* 0x62 'b' */ {0,0,0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0,0,0,0,0,0,0},
    /* 0x63 'c' */ {0,0,0,0,0x3C,0x66,0x60,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x64 'd' */ {0,0,0x06,0x06,0x3E,0x66,0x66,0x66,0x3B,0,0,0,0,0,0,0},
    /* 0x65 'e' */ {0,0,0,0,0x3C,0x66,0x7E,0x60,0x3C,0,0,0,0,0,0,0},
    /* 0x66 'f' */ {0,0,0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0,0,0,0,0,0,0},
    /* 0x67 'g' */ {0,0,0,0,0x3B,0x66,0x66,0x3E,0x06,0x66,0x3C,0,0,0,0,0},
    /* 0x68 'h' */ {0,0,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0,0,0,0,0,0,0},
    /* 0x69 'i' */ {0,0,0x18,0,0x38,0x18,0x18,0x18,0x3C,0,0,0,0,0,0,0},
    /* 0x6A 'j' */ {0,0,0x06,0,0x06,0x06,0x06,0x06,0x66,0x3C,0,0,0,0,0,0},
    /* 0x6B 'k' */ {0,0,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0,0,0,0,0,0,0},
    /* 0x6C 'l' */ {0,0,0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0,0,0},
    /* 0x6D 'm' */ {0,0,0,0,0x66,0x7F,0x7F,0x6B,0x63,0,0,0,0,0,0,0},
    /* 0x6E 'n' */ {0,0,0,0,0x7C,0x66,0x66,0x66,0x66,0,0,0,0,0,0,0},
    /* 0x6F 'o' */ {0,0,0,0,0x3C,0x66,0x66,0x66,0x3C,0,0,0,0,0,0,0},
    /* 0x70 'p' */ {0,0,0,0,0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0,0,0,0,0},
    /* 0x71 'q' */ {0,0,0,0,0x3B,0x66,0x66,0x3E,0x06,0x06,0x06,0,0,0,0,0},
    /* 0x72 'r' */ {0,0,0,0,0x7C,0x66,0x60,0x60,0x60,0,0,0,0,0,0,0},
    /* 0x73 's' */ {0,0,0,0,0x3E,0x60,0x3C,0x06,0x7C,0,0,0,0,0,0,0},
    /* 0x74 't' */ {0,0,0x30,0x30,0x7E,0x30,0x30,0x30,0x1E,0,0,0,0,0,0,0},
    /* 0x75 'u' */ {0,0,0,0,0x66,0x66,0x66,0x66,0x3B,0,0,0,0,0,0,0},
    /* 0x76 'v' */ {0,0,0,0,0x66,0x66,0x66,0x3C,0x18,0,0,0,0,0,0,0},
    /* 0x77 'w' */ {0,0,0,0,0x63,0x6B,0x7F,0x3E,0x36,0,0,0,0,0,0,0},
    /* 0x78 'x' */ {0,0,0,0,0x66,0x3C,0x18,0x3C,0x66,0,0,0,0,0,0,0},
    /* 0x79 'y' */ {0,0,0,0,0x66,0x66,0x3E,0x06,0x3C,0,0,0,0,0,0,0},
    /* 0x7A 'z' */ {0,0,0,0,0x7E,0x0C,0x18,0x30,0x7E,0,0,0,0,0,0,0},
    /* 0x7B '{' */ {0,0,0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0,0,0,0,0,0,0},
    /* 0x7C '|' */ {0,0,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0,0,0,0,0,0,0},
    /* 0x7D '}' */ {0,0,0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0,0,0,0,0,0,0},
    /* 0x7E '~' */ {0,0,0x3B,0x6E,0,0,0,0,0,0,0,0,0,0,0,0},
};

/* ─── Clamp helpers ────────────────────────────────────────────────────── */

static inline int32_t imax(int32_t a, int32_t b) { return a > b ? a : b; }
static inline int32_t imin(int32_t a, int32_t b) { return a < b ? a : b; }

/* ─── Public API ──────────────────────────────────────────────────────── */

/* Fill a solid rectangle. Alpha in color is respected via blending. */
void fb_fill_rect(void *fb, uint32_t stride,
                  int32_t x, int32_t y, int32_t w, int32_t h,
                  uint32_t color)
{
    if (!fb || w <= 0 || h <= 0) return;
    uint8_t a = (color >> 24) & 0xFF;
    for (int32_t row = 0; row < h; ++row) {
        uint32_t *p = pixel_at(fb, stride, x, y + row);
        if (a == 0xFF) {
            for (int32_t col = 0; col < w; ++col)
                p[col] = color;
        } else {
            for (int32_t col = 0; col < w; ++col)
                p[col] = blend(color, p[col]);
        }
    }
}

/* Draw a hollow rectangle (1-pixel border). */
void fb_draw_rect(void *fb, uint32_t stride,
                  int32_t x, int32_t y, int32_t w, int32_t h,
                  uint32_t color)
{
    if (!fb || w <= 0 || h <= 0) return;
    /* Top & bottom rows */
    fb_fill_rect(fb, stride, x, y, w, 1, color);
    fb_fill_rect(fb, stride, x, y + h - 1, w, 1, color);
    /* Left & right columns */
    fb_fill_rect(fb, stride, x, y + 1, 1, h - 2, color);
    fb_fill_rect(fb, stride, x + w - 1, y + 1, 1, h - 2, color);
}

/* ─── Rounded-corner helpers ──────────────────────────────────────────── */
/*
 * For a corner of radius r, we draw the filled arc by iterating over rows
 * and computing the chord half-width via integer square root.
 * Each call handles one of four corners.
 */

static inline uint32_t isqrt32(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t y = (x + 1) >> 1;
    while (y < x) { x = y; y = (x + n / x) >> 1; }
    return x;
}

/* Fill a rounded rectangle. r = corner radius (pixels). */
void fb_fill_rect_round(void *fb, uint32_t stride,
                         int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t color, int32_t r)
{
    if (!fb || w <= 0 || h <= 0) return;
    if (r <= 0 || r * 2 > w || r * 2 > h) {
        fb_fill_rect(fb, stride, x, y, w, h, color);
        return;
    }

    for (int32_t row = 0; row < h; ++row) {
        int32_t ry = row;
        int32_t x0 = x, rw = w;

        /* Clip corners */
        if (ry < r) {
            /* top corners */
            int32_t dy = r - 1 - ry;
            int32_t dx = (int32_t)isqrt32((uint32_t)(r * r - dy * dy));
            x0 = x + r - dx;
            rw = w - 2 * (r - dx);
        } else if (ry >= h - r) {
            /* bottom corners */
            int32_t dy = r - (h - ry);
            int32_t dx = (int32_t)isqrt32((uint32_t)(r * r - dy * dy));
            x0 = x + r - dx;
            rw = w - 2 * (r - dx);
        }

        if (rw <= 0) continue;
        uint32_t *p = pixel_at(fb, stride, x0, y + row);
        uint8_t a = (color >> 24) & 0xFF;
        if (a == 0xFF) {
            for (int32_t col = 0; col < rw; ++col)
                p[col] = color;
        } else {
            for (int32_t col = 0; col < rw; ++col)
                p[col] = blend(color, p[col]);
        }
    }
}

/* Draw a hollow rounded rectangle (1-pixel border). */
void fb_draw_rect_round(void *fb, uint32_t stride,
                         int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t color, int32_t r)
{
    if (!fb || w <= 0 || h <= 0) return;
    if (r <= 0 || r * 2 > w || r * 2 > h) {
        fb_draw_rect(fb, stride, x, y, w, h, color);
        return;
    }

    /* Draw horizontal spans at top and bottom (only the straight part) */
    fb_fill_rect(fb, stride, x + r, y, w - 2 * r, 1, color);
    fb_fill_rect(fb, stride, x + r, y + h - 1, w - 2 * r, 1, color);
    /* Left / right straight verticals */
    fb_fill_rect(fb, stride, x, y + r, 1, h - 2 * r, color);
    fb_fill_rect(fb, stride, x + w - 1, y + r, 1, h - 2 * r, color);

    /* Draw arcs at four corners (single pixel per row) */
    for (int32_t i = 0; i < r; ++i) {
        int32_t dy  = r - 1 - i;
        int32_t dx  = (int32_t)isqrt32((uint32_t)(r * r - dy * dy));
        int32_t ax  = r - dx;           /* offset from rect edge */

        /* top-left */
        *pixel_at(fb, stride, x + ax,         y + i)         = color;
        /* top-right */
        *pixel_at(fb, stride, x + w - 1 - ax, y + i)         = color;
        /* bottom-left */
        *pixel_at(fb, stride, x + ax,         y + h - 1 - i) = color;
        /* bottom-right */
        *pixel_at(fb, stride, x + w - 1 - ax, y + h - 1 - i) = color;
    }
}

/* ─── Text rendering ──────────────────────────────────────────────────── */

/*
 * Draw a NUL-terminated ASCII string.
 * bg_color = 0x00000000 means transparent background (no bg fill).
 */
void fb_draw_text(void *fb, uint32_t stride,
                  int32_t x, int32_t y,
                  const char *s, uint32_t fg, uint32_t bg)
{
    if (!fb || !s) return;
    int32_t cx = x;
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c < FONT_FIRST || c > FONT_LAST) {
            cx += FONT_W;
            continue;
        }
        const uint8_t *glyph = fb_font8x16[c - FONT_FIRST];
        for (int row = 0; row < FONT_H; ++row) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < FONT_W; ++col) {
                uint32_t *p = pixel_at(fb, stride, cx + col, y + row);
                if (bits & (0x80 >> col)) {
                    *p = blend(fg, *p);
                } else if ((bg >> 24) != 0) {
                    *p = blend(bg, *p);
                }
            }
        }
        cx += FONT_W;
    }
}

/*
 * Draw text with word-wrap inside a rectangle (x, y, w, h).
 */
void fb_draw_text_wrap(void *fb, uint32_t stride,
                        int32_t x, int32_t y, int32_t w, int32_t h,
                        const char *s, uint32_t fg, uint32_t bg)
{
    if (!fb || !s || w <= 0 || h <= 0) return;

    int32_t cols = w / FONT_W;
    int32_t rows = h / FONT_H;
    int32_t cur_col = 0, cur_row = 0;

    for (; *s && cur_row < rows; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == '\n') {
            cur_col = 0;
            cur_row++;
            continue;
        }
        if (cur_col >= cols) {
            cur_col = 0;
            cur_row++;
            if (cur_row >= rows) break;
        }
        /* Draw single character */
        if (c >= FONT_FIRST && c <= FONT_LAST) {
            const uint8_t *glyph = fb_font8x16[c - FONT_FIRST];
            int32_t px = x + cur_col * FONT_W;
            int32_t py = y + cur_row * FONT_H;
            for (int row = 0; row < FONT_H; ++row) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < FONT_W; ++col) {
                    uint32_t *p = pixel_at(fb, stride, px + col, py + row);
                    if (bits & (0x80 >> col)) {
                        *p = blend(fg, *p);
                    } else if ((bg >> 24) != 0) {
                        *p = blend(bg, *p);
                    }
                }
            }
        }
        cur_col++;
    }
}

/*
 * Return the number of characters (≠ pixel width) in a NUL-terminated string.
 * Callers multiply by FONT_W (8) to get pixel width.
 */
int fb_text_width(const char *s)
{
    if (!s) return 0;
    int n = 0;
    while (*s++) n++;
    return n;
}

/* ─── Blit ────────────────────────────────────────────────────────────── */

/*
 * Copy a w×h region from src (at src_stride bytes/row) to dst starting at
 * (dx, dy). No blending — raw pixel copy.
 */
void fb_blit_rect(void *dst, uint32_t dst_stride,
                  const void *src, uint32_t src_stride,
                  int32_t dx, int32_t dy, int32_t w, int32_t h)
{
    if (!dst || !src || w <= 0 || h <= 0) return;
    for (int32_t row = 0; row < h; ++row) {
        const uint32_t *s = (const uint32_t *)((const uint8_t *)src
                            + (uint32_t)row * src_stride);
        uint32_t       *d = (uint32_t *)((uint8_t *)dst
                            + (uint32_t)(dy + row) * dst_stride
                            + (uint32_t)dx * sizeof(uint32_t));
        for (int32_t col = 0; col < w; ++col)
            d[col] = s[col];
    }
}
