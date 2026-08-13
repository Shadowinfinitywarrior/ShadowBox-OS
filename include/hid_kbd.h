#ifndef SHADOWBOX_HID_KBD_H
#define SHADOWBOX_HID_KBD_H

#include "types.h"
#include "hid.h"

// Modifier bitmask
#define KBD_MOD_SHIFT     (1 << 0)
#define KBD_MOD_CTRL      (1 << 1)
#define KBD_MOD_ALT       (1 << 2)
#define KBD_MOD_META      (1 << 3)
#define KBD_MOD_ALTGR     (1 << 4)
#define KBD_MOD_CAPS      (1 << 5)
#define KBD_MOD_NUM       (1 << 6)
#define KBD_MOD_SCROLL    (1 << 7)

// Common Keysyms
#define KS_UNKNOWN  0
#define KS_ESC      0x1B
#define KS_ENTER    '\n'
#define KS_TAB      '\t'
#define KS_BACKSPACE '\b'

#define KS_F1       0x1001
#define KS_UP       0x1011
#define KS_DOWN     0x1012
#define KS_LEFT     0x1013
#define KS_RIGHT    0x1014
#define KS_LSHIFT   0x1021
#define KS_RSHIFT   0x1022
#define KS_LCTRL    0x1023
#define KS_LALT     0x1024
#define KS_CAPS     0x1025

// Key event pipeline struct
typedef struct key_event {
    uint32_t  keycode;    // physical key (scancode or standardized HID code)
    uint32_t  keysym;     // logical symbol
    uint32_t  modifiers;  // modifier bitmask
    uint32_t  unicode;    // resulting Unicode codepoint
    uint8_t   pressed;    // down (1) or up (0)
    uint8_t   repeat;     // auto-repeat flag
    uint64_t  timestamp;  // nanoseconds
} key_event_t;

void hid_kbd_init(void);
void hid_kbd_process_report(hid_device_t *hdev, uint8_t *report, size_t len);
void kbd_dispatch_event(key_event_t *ev);

#endif
