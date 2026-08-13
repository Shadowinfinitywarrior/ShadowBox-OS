#ifndef SHADOWBOX_UI_COLOR_PICKER_H
#define SHADOWBOX_UI_COLOR_PICKER_H

#include "gui_toolkit.h"

// Create a color picker widget. Returns a pointer to the widget, or NULL on failure.
widget_t* color_picker_create(int32_t x, int32_t y, uint32_t width, uint32_t height);

// Set the current selected color of the color picker.
void color_picker_set_color(widget_t *picker, uint32_t color);

// Get the current selected color from the color picker.
uint32_t color_picker_get_color(const widget_t *picker);

#endif // SHADOWBOX_UI_COLOR_PICKER_H