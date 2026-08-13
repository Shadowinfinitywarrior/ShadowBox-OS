#ifndef SHADOWBOX_KEYBOARD_LAYOUTS_H
#define SHADOWBOX_KEYBOARD_LAYOUTS_H

#include "hid_kbd.h"

// Layout IDs
enum {
    KBD_LAYOUT_US_QWERTY = 0,
    KBD_LAYOUT_DVORAK,
    KBD_LAYOUT_AZERTY,
    KBD_LAYOUT_COUNT
};

// Initialise the layout subsystem (defaults to US QWERTY)
void keyboard_layout_init(void);

// Set active layout by ID
void keyboard_layout_set(int layout_id);

// Get current layout ID
int keyboard_layout_get_current(void);

// Retrieve keysym for a scancode in normal (unshifted) mode
uint32_t keyboard_layout_get_normal(uint8_t base_code);

// Retrieve keysym for a scancode in shifted mode
uint32_t keyboard_layout_get_shift(uint8_t base_code);

#endif // SHADOWBOX_KEYBOARD_LAYOUTS_H