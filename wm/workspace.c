/*
 * Virtual Desktops (Workspaces) implementation for the window manager.
 * This provides a minimal functional stub that creates a fixed number of
 * workspaces, tracks the active workspace, and offers basic APIs to switch
 * workspaces and associate windows with them. The implementation is kept
 * deliberately lightweight – it does not perform any rendering or complex
 * state management, but it satisfies the compile‑time requirements and
 * lays the groundwork for future extensions.
 */

#include "wm.h"
#include "compositor.h"
#include <stddef.h>

/* Number of virtual workspaces the system provides. */
#define DEFAULT_NUM_WORKSPACES 4

/* Static storage for workspaces and the global WM state.
 * Using static allocation avoids dynamic memory allocation, which is a
 * reasonable constraint for early kernel‑space code.
 */
static workspace_t workspaces[DEFAULT_NUM_WORKSPACES];
static wm_state_t wm_state;

/* Forward declarations of internal helpers. */
static workspace_t *find_workspace_by_id(uint32_t id);

/* Initialise the window manager and create the default workspaces.
 * This function is declared in include/wm.h and is invoked by the kernel
 * early‑initialisation code (or can be called manually from tests). It sets
 * up a linked list of workspaces, assigns IDs 1‑4, selects the first workspace
 * as active, and initializes default layout and snap distance.
 */
void wm_init(void) {
    for (uint32_t i = 0; i < DEFAULT_NUM_WORKSPACES; ++i) {
        workspaces[i].id = i + 1;                     // IDs start at 1
        workspaces[i].layout = WM_LAYOUT_FLOATING;  // Default layout mode
        workspaces[i].windows_head = NULL;          // No windows yet
        workspaces[i].next = (i + 1 < DEFAULT_NUM_WORKSPACES) ? &workspaces[i + 1] : NULL;
    }
    wm_state.workspaces = workspaces;
    wm_state.active_workspace = &workspaces[0];
    wm_state.magnetic_snap_distance = 10; /* arbitrary default */
    wm_state.pip_window = NULL;
}

/* Set the layout mode for a given workspace. */
void wm_set_layout(workspace_t *ws, wm_layout_mode_t layout) {
    if (ws) {
        ws->layout = layout;
    }
}

/* Switch the active workspace to the one identified by `id`. If the ID does not
 * exist the function leaves the current workspace unchanged.
 */
void wm_switch_workspace(uint32_t id) {
    workspace_t *target = find_workspace_by_id(id);
    if (target) {
        wm_state.active_workspace = target;
    }
}

/* Return the currently active workspace (may be NULL if `wm_init` was not
 * called). This helper is useful for diagnostics and future extensions.
 */
workspace_t *wm_get_active_workspace(void) {
    return wm_state.active_workspace;
}

/* Associate a window with the active workspace. This stub simply stores the
 * window pointer as the head of the list; a full implementation would maintain
 * a proper linked list of windows. */
void wm_add_window_to_active(xdg_toplevel_t *win) {
    if (!win) return;
    wm_state.active_workspace->windows_head = win;
}

/* Remove a window from the active workspace. The stub clears the head pointer
 * if it matches the given window.
 */
void wm_remove_window_from_active(xdg_toplevel_t *win) {
    if (!win) return;
    if (wm_state.active_workspace->windows_head == win) {
        wm_state.active_workspace->windows_head = NULL;
    }
}

/* Internal helper to locate a workspace by its numeric ID. */
static workspace_t *find_workspace_by_id(uint32_t id) {
    for (workspace_t *ws = wm_state.workspaces; ws; ws = ws->next) {
        if (ws->id == id) {
            return ws;
        }
    }
    return NULL;
}

/* Handle moving a window; calculates snapping in the full version.
 * Stub does nothing – the function remains for API compatibility.
 */
void wm_handle_window_move(xdg_toplevel_t *win, int32_t x, int32_t y) {
    (void)win;
    (void)x;
    (void)y;
    /* No‑op stub */
}

/* Toggle picture‑in‑picture for a window. Stub does nothing. */
void wm_toggle_pip(xdg_toplevel_t *win) {
    (void)win;
    /* No‑op stub */
}

/* Visual transition hooks – open and close animations.
 * Stubs suppress unused‑parameter warnings.
 */
void wm_animate_window_open(xdg_toplevel_t *win) {
    (void)win;
    /* No‑op stub */
}

void wm_animate_window_close(xdg_toplevel_t *win) {
    (void)win;
    /* No‑op stub */
}
