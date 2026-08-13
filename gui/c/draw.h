#ifndef GUI_DRAW_H
#define GUI_DRAW_H

#include <stdint.h>

/* Assembly primitives (implemented in gui/asm/draw.S) */
extern void draw_pixel(int x, int y, uint32_t color);
extern void clear_screen(void);
extern void draw_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
extern void draw_hline(int x, int y, int length, uint32_t color);
extern void draw_vline(int x, int y, int length, uint32_t color);

/* Higher‑level helper functions (implemented in draw.c) */
void gui_clear_screen(void);
void gui_draw_pixel(int x, int y, uint32_t color);
void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_filled_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_line(int x0, int y0, int x1, int y1, uint32_t color);

#endif // GUI_DRAW_H
