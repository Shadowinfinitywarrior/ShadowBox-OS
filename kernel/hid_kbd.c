#include "hid_kbd.h"
#include "kernel.h"
#include "input.h"
#include "keyboard_shortcuts.h"
#include "time.h"
#include "keyboard_layouts.h"

static uint32_t current_modifiers = 0;
static uint32_t current_dead_key = 0;
static int scancode_e0_prefix = 0;
static int repeat_active = 0;
static uint32_t repeat_keycode = 0;
static uint32_t repeat_unicode = 0;
static uint64_t repeat_press_time = 0;
static uint64_t repeat_last_time = 0;
static uint16_t repeat_rate_ms = 30;
static uint16_t repeat_delay_ms = 500;

// Minimal Scancode Set 1 Keymap (US QWERTY)
static const uint32_t keymap_set1_normal[128] = {
    0, KS_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KS_BACKSPACE,
    KS_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KS_ENTER,
    KS_LCTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', KS_LSHIFT,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KS_RSHIFT,
    '*', KS_LALT, ' ', KS_CAPS, KS_F1
    // Remaining entries omitted for brevity
};

static const uint32_t keymap_set1_shift[128] = {
    0, KS_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', KS_BACKSPACE,
    KS_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', KS_ENTER,
    KS_LCTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', KS_LSHIFT,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', KS_RSHIFT,
    '*', KS_LALT, ' ', KS_CAPS, KS_F1
    // Remaining entries omitted for brevity
};

void hid_kbd_init(void) {
    printk(KERN_INFO "HID-KBD: Initialized Deep Keyboard Subsystem (Scancodes, Layouts, Events)\n");
    current_modifiers = 0;
    current_dead_key = 0;
    scancode_e0_prefix = 0;
    keyboard_shortcuts_init();
	keyboard_layout_init();
}

static uint32_t translate_scancode_to_keysym(uint8_t base_code, int is_e0, uint32_t mods) {
    if (base_code >= 128) return KS_UNKNOWN;
    if (is_e0) {
        // E0 prefix handling (e.g. arrows)
        if (base_code == 0x48) return KS_UP;
        if (base_code == 0x4B) return KS_LEFT;
        if (base_code == 0x4D) return KS_RIGHT;
        if (base_code == 0x50) return KS_DOWN;
        return KS_UNKNOWN;
    }
    // CapsLock handling for letters
    int shift_active = (mods & KBD_MOD_SHIFT) ? 1 : 0;
    int caps_active = (mods & KBD_MOD_CAPS) ? 1 : 0;
    uint32_t normal = keyboard_layout_get_normal(base_code);
    int is_letter = (normal >= 'a' && normal <= 'z');
    if (is_letter && caps_active) {
        shift_active = !shift_active; // Invert shift for letters
    }
    if (shift_active) {
        return keyboard_layout_get_shift(base_code);
    }
    return normal;
}

static uint32_t map_keysym_to_unicode(uint32_t keysym) {
    if (keysym < 0x1000) return keysym; // Simple ASCII maps directly to Unicode
    return 0; // Non-printable (e.g. F1, Shift, Arrows)
}

static void process_dead_key(key_event_t *ev) {
    // Dead key composition (e.g. ` + e -> è)
    if (ev->unicode == '`' && current_dead_key == 0) {
        current_dead_key = '`';
        ev->unicode = 0; // Consume the key
        return;
    }
    if (current_dead_key == '`') {
        if (ev->unicode == 'e') ev->unicode = 0x00E8; // è
        else if (ev->unicode == 'a') ev->unicode = 0x00E0; // à
        current_dead_key = 0;
    }
}

void kbd_dispatch_event(key_event_t *ev) {
    // Input method framework (IME) integration could hook here
    // Forward to compositor. Push every press (printable and non-printable
    // like arrows/function keys) so userland gets full key state, not just
    // text input. The raw scancode travels in `code`, the unicode char in `x`.
    if (ev->pressed) {
        input_push(INPUT_EVENT_KEY_PRESS, ev->keycode, ev->unicode, 0);
    } else if (!ev->pressed) {
        input_push(INPUT_EVENT_KEY_RELEASE, ev->keycode, 0, 0);
    }
}

