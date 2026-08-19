// gui_wrapper_contextmenu.cpp — C wrappers for ContextMenu
#include "ContextMenu.hpp"
#include "gui_api.h"

// Placement new
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {

gui_context_menu_t gui_context_menu_create(gui_widget_t parent) {
  return static_cast<gui_context_menu_t>(new ContextMenu(static_cast<Widget*>(parent)));
}
void gui_context_menu_destroy(gui_context_menu_t menu) {
  if (menu) delete static_cast<ContextMenu*>(menu);
}
void gui_context_menu_add_item(gui_context_menu_t menu, const char* label, void (*cb)(void*), void* ud) {
  if (!menu) return;
  static_cast<ContextMenu*>(menu)->add_item(label, cb, ud);
}
}
