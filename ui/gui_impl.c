#include "gui_toolkit.h"

// ── C++ widget layer bridge ────────────────────────────────────────────────
// These forward-declarations allow C code (desktop.c) to call into the
// C++ Compositor and InputRouter without including C++ headers.
// The actual implementations live in gui/cpp/Compositor.cpp and
// gui/cpp/InputRouter.cpp which export these as extern "C".

struct Compositor;  // opaque C handle
struct InputRouter; // opaque C handle
struct Widget;      // opaque C handle

// Compositor
extern struct Compositor* compositor_create(void* fb, unsigned int stride,
                                            unsigned int w, unsigned int h);
extern void compositor_destroy   (struct Compositor* c);
extern void compositor_add_root  (struct Compositor* c, struct Widget* w);
extern void compositor_frame     (struct Compositor* c);
extern void compositor_invalidate(struct Compositor* c);
extern void compositor_set_focus (struct Compositor* c, struct Widget* w);
extern void compositor_set_backbuf(struct Compositor* c, void* buf);

// InputRouter
extern struct InputRouter* input_router_create(struct Compositor* c,
                                               int sw, int sh);
extern void input_router_destroy       (struct InputRouter* r);
extern void input_router_mouse_packet  (struct InputRouter* r,
                                        unsigned char buttons,
                                        signed char dx, signed char dy);
extern void input_router_mouse_absolute(struct InputRouter* r,
                                        int x, int y, unsigned char buttons);
extern void input_router_key_press     (struct InputRouter* r,
                                        unsigned int key, unsigned char mods);
extern void input_router_scroll        (struct InputRouter* r, int delta);
extern int  input_router_mouse_x       (struct InputRouter* r);
extern int  input_router_mouse_y       (struct InputRouter* r);
extern unsigned int input_router_scancode(unsigned char sc, int shift);
// ──────────────────────────────────────────────────────────────────────────

// Global cursor state for the GUI
static int32_t cursor_x = 0;
static int32_t cursor_y = 0;
#include "libinput.h"
#include <stdlib.h>

// Forward declarations removed as they are in gui_toolkit.h


// Simple window creation – in a real system this would interface with the compositor.
window_t* gui_create_window(const char *title) {
    (void)title; // title unused in stub
    window_t *win = (window_t*)kmalloc(sizeof(window_t));
    if (!win) return NULL;
    win->toplevel = NULL; // placeholder; compositor integration not shown
    win->root_widget = NULL;
    return win;
}

// Recursive helper to test if a point lies within a widget's bounds.
static widget_t* hit_test_recursive(widget_t *w, int32_t x, int32_t y) {
    if (!w) return NULL;
    if (x >= w->x && x < w->x + (int32_t)w->width &&
        y >= w->y && y < w->y + (int32_t)w->height) {
        // Check children first for deeper hit.
        widget_t *child = w->children;
        while (child) {
            widget_t *res = hit_test_recursive(child, x - w->x, y - w->y);
            if (res) return res;
            child = child->next_sibling;
        }
        return w;
    }
    return NULL;
}

// Public hit‑test: translate to root coordinates using global cursor position.
widget_t* gui_hit_test(window_t *win) {
    if (!win || !win->root_widget) return NULL;
    return hit_test_recursive(win->root_widget, cursor_x, cursor_y);
}

widget_t* gui_hit_test_at(window_t *win, int32_t x, int32_t y) {
    if (!win || !win->root_widget) return NULL;
    return hit_test_recursive(win->root_widget, x, y);
}

// Dispatch a libinput event to the appropriate widget.
void gui_dispatch_event(window_t *win, libinput_event_t *ev) {
    if (!win || !ev) return;
    switch (ev->type) {
        case LIBINPUT_EVENT_POINTER_MOTION: {
            // Update cursor position with relative motion
            cursor_x += (int32_t)ev->data.pointer_motion.dx;
            cursor_y += (int32_t)ev->data.pointer_motion.dy;
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE: {
            // For absolute motion, replace cursor coordinates (assuming data provides x,y in pointer_motion)
            cursor_x = (int32_t)ev->data.pointer_motion.dx;
            cursor_y = (int32_t)ev->data.pointer_motion.dy;
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON: {
            // Simple button press handling – find widget under cursor.
            // For this stub we assume the coordinates are stored elsewhere; use (0,0).
            // In a full implementation you would obtain the latest cursor position.
            widget_t *w = gui_hit_test(win);
                if (w) {
                    widget_set_focus(w);
                    // TODO: Draw focus highlight around widget
                }
            if (w && w->on_click) {
                w->on_click(w);
            }
            break;
        }
        case LIBINPUT_EVENT_TOUCH_DOWN: {
            // Set cursor to touch coordinates and simulate click
            cursor_x = (int32_t)ev->data.touch.x;
            cursor_y = (int32_t)ev->data.touch.y;
            widget_t *w = gui_hit_test(win);
            if (w) {
                widget_set_focus(w);
                if (w->on_click) w->on_click(w);
            }
            break;
        }
        case LIBINPUT_EVENT_TOUCH_UP: {
            // No action for touch release in this stub
            break;
        }
        case LIBINPUT_EVENT_TOUCH_MOTION: {
            // Update cursor to new touch position
            cursor_x = (int32_t)ev->data.touch.x;
            cursor_y = (int32_t)ev->data.touch.y;
            break;
        }
        case LIBINPUT_EVENT_KEYBOARD_KEY: {
            // Forward keyboard event to the currently focused widget, if it has a handler.
            widget_t *fw = widget_get_focused();
            if (fw && fw->on_key) {
                fw->on_key(fw, ev->data.keyboard_key.key);
            }
            break;
        }
        default:
            // Other event types not handled in this minimal stub.
            break;
    }
}

/* Note: The above implementations are minimal and intended only to make the GUI
 * subsystem operational enough for basic interaction testing. Real drawing,
 * compositor integration, cursor tracking, and full input handling should be
 * added later.
 */
