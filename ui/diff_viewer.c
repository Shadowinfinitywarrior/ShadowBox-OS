/* Diff Viewer component (stub implementation)
 *
 * Provides init and tick functions to fit the UI component pattern.
 * Currently a minimal stub – no UI drawing is performed.
 */

#include "diff_viewer.h"
#include "gui_toolkit.h"

/* Backbuffer and screen dimensions are defined in desktop.c */
extern uint32_t *backbuffer;
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

void diff_viewer_init(void) {
    // No initialization needed for stub implementation.
    (void)backbuffer; // suppress unused‑variable warning
}

void diff_viewer_tick(float dt) {
    // Stub tick – does nothing.
    (void)dt; // suppress unused‑parameter warning
    (void)backbuffer;
}
