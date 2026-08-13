#ifndef SCREENSAVER_ENGINE_H
#define SCREENSAVER_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the screensaver engine. Returns 0 on success, -1 on allocation failure.
int screensaver_engine_init(void);

// Advance the screensaver animation by dt seconds (or ticks). dt should be positive.
void screensaver_engine_tick(float dt);

// Get pointer to the screensaver pixel buffer (size SCREEN_WIDTH*SCREEN_HEIGHT).
uint32_t *screensaver_engine_get_buffer(void);

#ifdef __cplusplus
}
#endif

#endif // SCREENSAVER_ENGINE_H
