// Keyboard Shortcuts Component Header
#ifndef SHADOWBOX_KEYBOARD_SHORTCUTS_H
#define SHADOWBOX_KEYBOARD_SHORTCUTS_H

#include "hid_kbd.h"

// Initialize the keyboard shortcuts subsystem and register default shortcuts.
void keyboard_shortcuts_init(void);

// Register a keyboard shortcut.
// keysym: logical key symbol (e.g., 't')
// modifiers: bitmask of KBD_MOD_* flags required (e.g., KBD_MOD_CTRL|KBD_MOD_ALT)
// action: function to call when shortcut is triggered.
int keyboard_shortcuts_register(uint32_t keysym, uint32_t modifiers, void (*action)(void));

// Process a key event; call registered actions if matching shortcut.
void keyboard_shortcuts_process_event(const key_event_t *ev);

#endif // SHADOWBOX_KEYBOARD_SHORTCUTS_H
