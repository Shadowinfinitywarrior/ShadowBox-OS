// gui_wrapper_scrollview.cpp — C wrappers for ScrollView
#include "ScrollView.hpp"
#include "gui_api.h"

// Placement new
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {

gui_scrollview_t gui_scrollview_create(gui_widget_t parent) {
  return static_cast<gui_scrollview_t>(new ScrollView(static_cast<Widget*>(parent)));
}
void gui_scrollview_destroy(gui_scrollview_t s) {
  if (s) delete static_cast<ScrollView*>(s);
}
void gui_scrollview_set_pos(gui_scrollview_t s, int32_t x, int32_t y) {
  if (s) static_cast<ScrollView*>(s)->set_pos(x, y);
}
void gui_scrollview_set_size(gui_scrollview_t s, int32_t w_, int32_t h) {
  if (s) static_cast<ScrollView*>(s)->set_size(w_, h);
}
void gui_scrollview_set_content(gui_scrollview_t s, gui_widget_t c) {
  if (s) static_cast<ScrollView*>(s)->set_content(static_cast<Widget*>(c));
}
}
