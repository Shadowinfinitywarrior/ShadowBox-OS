#ifndef SHADOWBOX_ACCESSIBILITY_H
#define SHADOWBOX_ACCESSIBILITY_H

#include "types.h"
#include "gui_toolkit.h"

// AT-SPI2-like Accessibility Node (Semantic mapping of Widget Tree)
typedef struct a11y_node {
    uint32_t id;
    const char *role; // e.g., "button", "text_input", "dialog"
    const char *name; // e.g., "Submit", "Username"
    const char *description;
    
    // State
    uint8_t is_focused;
    uint8_t is_disabled;
    
    struct a11y_node *parent;
    struct a11y_node *children;
    struct a11y_node *next_sibling;
} a11y_node_t;

// Global Accessibility Settings
typedef struct a11y_settings {
    uint8_t screen_reader_active;
    uint8_t high_contrast_mode;
    float font_scaling_factor; // e.g., 1.0x, 1.5x
    uint8_t keyboard_only_nav;
} a11y_settings_t;

void a11y_init(void);
void a11y_update_settings(a11y_settings_t *settings);

// Focus Navigation (Keyboard-only nav)
void a11y_move_focus_next(void);
void a11y_move_focus_prev(void);

// Tree Synchronization (Maps UI Toolkit Widgets to A11y Nodes)
a11y_node_t* a11y_build_tree(widget_t *root_widget);

#endif
