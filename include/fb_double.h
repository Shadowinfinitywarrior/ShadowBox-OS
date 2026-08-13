#ifndef FB_DOUBLE_H
#define FB_DOUBLE_H

#include "types.h"

/* Initialize a double‑buffer for the current framebuffer.
 * This allocates a back‑buffer of the same size as the framebuffer
 * and clears it. It must be called after fb_init() and fb_console_init()
 * have set up the framebuffer mapping.
 */
void fb_double_init(void);

/* Copy the back‑buffer contents to the visible framebuffer
 * (swap the buffers). Call this after drawing into the back‑buffer.
 */
void fb_double_swap(void);

/* Release the back‑buffer when it is no longer needed. */
void fb_double_free(void);

#endif // FB_DOUBLE_H
