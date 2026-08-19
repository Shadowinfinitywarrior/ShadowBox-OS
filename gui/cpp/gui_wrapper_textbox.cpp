// gui_wrapper_textbox.cpp — C wrappers for TextBox
#include "TextBox.hpp"
#include "gui_api.h"

// Placement new
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {

gui_textbox_t gui_textbox_create(gui_widget_t parent) {
  return static_cast<gui_textbox_t>(new TextBox(static_cast<Widget*>(parent)));
}
void gui_textbox_destroy(gui_textbox_t t) {
  if (t) delete static_cast<TextBox*>(t);
}
void gui_textbox_set_pos(gui_textbox_t t, int32_t x, int32_t y) {
  if (t) static_cast<TextBox*>(t)->set_pos(x, y);
}
void gui_textbox_set_size(gui_textbox_t t, int32_t w_, int32_t h) {
  if (t) static_cast<TextBox*>(t)->set_size(w_, h);
}
void gui_textbox_set_text(gui_textbox_t t, const char* text) {
  if (t) static_cast<TextBox*>(t)->set_text(text);
}
void gui_textbox_set_placeholder(gui_textbox_t t, const char* ph) {
  if (t) static_cast<TextBox*>(t)->set_placeholder(ph);
}
const char* gui_textbox_text(gui_textbox_t t) {
  if (!t) return "";
  return static_cast<TextBox*>(t)->text();
}
}
