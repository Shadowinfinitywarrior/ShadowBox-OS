#ifndef DESKTOP_ICONS_H
#define DESKTOP_ICONS_H

#include <stdint.h>

typedef struct {
    int type;               // Window type to launch
    const char *title;      // Icon label
    int x;                  // X position on desktop
    int y;                  // Y position on desktop
    uint32_t color;         // Icon color
} desktop_icon_t;

extern const desktop_icon_t desktop_icons[];
extern const int NUM_DESKTOP_ICONS;

void draw_desktop_icons(void);

#endif // DESKTOP_ICONS_H
