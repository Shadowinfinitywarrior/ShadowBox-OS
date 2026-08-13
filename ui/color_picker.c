#include "color_picker.h"

// Simple static storage for the selected color. In a full implementation this would be per-widget state.
static uint32_t current_color = 0xFFFFFFFF; // default to white

// Create a color picker widget. Currently a thin wrapper around widget_create.
widget_t* color_picker_create(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    // Use the new widget type defined in gui_toolkit.h.
    widget_t *w = widget_create(WIDGET_TYPE_COLOR_PICKER, x, y, width, height);
    // No additional widget-specific state is initialized in this stub.
    return w;
}

// Set the currently selected color. This stub stores the color globally.
void color_picker_set_color(widget_t *picker, uint32_t color) {
    (void)picker; // Unused in stub implementation.
    current_color = color;
}

// Get the currently selected color.
uint32_t color_picker_get_color(const widget_t *picker) {
    (void)picker; // Unused in stub implementation.
    return current_color;
}
