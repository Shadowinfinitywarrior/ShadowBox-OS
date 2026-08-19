// gui_wrapper_label.cpp — C wrappers for Label
#include "Label.hpp"
#include "gui_api.h"

// Placement new
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {

gui_label_t gui_label_create(gui_widget_t parent) {
  return static_cast<gui_label_t>(new Label(static_cast<Widget*>(parent)));
}
void gui_label_destroy(gui_label_t l) {
  if (l) delete static_cast<Label*>(l);
}
void gui_label_set_text(gui_label_t l, const char* text) {
  if (l) static_cast<Label*>(l)->set_text(text);
}
}
