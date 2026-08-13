// Minimal C wrappers for the C++ GUI toolkit.
// Keeps desktop.c in pure C while reusing Compositor/InputRouter/Window/Button/etc.
#include "Compositor.hpp"
#include "InputRouter.hpp"
#include "Window.hpp"
#include "Widget.hpp"
#include "Button.hpp"
#include "Label.hpp"
#include "TextBox.hpp"
#include "ScrollView.hpp"
#include "ContextMenu.hpp"
#include <cstdlib>
#include "gui_api.h"

// Placement new required by Compositor/InputRouter wrappers.
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

// Trampoline for Button callbacks: C function pointer -> Widget::on_clicked
static void button_click_trampoline(Widget* w) {
  if (!w || !w->user_data) return;
  void (*fn)(void*) = reinterpret_cast<void (*)(void*)>(w->user_data);
  fn(w);
}

extern "C" {

typedef void* gui_comp_t;
typedef void* gui_input_t;
typedef void* gui_widget_t;
typedef void* gui_window_t;
typedef void* gui_button_t;
typedef void* gui_label_t;
typedef void* gui_textbox_t;
typedef void* gui_scrollview_t;

// ---- Compositor ----
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

// ---- InputRouter ----
gui_input_t gui_input_router_create(gui_comp_t c, int32_t sw, int32_t sh) {
  void* mem = std::malloc(sizeof(InputRouter));
  if (!mem) return nullptr;
  return new(mem) InputRouter(static_cast<Compositor*>(c), sw, sh);
}
void gui_input_router_destroy(gui_input_t r) {
  if (!r) return;
  static_cast<InputRouter*>(r)->~InputRouter();
  std::free(r);
}
void gui_input_router_mouse_packet(gui_input_t r, uint8_t buttons, int8_t dx, int8_t dy) {
  if (r) static_cast<InputRouter*>(r)->inject_mouse_packet(buttons, dx, dy);
}
void gui_input_router_mouse_absolute(gui_input_t r, int32_t x, int32_t y, uint8_t buttons) {
  if (r) static_cast<InputRouter*>(r)->inject_mouse_absolute(x, y, buttons);
}
void gui_input_router_key_press(gui_input_t r, uint32_t key, uint8_t mods) {
  if (r) static_cast<InputRouter*>(r)->inject_key_press(key, mods);
}
void gui_input_router_key_release(gui_input_t r, uint32_t key, uint8_t mods) {
  if (r) static_cast<InputRouter*>(r)->inject_key_release(key, mods);
}
void gui_input_router_scroll(gui_input_t r, int32_t delta) {
  if (r) static_cast<InputRouter*>(r)->inject_scroll(delta);
}

// ---- Widget helpers ----
void gui_widget_set_pos(gui_widget_t w, int32_t x, int32_t y) {
  if (w) static_cast<Widget*>(w)->set_pos(x, y);
}
void gui_widget_set_size(gui_widget_t w, int32_t w_, int32_t h) {
  if (w) static_cast<Widget*>(w)->set_size(w_, h);
}
void gui_widget_add_child(gui_widget_t parent, gui_widget_t child) {
  if (parent) static_cast<Widget*>(parent)->add_child(static_cast<Widget*>(child));
}
gui_widget_t gui_widget_parent(gui_widget_t w) {
  if (!w) return nullptr;
  return static_cast<gui_widget_t>(static_cast<Widget*>(w)->parent());
}

// ---- Window ----
gui_window_t gui_window_create(gui_widget_t /*parent*/) {
  return static_cast<gui_window_t>(new Window(nullptr));
}
void gui_window_destroy(gui_window_t w) {
  if (w) delete static_cast<Window*>(w);
}
void gui_window_set_title(gui_window_t w, const char* t) {
  if (w) static_cast<Window*>(w)->set_title(t);
}
void gui_window_set_pos(gui_window_t w, int32_t x, int32_t y) {
  if (w) static_cast<Window*>(w)->set_pos(x, y);
}
void gui_window_set_size(gui_window_t w, int32_t w_, int32_t h) {
  if (w) static_cast<Window*>(w)->set_size(w_, h);
}
void gui_window_add_client(gui_window_t w, gui_widget_t child) {
  if (w) static_cast<Window*>(w)->add_client(static_cast<Widget*>(child));
}

// ---- Button ----
gui_button_t gui_button_create(gui_widget_t parent) {
  return static_cast<gui_button_t>(new Button(parent ? static_cast<Widget*>(parent) : nullptr));
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

// ---- Label ----
gui_label_t gui_label_create(gui_widget_t parent) {
  return static_cast<gui_label_t>(new Label(parent ? static_cast<Widget*>(parent) : nullptr));
}
void gui_label_destroy(gui_label_t l) {
  if (l) delete static_cast<Label*>(l);
}
void gui_label_set_text(gui_label_t l, const char* text) {
  if (l) static_cast<Label*>(l)->set_text(text);
}

// ---- TextBox ----
gui_textbox_t gui_textbox_create(gui_widget_t parent) {
  return static_cast<gui_textbox_t>(new TextBox(parent ? static_cast<Widget*>(parent) : nullptr));
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

// ---- ScrollView ----
gui_scrollview_t gui_scrollview_create(gui_widget_t parent) {
  return static_cast<gui_scrollview_t>(new ScrollView(parent ? static_cast<Widget*>(parent) : nullptr));
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

    // ContextMenu C wrappers
    gui_context_menu_t gui_context_menu_create(gui_widget_t parent) {
        return static_cast<gui_context_menu_t>(new ContextMenu(parent ? static_cast<Widget*>(parent) : nullptr));
    }
    void gui_context_menu_destroy(gui_context_menu_t menu) {
        if (menu) delete static_cast<ContextMenu*>(menu);
    }
    void gui_context_menu_add_item(gui_context_menu_t menu, const char* label, void (*cb)(void*), void* ud) {
        if (!menu) return;
        static_cast<ContextMenu*>(menu)->add_item(label, cb, ud);
    }

} // extern "C"
