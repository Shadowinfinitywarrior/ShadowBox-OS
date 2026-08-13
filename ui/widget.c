// Minimal stub implementation for UI widget subsystem
//
// NOTE: The production widget implementation now lives in gui/cpp/ (Widget.cpp,
// Button.cpp, Label.cpp, Window.cpp, TextBox.cpp, ScrollView.cpp).  Those C++
// classes are compiled into desktop.elf.
//
// The stubs below remain in place for:
//   (a) Kernel-side code that #includes gui_toolkit.h (ui/ is linked into os.bin).
//   (b) Backward-compat with existing call sites that use widget_t* / widget_create().
//
// The C++ layer is entirely independent and does NOT call these stubs.

#include "malloc.h"
#define __NLINK_T_DEFINED
#include "gui_toolkit.h"

// Global focused widget
static widget_t *focused_widget = NULL;

// Set focus to a widget, updating previous focus state
void widget_set_focus(widget_t *w) {
    if (focused_widget && focused_widget != w) {
        focused_widget->is_focused = 0;
    }
    focused_widget = w;
    if (w) {
        w->is_focused = 1;
    }
}

// Retrieve the currently focused widget
widget_t* widget_get_focused(void) {
    return focused_widget;
}

static uint32_t next_widget_id = 1; // simple id generator

/* Create a new widget with given parameters. */
widget_t *widget_create(widget_type_t type,
                        int32_t x, int32_t y,
                        uint32_t width, uint32_t height)
{
    widget_t *w = (widget_t *)kmalloc(sizeof(widget_t));
    if (!w)
        return NULL;
    w->id = next_widget_id++;
    w->type = type;
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->on_click = NULL;
    w->on_hover = NULL;
    w->on_key = NULL;
    w->is_hovered = 0;
    w->is_focused = 0;
    w->parent = NULL;
    w->children = NULL;
    w->next_sibling = NULL;
    return w;
}

/* Destroy a widget and its children recursively. */
void widget_destroy(widget_t *w)
{
    if (!w)
        return;
    // Recursively free children list
    widget_t *child = w->children;
    while (child) {
        widget_t *next = child->next_sibling;
        widget_destroy(child);
        child = next;
    }
    kfree(w);
}

/* Attach a child widget to a parent. */
void widget_set_parent(widget_t *child, widget_t *parent)
{
    if (!child || !parent)
        return;
    child->parent = parent;
    // Simple singly‑linked list of children
    child->next_sibling = parent->children;
    parent->children = child;
}

/* Placeholder implementations for toolkit hooks related to widgets.
   These are intentionally no‑ops; real functionality lives elsewhere. */
void gui_layout_pass(window_t *win) { (void)win; }
void gui_paint_pass(window_t *win) { (void)win; }
void gui_mark_damage(window_t *win, widget_t *w) { (void)win; (void)w; }

// End of stub
