#include "draw.h"


/* Simple wrappers that use the low‑level assembly primitives. */

void gui_clear_screen(void) {
    clear_screen();
}

void gui_draw_pixel(int x, int y, uint32_t color) {
    draw_pixel(x, y, color);
}

void gui_draw_rect(int x, int y, int w, int h, uint32_t color) {
    // Draw four edges using draw_hline and draw_vline (which are stubs for now)
    draw_hline(x, y, w, color);               // top
    draw_hline(x, y + h - 1, w, color);       // bottom
    draw_vline(x, y, h, color);               // left
    draw_vline(x + w - 1, y, h, color);       // right
}

void gui_draw_filled_rect(int x, int y, int w, int h, uint32_t color) {
    for (int iy = 0; iy < h; ++iy) {
        draw_hline(x, y + iy, w, color);
    }
}

void gui_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    draw_line(x0, y0, x1, y1, color);
}
