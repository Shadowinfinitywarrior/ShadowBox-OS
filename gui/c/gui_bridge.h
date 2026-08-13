#ifndef GUI_BRIDGE_H
#define GUI_BRIDGE_H

#include <stdint.h>

/* Copy a full-screen backbuffer to the screen framebuffer.
 * This mirrors the historic `gui_blit_screen` API used by the C desktop.
 */
void gui_blit_screen(void *dst_fb, const void *src_fb, uint32_t stride, int w, int h);

#endif // GUI_BRIDGE_H
