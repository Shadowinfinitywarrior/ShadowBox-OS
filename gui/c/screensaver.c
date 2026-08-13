// Screensaver Engine implementation for ShadowBox OS.
// Provides a simple bouncing ball screensaver.

#include "screensaver_engine.h"

#include <stddef.h>
#include <stdint.h>

// Use same dimensions as wallpaper.
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

static uint32_t ss_buffer_data[SCREEN_WIDTH * SCREEN_HEIGHT];
static uint32_t *ss_buffer = NULL; static uint32_t ss_buffer_static[SCREEN_WIDTH * SCREEN_HEIGHT];

// Simple ball state
typedef struct {
    float x, y;      // Position (pixel coordinates)
    float vx, vy;    // Velocity (pixels per tick)
    int size;        // Square size of ball
    uint32_t color;  // Ball color (ARGB without alpha)
} ball_t;

static ball_t ball = { .x = SCREEN_WIDTH/2, .y = SCREEN_HEIGHT/2, .vx = 2.0f, .vy = 2.5f, .size = 16, .color = 0xFF00FF00 };

int screensaver_engine_init(void) {
    // Allocate buffer for screensaver framebuffer.
    ss_buffer = ss_buffer_data;
    // Initialise buffer to black.
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        ss_buffer[i] = 0x00000000;
    }
    return 0;
}

static void ss_draw_ball(void) {
    // Draw a simple filled square representing the ball.
    int ix = (int)ball.x;
    int iy = (int)ball.y;
    int sz = ball.size;
    for (int dy = 0; dy < sz; ++dy) {
        int y = iy + dy;
        if (y < 0 || y >= SCREEN_HEIGHT) continue;
        for (int dx = 0; dx < sz; ++dx) {
            int x = ix + dx;
            if (x < 0 || x >= SCREEN_WIDTH) continue;
            ss_buffer[y * SCREEN_WIDTH + x] = ball.color;
        }
    }
}

static void ss_clear(void) {
    // Fill buffer with black.
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        ss_buffer[i] = 0x00000000;
    }
}

void screensaver_engine_tick(float dt) {
    if (!ss_buffer) return;
    // Update ball position.
    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    // Bounce off screen edges.
    if (ball.x < 0) { ball.x = 0; ball.vx = -ball.vx; }
    if (ball.y < 0) { ball.y = 0; ball.vy = -ball.vy; }
    if (ball.x + ball.size > SCREEN_WIDTH) { ball.x = SCREEN_WIDTH - ball.size; ball.vx = -ball.vx; }
    if (ball.y + ball.size > SCREEN_HEIGHT) { ball.y = SCREEN_HEIGHT - ball.size; ball.vy = -ball.vy; }

    ss_clear();
    ss_draw_ball();
}

uint32_t *screensaver_engine_get_buffer(void) {
    return ss_buffer;
}
