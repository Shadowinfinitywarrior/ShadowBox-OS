/* Minimal stub for font handling in UI subsystem */

#include <stddef.h>

/* Font descriptor */
typedef struct {
    const char *name;
    int size;
} font_t;

/* Load a font from a file - stub returns success */
int font_load(const char *path, font_t *out) {
    (void)path;
    if (!out) return -1;
    out->name = "stub";
    out->size = 0;
    return 0;
}

/* Render text using a font - stub does nothing */
int font_render(const font_t *font, const char *text, void *framebuffer,
                int width, int height) {
    (void)font; (void)text; (void)framebuffer; (void)width; (void)height;
    return 0;
}
