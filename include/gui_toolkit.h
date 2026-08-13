#ifndef SHADOWBOX_GUI_TOOLKIT_H
#define SHADOWBOX_GUI_TOOLKIT_H

#include "types.h"
#include "compositor.h"
#include "libinput.h"

// Widget Types (Core Widgets)
typedef enum {
    WIDGET_TYPE_CONTAINER,
    WIDGET_TYPE_TITLEBAR,
    WIDGET_TYPE_SIDEBAR,
    WIDGET_TYPE_SCROLL_CONTAINER,
    WIDGET_TYPE_BUTTON,
    WIDGET_TYPE_TOGGLE,
    WIDGET_TYPE_SLIDER,
    WIDGET_TYPE_SPINNER,
    WIDGET_TYPE_TEXTINPUT,
    WIDGET_TYPE_TEXTAREA, // With IME integration
    WIDGET_TYPE_DROPDOWN,
    WIDGET_TYPE_CONTEXT_MENU,
    WIDGET_TYPE_MENU_BAR,
    WIDGET_TYPE_LISTVIEW,
    WIDGET_TYPE_GRIDVIEW,
    WIDGET_TYPE_TREEVIEW,
    WIDGET_TYPE_DIALOG,
    WIDGET_TYPE_SHEET,
    WIDGET_TYPE_POPOVER,
    WIDGET_TYPE_TOOLTIP,
    WIDGET_TYPE_TABBAR,
    WIDGET_TYPE_NAVIGATION_STACK,
    WIDGET_TYPE_SPLITVIEW,
    WIDGET_TYPE_PROGRESSBAR,
    WIDGET_TYPE_ACTIVITY_INDICATOR,
    WIDGET_TYPE_NOTIFICATION,
    WIDGET_TYPE_COLOR_PICKER
} widget_type_t;

// Application Widget (UI Component Tree Node)
typedef struct widget {
    uint32_t id;
    widget_type_t type;
    
    int32_t x, y;
    uint32_t width, height;
    
    // Event Handlers
    void (*on_click)(struct widget *w);
    void (*on_hover)(struct widget *w);
    void (*on_key)(struct widget *w, uint32_t key);
    
    // UI State
    uint8_t is_hovered;
    uint8_t is_focused;
    
    struct widget *parent;
    struct widget *children;
    struct widget *next_sibling;
} widget_t;

typedef struct window {
    xdg_toplevel_t *toplevel;
    widget_t *root_widget; // Top of the hierarchical Widget Tree
} window_t;

// Toolkit Core API
window_t* gui_create_window(const char *title);

// Widget API
widget_t *widget_create(widget_type_t type, int32_t x, int32_t y, uint32_t width, uint32_t height);
void widget_destroy(widget_t *w);
void widget_set_parent(widget_t *child, widget_t *parent);
void widget_set_focus(widget_t *w);
widget_t* widget_get_focused(void);

// Input routing down to widgets
widget_t* gui_hit_test(window_t *win);
widget_t* gui_hit_test_at(window_t *win, int32_t local_x, int32_t local_y);
void gui_dispatch_event(window_t *win, libinput_event_t *ev);

// Rendering Pipeline Hooks (Layout -> Paint -> Display List)
void gui_layout_pass(window_t *win);
void gui_paint_pass(window_t *win); // Generates display list for rendering engine
void gui_mark_damage(window_t *win, widget_t *w);

#endif
