// gui_wrapper_compositor.cpp — C wrappers for Compositor only
#include "Compositor.hpp"
#include "gui_api.h"

// Placement new required by Compositor wrapper.
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {

gui_comp_t gui_compositor_create(void* fb, uint32_t stride, uint32_t w, uint32_t h) {
  void* mem = std::malloc(sizeof(Compositor));
  if (!mem) return nullptr;
  return new(mem) Compositor(fb, stride, w, h);
}
void gui_compositor_destroy(gui_comp_t c) {
  if (!c) return;
  static_cast<Compositor*>(c)->~Compositor();
  std::free(c);
}
void gui_compositor_add_root(gui_comp_t c, gui_widget_t w) {
  if (c) static_cast<Compositor*>(c)->add_root(static_cast<Widget*>(w));
}
void gui_compositor_remove_root(gui_comp_t c, gui_widget_t w) {
  if (c) static_cast<Compositor*>(c)->remove_root(static_cast<Widget*>(w));
}
void gui_compositor_frame(gui_comp_t c) {
  if (c) static_cast<Compositor*>(c)->frame();
}
void gui_compositor_set_backbuf(gui_comp_t c, void* buf) {
  if (c) static_cast<Compositor*>(c)->backbuf = buf;
}
gui_widget_t gui_compositor_focused(gui_comp_t c) {
  if (!c) return nullptr;
  return static_cast<gui_widget_t>(static_cast<Compositor*>(c)->focused());
}
void gui_compositor_set_focus(gui_comp_t c, gui_widget_t w) {
  if (c) static_cast<Compositor*>(c)->set_focus(static_cast<Widget*>(w));
}
}
