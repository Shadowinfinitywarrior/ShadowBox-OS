#ifndef SHADOWBOX_COMPOSITOR_H
#define SHADOWBOX_COMPOSITOR_H

#include "types.h"
#include "drm.h"
#include "input.h"

// Wayland-inspired Protocol Objects

typedef struct wl_buffer {
    drm_gem_object_t *gem_obj;
    uint32_t width, height, stride;
    uint32_t format;
} wl_buffer_t;

typedef struct wl_surface {
    uint32_t id;
    wl_buffer_t *current_buffer;
    
    // Damage Tracking (Only re-render changed regions)
    int32_t damage_x, damage_y;
    uint32_t damage_w, damage_h;
    
    // Scene Graph Properties
    int32_t x, y;
    float alpha;
    
    struct wl_surface *next;
} wl_surface_t;

typedef struct xdg_toplevel {
    wl_surface_t *surface;
    char title[64];
    char app_id[64];
    uint8_t maximized;
    uint8_t fullscreen;
} xdg_toplevel_t;

typedef struct wl_seat {
    uint32_t id;
    uint32_t capabilities; // Pointer, Keyboard, Touch mask
} wl_seat_t;

typedef struct wl_output {
    uint32_t id;
    drm_crtc_t *crtc;
    int32_t x, y; // Multi-monitor coordinate space
    uint32_t refresh_rate;
    uint8_t hdr_enabled;
} wl_output_t;

void compositor_init(void);

// Core Compositor Loop (GPU-accelerated Composition + VSync)
void compositor_composite_frame(void);

// App Protocol Stubs
wl_surface_t* compositor_create_surface(void);
void compositor_commit_buffer(wl_surface_t *surf, wl_buffer_t *buf);

// --- Window Protocol (Input Routing) ---

struct libinput_event; // Forward decl

typedef struct wl_pointer {
    wl_surface_t *focus;
    double current_x, current_y;
} wl_pointer_t;

typedef struct wl_keyboard {
    wl_surface_t *focus;
    uint32_t modifiers; // Shift, Ctrl, Alt
} wl_keyboard_t;

typedef struct wl_touch {
    wl_surface_t *focus;
    // Multi-touch slots and tracking
} wl_touch_t;

// Drag & Drop routing
typedef struct wl_dnd {
    wl_surface_t *source;
    wl_surface_t *target;
    void *drag_data;
} wl_dnd_t;

// Compositor Hit Testing & Event Dispatch
int compositor_hit_test(double x, double y, wl_surface_t **hit_surface);
int compositor_handle_global_shortcut(uint32_t key, uint32_t modifiers);
void compositor_route_input_event(struct libinput_event *ev);

#endif
