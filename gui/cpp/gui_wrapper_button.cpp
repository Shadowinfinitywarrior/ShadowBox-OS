// gui_wrapper_button.cpp — C wrappers for Button
#include "Button.hpp"
#include "gui_api.h"

// Placement new
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {

gui_button_t gui_button_create(gui_widget_t parent) {
  return static_cast<gui_button_t>(new Button(static_cast<Widget*>(parent)));
}
void gui_button_destroy(gui_button_t b) {
  if (b) delete static_cast<Button*>(b);
}
void gui_button_set_label(gui_button_t b, const char* text) {
  if (b) static_cast<Button*>(b)->set_label(text);
}
void gui_button_set_pos(gui_button_t b, int32_t x, int32_t y) {
  if (b) static_cast<Button*>(b)->set_pos(x, y);
}
void gui_button_set_size(gui_button_t b, int32_t w_, int32_t h) {
  if (b) static_cast<Button*>(b)->set_size(w_, h);
}
void gui_button_set_on_clicked(gui_button_t b, void (*fn)(void*)) {
  if (!b) return;
  Button* btn = static_cast<Button*>(b);
  btn->user_data = reinterpret_cast<void*>(fn);
  btn->on_clicked = button_click_trampoline;
}
void gui_button_set_user_data(gui_button_t b, void* ud) {
  if (b) static_cast<Button*>(b)->user_data = ud;
}
void* gui_button_user_data(gui_button_t b) {
  if (!b) return nullptr;
  return static_cast<Button*>(b)->user_data;
}
}
