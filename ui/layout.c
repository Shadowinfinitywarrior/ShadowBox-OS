// UI Layout Manager Implementation
// Provides a simple layout manager for the OS kernel UI.
// Implements vertical stacking of child widgets with padding and a basic
// layout pass that updates widget positions. This replaces the previous stub
// implementation.

#include <stddef.h>
#include <stdint.h>

// Include relevant UI headers.
#include "gui_toolkit.h"
#include "wm.h"

// Initialize layout subsystem (no-op for now).
void ui_layout_init(void) {
    // No global state needed for the simple layout manager.
}

// Set the layout mode for a workspace. Returns 0 on success.
int ui_set_layout_mode(workspace_t *ws, wm_layout_mode_t mode) {
    if (ws) {
        ws->layout = mode;
    }
    return 0;
}

// Internal helper: recursively layout a widget and its children with padding.
static void layout_widget_recursive(widget_t *w, int32_t offset_x, int32_t offset_y) {
    if (!w) return;
    // Apply the given offset as the widget's absolute position.
    w->x = offset_x;
    w->y = offset_y;
    // Simple vertical stacking layout for children with padding.
    const int padding = 8;   // inner padding from widget border
    const int spacing = 4;   // spacing between stacked children
    widget_t *child = w->children;
    int32_t child_y = offset_y + padding;
    while (child) {
        // Position child with horizontal padding.
        layout_widget_recursive(child,
                                offset_x + padding,
                                child_y);
        // Advance Y position for next sibling.
        child_y += child->height + spacing;
        child = child->next_sibling;
    }
}

// Perform a layout pass for a window.
void ui_layout_pass(window_t *win) {
    if (!win || !win->root_widget) return;
    // Start layout from the root widget's current position.
    layout_widget_recursive(win->root_widget,
                            win->root_widget->x,
                            win->root_widget->y);
}

// Cleanup layout subsystem (no-op for now).
void ui_layout_cleanup(void) {
    // No allocated resources in this simple manager.
}
