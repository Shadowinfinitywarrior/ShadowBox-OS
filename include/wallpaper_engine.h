#ifndef WALLPAPER_ENGINE_H
#define WALLPAPER_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the wallpaper engine. Returns 1 if wallpaper loaded from file,
// 0 if fallback gradient generated, or -1 on allocation failure.
int wallpaper_engine_init(void);

// Get pointer to the wallpaper pixel buffer (size SCREEN_WIDTH*SCREEN_HEIGHT).
uint32_t *wallpaper_engine_get_buffer(void);

#ifdef __cplusplus
}
#endif

#endif // WALLPAPER_ENGINE_H
