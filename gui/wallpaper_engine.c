#include "wallpaper_engine.h"
#include "../userland/sys.h"
#include <stddef.h>
#include <stdint.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

static uint32_t *wallpaper_buffer = NULL;

int wallpaper_engine_init(void) {
    // Allocate buffer for wallpaper
    wallpaper_buffer = (uint32_t *)sys_sbrk(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    if ((int64_t)wallpaper_buffer < 0 || !wallpaper_buffer) {
        return -1;
    }

    // Try to load BMP from /wallpaper.bmp
    int wp_fd = sb_acquire("/wallpaper.bmp", 0);
    if (wp_fd >= 0) {
        uint8_t header[54];
        if (sb_pull(wp_fd, header, 54) == 54 && header[0] == 'B' && header[1] == 'M') {
            uint32_t offset = *(uint32_t *)&header[10];
            int w = *(int32_t *)&header[18];
            int h = *(int32_t *)&header[22];
            if (offset > 54) {
                uint8_t dummy[128];
                int to_skip = offset - 54;
                while (to_skip > 0) {
                    int chunk = to_skip > 128 ? 128 : to_skip;
                    sb_pull(wp_fd, dummy, chunk);
                    to_skip -= chunk;
                }
            }
            uint8_t row_buf[1024 * 3 + 32];
            int row_bytes = (w * 3 + 3) & ~3;
            for (int y = h - 1; y >= 0; y--) {
                sb_pull(wp_fd, row_buf, row_bytes);
                for (int x = 0; x < w; x++) {
                    uint8_t b = row_buf[x * 3];
                    uint8_t g = row_buf[x * 3 + 1];
                    uint8_t r = row_buf[x * 3 + 2];
                    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
                        wallpaper_buffer[y * SCREEN_WIDTH + x] = (r << 16) | (g << 8) | b;
                    }
                }
            }
            sb_release(wp_fd);
            return 1; // Loaded from file
        }
        sb_release(wp_fd);
    }

    // Fallback gradient if loading failed
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint32_t rb = (50 + (y * 50 / SCREEN_HEIGHT)) << 16;
        uint32_t g = (10 + (y * 30 / SCREEN_HEIGHT)) << 8;
        uint32_t b = (100 + (y * 100 / SCREEN_HEIGHT));
        uint32_t color = rb | g | b;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            wallpaper_buffer[y * SCREEN_WIDTH + x] = color;
        }
    }
    return 0; // Fallback used
}

uint32_t *wallpaper_engine_get_buffer(void) {
    return wallpaper_buffer;
}
