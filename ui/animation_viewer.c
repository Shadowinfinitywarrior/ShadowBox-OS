/*
 * Animation Viewer component
 *
 * Implements a very simple animation that moves a rectangle horizontally
 * across the screen using the spring physics helper from animation.c.
 * The component is integrated with the desktop main loop via a tick call.
 */

#include <stdint.h>
#include "animation.h"
#include "theme.h"

/* Backbuffer lives in desktop.c; we reference it as an external symbol. */
extern uint32_t *backbuffer;
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

/* Animation state */
static float anim_pos = 0.0f;                /* current X position */
static const float target_pos = 800.0f;      /* where the rectangle wants to go */
static spring_physics_t spring = {
    .mass = 1.0f,
    .tension = 30.0f,
    .damping = 5.0f,
    .velocity = 0.0f
};

/* Helper: draw a filled rectangle directly into the backbuffer. */
static void draw_filled_rect(int x, int y, int w, int h, uint32_t color) {
    if (!backbuffer) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    for (int iy = 0; iy < h; ++iy) {
        uint32_t *row = backbuffer + (y + iy) * SCREEN_WIDTH + x;
        for (int ix = 0; ix < w; ++ix) {
            row[ix] = color;
        }
    }
}

/* Called once during desktop startup. */
void animation_viewer_init(void) {
    animation_engine_init();
    animation_engine_set_hz(60);
    anim_pos = 0.0f;
    spring.velocity = 0.0f;
}

/* Called each frame with a fixed time step (seconds). */
void animation_viewer_tick(float dt) {
    /* Update position using the spring helper. */
    anim_pos = animation_step_spring(&spring, anim_pos, target_pos, dt);
    /* Draw a magenta rectangle at the current position. */
    const int rect_w = 80;
    const int rect_h = 40;
    int rect_x = (int)anim_pos;
    int rect_y = 120; /* vertical offset */
    draw_filled_rect(rect_x, rect_y, rect_w, rect_h, ui_theme_get_accent_color());
}
