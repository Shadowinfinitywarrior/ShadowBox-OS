#ifndef GUI_PUBLIC_H
#define GUI_PUBLIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Core types used by the public API --- */
typedef void* gui_comp_t;
typedef void* gui_input_t;
typedef void* gui_widget_t;
typedef void* gui_window_t;
typedef void* gui_button_t;
typedef void* gui_label_t;
typedef void* gui_textbox_t;
typedef void* gui_scrollview_t;
typedef void* gui_context_menu_t;

/* --- Compositor --- */
gui_comp_t gui_compositor_create(void* fb, uint32_t stride, uint32_t w, uint32_t h);
void gui_compositor_destroy(gui_comp_t c);
void gui_compositor_add_root(gui_comp_t c, gui_widget_t w);
void gui_compositor_remove_root(gui_comp_t c, gui_widget_t w);
void gui_compositor_frame(gui_comp_t c);
void gui_compositor_set_backbuf(gui_comp_t c, void* buf);
gui_widget_t gui_compositor_focused(gui_comp_t c);
void gui_compositor_set_focus(gui_comp_t c, gui_widget_t w);

/* --- InputRouter --- */
gui_input_t gui_input_router_create(gui_comp_t c, int32_t sw, int32_t sh);
void gui_input_router_destroy(gui_input_t r);
void gui_input_router_mouse_packet(gui_input_t r, uint8_t buttons, int8_t dx, int8_t dy);
void gui_input_router_mouse_absolute(gui_input_t r, int32_t x, int32_t y, uint8_t buttons);
void gui_input_router_key_press(gui_input_t r, uint32_t key, uint8_t mods);
void gui_input_router_key_release(gui_input_t r, uint32_t key, uint8_t mods);
void gui_input_router_scroll(gui_input_t r, int32_t delta);
int32_t gui_input_router_mouse_x(gui_input_t r);
int32_t gui_input_router_mouse_y(gui_input_t r);

/* --- Widget --- */
void gui_widget_set_pos(gui_widget_t w, int32_t x, int32_t y);
void gui_widget_set_size(gui_widget_t w, int32_t w_, int32_t h);
void gui_widget_add_child(gui_widget_t parent, gui_widget_t child);
gui_widget_t gui_widget_parent(gui_widget_t w);

gui_window_t gui_window_create(gui_widget_t parent);
void gui_window_destroy(gui_window_t w);
void gui_window_set_title(gui_window_t w, const char* t);
void gui_window_set_pos(gui_window_t w, int32_t x, int32_t y);
void gui_window_set_size(gui_window_t w, int32_t w_, int32_t h);
void gui_window_add_client(gui_window_t w, gui_widget_t child);

/* --- Button --- */
gui_button_t gui_button_create(gui_widget_t parent);
void gui_button_destroy(gui_button_t b);
void gui_button_set_label(gui_button_t b, const char* text);
void gui_button_set_pos(gui_button_t b, int32_t x, int32_t y);
void gui_button_set_size(gui_button_t b, int32_t w_, int32_t h);
void gui_button_set_on_clicked(gui_button_t b, void (*fn)(void*));
void gui_button_set_user_data(gui_button_t b, void* ud);
void* gui_button_user_data(gui_button_t b);

/* --- Label --- */
gui_label_t gui_label_create(gui_widget_t parent);
void gui_label_destroy(gui_label_t l);
void gui_label_set_text(gui_label_t l, const char* text);

/* --- TextBox --- */
gui_textbox_t gui_textbox_create(gui_widget_t parent);
void gui_textbox_destroy(gui_textbox_t t);
void gui_textbox_set_pos(gui_textbox_t t, int32_t x, int32_t y);
void gui_textbox_set_size(gui_textbox_t t, int32_t w_, int32_t h);
void gui_textbox_set_text(gui_textbox_t t, const char* text);
void gui_textbox_set_placeholder(gui_textbox_t t, const char* ph);
const char* gui_textbox_text(gui_textbox_t t);

/* --- ScrollView --- */
gui_scrollview_t gui_scrollview_create(gui_widget_t parent);
void gui_scrollview_destroy(gui_scrollview_t s);
void gui_scrollview_set_pos(gui_scrollview_t s, int32_t x, int32_t y);
void gui_scrollview_set_size(gui_scrollview_t s, int32_t w_, int32_t h);
void gui_scrollview_set_content(gui_scrollview_t s, gui_widget_t c);

/* --- ContextMenu --- */
gui_context_menu_t gui_context_menu_create(gui_widget_t parent);
void gui_context_menu_destroy(gui_context_menu_t menu);
void gui_context_menu_add_item(gui_context_menu_t menu, const char* label, void (*cb)(void*), void* ud);

#ifdef __cplusplus
}
#endif

#endif /* GUI_PUBLIC_H */