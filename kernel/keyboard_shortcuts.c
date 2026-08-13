// Keyboard Shortcuts Component Implementation

#include "keyboard_shortcuts.h"
#include "kernel.h" // for printk

// Simple fixed-size registry for shortcuts
#define MAX_SHORTCUTS 8

struct shortcut_entry {
    uint32_t keysym;
    uint32_t modifiers;
    void (*action)(void);
};

static struct shortcut_entry shortcuts[MAX_SHORTCUTS];
static int shortcut_count = 0;

// Default action for Ctrl+Alt+T (open terminal placeholder)
static void default_action_terminal(void) {
    printk(KERN_INFO "Keyboard Shortcut: Ctrl+Alt+T triggered (placeholder)\n");
    // In a full OS, this would launch the terminal service.
}

void keyboard_shortcuts_init(void) {
    // Register the default terminal shortcut.
    keyboard_shortcuts_register('t', KBD_MOD_CTRL | KBD_MOD_ALT, default_action_terminal);
}

int keyboard_shortcuts_register(uint32_t keysym, uint32_t modifiers, void (*action)(void)) {
    if (shortcut_count >= MAX_SHORTCUTS) {
        return -1; // registry full
    }
    shortcuts[shortcut_count].keysym = keysym;
    shortcuts[shortcut_count].modifiers = modifiers;
    shortcuts[shortcut_count].action = action;
    shortcut_count++;
    return 0;
}

void keyboard_shortcuts_process_event(const key_event_t *ev) {
    if (!ev || !ev->pressed) return;
    for (int i = 0; i < shortcut_count; ++i) {
        if (ev->keysym == shortcuts[i].keysym &&
            (ev->modifiers & shortcuts[i].modifiers) == shortcuts[i].modifiers) {
            if (shortcuts[i].action) {
                shortcuts[i].action();
            }
        }
    }
}
