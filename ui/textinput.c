#include "gui_toolkit.h"
#include "font.h"
#include <stddef.h>
#include <stdint.h>

/* Minimal stub for a text input widget used by the kernel UI system.
   This implementation provides only the symbols required for compilation;
   it contains no functional behaviour. */

typedef struct {
    const char *placeholder;   /* Text shown when buffer is empty */
    char *buffer;               /* Mutable input buffer */
    size_t capacity;            /* Size of buffer in bytes */
    size_t length;              /* Current length of content */
} textinput_t;

/* Initialise a textinput instance with an external buffer.
   The caller must ensure `buf` points to a writable region of at least
   `cap` bytes. */
static inline void textinput_init(textinput_t *ti, char *buf, size_t cap) {
    if (!ti) return;
    ti->placeholder = NULL;
    ti->buffer = buf;
    ti->capacity = cap;
    ti->length = 0;
}

/* Set the placeholder string – a shallow pointer copy. */
static inline void textinput_set_placeholder(textinput_t *ti, const char *ph) {
    if (!ti) return;
    ti->placeholder = ph;
}

/* Retrieve the current text; returns a pointer to the internal buffer. */
static inline const char *textinput_get_text(const textinput_t *ti) {
    return (ti && ti->buffer) ? ti->buffer : NULL;
}

/* Stub draw routine – does nothing in this minimal build. */
static inline void textinput_draw(const textinput_t *ti) {
    (void)ti; /* suppress unused‑parameter warning */
}

/* Stub event handler – returns 0 to indicate the key was not processed. */
static inline int textinput_handle_key(textinput_t *ti, uint32_t keycode) {
    (void)ti; (void)keycode;
    return 0;
}
