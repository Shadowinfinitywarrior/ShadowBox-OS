#ifndef FB_H
#define FB_H

#include "types.h"

void fb_init(void);
void fb_get_info(uint32_t *width, uint32_t *height, uint32_t *pitch, uint8_t *bpp);
uint8_t *fb_get_addr(void);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);
uint32_t fb_get_pitch(void);
uint8_t fb_get_bpp(void);

#endif