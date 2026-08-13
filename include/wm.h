#ifndef SHADOWBOX_WM_H
#define SHADOWBOX_WM_H

#include "types.h"
#include "compositor.h"

// Window Manager Layout Modes
typedef enum {
    WM_LAYOUT_FLOATING,
    WM_LAYOUT_TILING
} wm_layout_mode_t;

// Virtual Desktop / Workspace
typedef struct workspace {
    uint32_t id;
    wm_layout_mode_t layout;
    xdg_toplevel_t *windows_head; // Z-stacked (Floating) or Tree (Tiled)
    struct workspace *next;
} workspace_t;

// Window Manager State
typedef struct wm_state {
    workspace_t *workspaces;
    workspace_t *active_workspace;
    
    // Snapping & Picture-in-Picture
    uint32_t magnetic_snap_distance;
    xdg_toplevel_t *pip_window; // Picture-in-picture floating above all
} wm_state_t;

void wm_init(void);
void wm_set_layout(workspace_t *ws, wm_layout_mode_t layout);

// Layout & Window control
void wm_handle_window_move(xdg_toplevel_t *win, int32_t x, int32_t y); // Calculates magnetic snapping to edges/windows
void wm_toggle_pip(xdg_toplevel_t *win);

// Visual transitions (Hooks into animation engine)
void wm_animate_window_open(xdg_toplevel_t *win); 
void wm_animate_window_close(xdg_toplevel_t *win);

#endif
