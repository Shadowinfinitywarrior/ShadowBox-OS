#include "keyboard_layouts.h"
#include "hid_kbd.h"

// US QWERTY layout tables (based on existing keymap_set1_* definitions)
static const uint32_t us_keymap_normal[128] = {
    0, KS_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KS_BACKSPACE,
    KS_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KS_ENTER,
    KS_LCTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', KS_LSHIFT,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KS_RSHIFT,
    '*', KS_LALT, ' ', KS_CAPS, KS_F1
    // Remaining entries omitted for brevity – they default to 0.
};

static const uint32_t us_keymap_shift[128] = {
    0, KS_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', KS_BACKSPACE,
    KS_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', KS_ENTER,
    KS_LCTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', KS_LSHIFT,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', KS_RSHIFT,
    '*', KS_LALT, ' ', KS_CAPS, KS_F1
    // Remaining entries omitted for brevity.
};

// Placeholder layouts for DVORAK and AZERTY – currently map to US QWERTY.

typedef struct {
    const uint32_t *normal;
    const uint32_t *shift;
    const char *name;
} keyboard_layout_t;

static const keyboard_layout_t layout_table[] = {
    { us_keymap_normal, us_keymap_shift, "US QWERTY" },
    { (const uint32_t *)us_keymap_normal, (const uint32_t *)us_keymap_shift, "DVORAK (placeholder)" },
    { (const uint32_t *)us_keymap_normal, (const uint32_t *)us_keymap_shift, "AZERTY (placeholder)" }
};

static const keyboard_layout_t *current_layout = &layout_table[0];
static int current_layout_id = KBD_LAYOUT_US_QWERTY;

void keyboard_layout_init(void) {
    current_layout = &layout_table[KBD_LAYOUT_US_QWERTY];
    current_layout_id = KBD_LAYOUT_US_QWERTY;
}

void keyboard_layout_set(int layout_id) {
    if (layout_id < 0 || layout_id >= (int)(sizeof(layout_table)/sizeof(layout_table[0]))) {
        return; // Invalid ID – ignore.
    }
    current_layout = &layout_table[layout_id];
    current_layout_id = layout_id;
}

int keyboard_layout_get_current(void) {
    return current_layout_id;
}

uint32_t keyboard_layout_get_normal(uint8_t base_code) {
    if (base_code >= 128) return KS_UNKNOWN;
    return current_layout->normal[base_code];
}

uint32_t keyboard_layout_get_shift(uint8_t base_code) {
    if (base_code >= 128) return KS_UNKNOWN;
    return current_layout->shift[base_code];
}
