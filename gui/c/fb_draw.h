#ifndef FB_DRAW_H
#define FB_DRAW_H

#include <stdint.h>

/* Framebuffer drawing primitives */
void fb_fill_rect(void *fb, uint32_t stride,
                  int32_t x, int32_t y, int32_t w, int32_t h,
                  uint32_t color);
void fb_draw_rect(void *fb, uint32_t stride,
                  int32_t x, int32_t y, int32_t w, int32_t h,
                  uint32_t color);
void fb_fill_rect_round(void *fb, uint32_t stride,
                        int32_t x, int32_t y, int32_t w, int32_t h,
                        uint32_t color, int32_t r);
void fb_draw_rect_round(void *fb, uint32_t stride,
                        int32_t x, int32_t y, int32_t w, int32_t h,
                        uint32_t color, int32_t r);
void fb_draw_text(void *fb, uint32_t stride,
                  int32_t x, int32_t y,
                  const char *s, uint32_t fg, uint32_t bg);
void fb_draw_text_wrap(void *fb, uint32_t stride,
                        int32_t x, int32_t y, int32_t w, int32_t h,
                        const char *s, uint32_t fg, uint32_t bg);
int fb_text_width(const char *s);
void fb_blit_rect(void *dst, uint32_t dst_stride,
                  const void *src, uint32_t src_stride,
                  int32_t dx, int32_t dy, int32_t w, int32_t h);

#endif // FB_DRAW_H
