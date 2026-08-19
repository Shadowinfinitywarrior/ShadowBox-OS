// gui_wrapper_widget.cpp — C wrappers for Widget base class
#include "Widget.hpp"
#include "gui_api.h"

// Placement new
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {

void gui_widget_set_pos(gui_widget_t w, int32_t x, int32_t y) {
  if (w) static_cast<Widget*>(w)->set_pos(x, y);
}
void gui_widget_set_size(gui_widget_t w, int32_t w_, int32_t h) {
  if (w) static_cast<Widget*>(w)->set_size(w_, h);
}
void gui_widget_add_child(gui_widget_t parent, gui_widget_t child) {
  if (parent && child) static_cast<Widget*>(parent)->add_child(static_cast<Widget*>(child));
}
gui_widget_t gui_widget_parent(gui_widget_t w) {
  if (w) return static_cast<gui_widget_t>(static_cast<Widget*>(w)->parent());
  return nullptr;
}

// Flag helpers
gui_bool_t gui_widget_visible(gui_widget_t w) {
  if (!w) return 0;
  return static_cast<Widget*>(w)->visible() ? 1 : 0;
}
gui_bool_t gui_widget_enabled(gui_widget_t w) {
  if (!w) return 0;
  return static_cast<Widget*>(w)->enabled() ? 1 : 0;
}
gui_bool_t gui_widget_focused(gui_widget_t w) {
  if (!w) return 0;
  return static_cast<Widget*>(w)->focused() ? 1 : 0;
}
gui_bool_t gui_widget_hovered(gui_widget_t w) {
  if (!w) return 0;
  return static_cast<Widget*>(w)->hovered() ? 1 : 0;
}
gui_bool_t gui_widget_pressed(gui_widget_t w) {
  if (!w) return 0;
  return static_cast<Widget*>(w)->pressed() ? 1 : 0;
}
}
