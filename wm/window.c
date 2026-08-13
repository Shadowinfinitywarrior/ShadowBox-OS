/* Minimal stub for window management in the kernel.
 * This file provides placeholder definitions to satisfy compilation
 * when the full implementation is not yet required.
 */

#include <stddef.h>

/* Forward declaration of a window structure. */
typedef struct window {
    int id;
    void *data;
} window_t;

/* Create a new window.
 * Returns a pointer to a newly allocated window stub, or NULL on failure.
 */
static inline window_t *window_create(int id) {
    (void)id; /* suppress unused parameter warning */
    return NULL;
}

/* Destroy a window. */
static inline void window_destroy(window_t *w) {
    (void)w;
}

/* Draw the window. */
static inline void window_draw(const window_t *w) {
    (void)w;
}