void hid_kbd_process_report(hid_device_t *hdev, uint8_t *report, size_t len) {
    if (len == 0) return;
    if (hdev->transport == HID_TRANSPORT_PS2) {
        uint8_t code = report[0];
        // E0 prefix handling
        if (code == 0xE0) {
            scancode_e0_prefix = 1;
            return;
        }
        uint8_t pressed = (code & 0x80) ? 0 : 1;
        uint8_t base_code = code & 0x7F;
        int is_e0 = scancode_e0_prefix; // capture prefix flag before reset
        uint32_t keycode = base_code | (is_e0 ? 0xE000 : 0);
        scancode_e0_prefix = 0; // reset
        // Modifier State Tracking – handle left/right modifiers
        // is_e0 indicates right-side modifier keys
        // Shift (both sides)
        if (base_code == 0x2A || base_code == 0x36) { // LShift / RShift
            if (pressed) current_modifiers |= KBD_MOD_SHIFT;
            else current_modifiers &= ~KBD_MOD_SHIFT;
        }
        // Ctrl – left (0x1D) or right (E0 0x1D)
        else if (base_code == 0x1D && (!is_e0 || is_e0)) {
            if (pressed) current_modifiers |= KBD_MOD_CTRL;
            else current_modifiers &= ~KBD_MOD_CTRL;
        }
        // Alt – left (0x38) or right Alt (E0 0x38) treated as AltGr
        else if (base_code == 0x38 && !is_e0) { // left Alt
            if (pressed) current_modifiers |= KBD_MOD_ALT;
            else current_modifiers &= ~KBD_MOD_ALT;
        }
        else if (base_code == 0x38 && is_e0) { // right Alt (AltGr)
            if (pressed) current_modifiers |= KBD_MOD_ALTGR;
            else current_modifiers &= ~KBD_MOD_ALTGR;
        }
        if (pressed && base_code == 0x3A) { // CapsLock
            current_modifiers ^= KBD_MOD_CAPS;
        }
        // Build the Keyboard Event
        key_event_t ev;
        ev.keycode = keycode;
        ev.pressed = pressed;
        ev.repeat = 0;
        ev.modifiers = current_modifiers;
        ev.timestamp = 0; // System timer needed
        // Keymap & Layout Engine translation
        ev.keysym = translate_scancode_to_keysym(base_code, (keycode & 0xE000), current_modifiers);
        ev.unicode = map_keysym_to_unicode(ev.keysym);
        // Apply dead key composition
        process_dead_key(&ev);
        // Update key repeat state
        if (pressed) {
            repeat_active = 1;
            repeat_keycode = ev.keycode;
            repeat_unicode = ev.unicode;
            repeat_press_time = get_ms_time();
            repeat_last_time = 0;
        } else {
            if (repeat_active && ev.keycode == repeat_keycode) {
                repeat_active = 0;
            }
        }
        // Process registered shortcuts
        keyboard_shortcuts_process_event(&ev);
        // Dispatch to input subsystem
        kbd_dispatch_event(&ev);
    }
}

// Called from the tick handler to generate key repeat events.
void hid_kbd_repeat_tick(void) {
    if (!repeat_active) return;
    uint64_t now = get_ms_time();
    if (now - repeat_press_time < repeat_delay_ms) {
        return; // not yet time for first repeat
    }
    if (repeat_last_time == 0) {
        repeat_last_time = now;
        // First repeat event
        input_push(INPUT_EVENT_KEY_PRESS, repeat_keycode, repeat_unicode, 0);
        return;
    }
    if (now - repeat_last_time >= repeat_rate_ms) {
        repeat_last_time = now;
        input_push(INPUT_EVENT_KEY_PRESS, repeat_keycode, repeat_unicode, 0);
    }
}
