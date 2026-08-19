#ifndef DESKTOP_ICONS_H
#define DESKTOP_ICONS_H

#include <stdint.h>

typedef struct {
    int type;               // Window type to launch
    const char *title;      // Icon label
    int x;                  // X position on desktop
    int y;                  // Y position on desktop
    uint32_t color;         // Icon color
    const char *icon;       // /icons/<name>.bmp path, NULL for procedural
} desktop_icon_t;

extern const desktop_icon_t desktop_icons[];
extern const int NUM_DESKTOP_ICONS;

void draw_desktop_icons(void);

/* Generic 32x32 BMP icon helpers shared with the start menu.
 * icon_bmp_load parses a /icons/<name>.bmp into `buf` (32x32 32-bit pixels,
 * top-down); returns 1 on success. icon_bmp_blit draws such a buffer onto the
 * backbuffer, skipping pixels that match the tile background colour. */
int  icon_bmp_load(const char *path, uint32_t *buf);
void icon_bmp_blit(const uint32_t *buf, int x, int y);

#endif // DESKTOP_ICONS_H
