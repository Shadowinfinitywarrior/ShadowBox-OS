#include "desktop_icons.h"

// Functions defined in desktop.c
extern void draw_icon_folder(int x, int y, uint32_t color);
extern void draw_string(int x, int y, const char *s, uint32_t color);

// Define desktop icons with positions and colors
static const desktop_icon_t icons[] = {
    {0, "Terminal", 50, 80, 0xECF0F1},
    {1, "Files", 150, 80, 0xECF0F1},
    {2, "SysMon", 250, 80, 0xECF0F1},
    {6, "Calc", 350, 80, 0xECF0F1},
    {7, "Editor", 450, 80, 0xECF0F1},
    {8, "Paint", 550, 80, 0xECF0F1}
};

const desktop_icon_t desktop_icons[] = {
    // Exported icons (same as static icons)
    // The array is defined by copying from the static icons to satisfy the extern.
    // This is done at compile time.
    {0, "Terminal", 50, 80, 0xECF0F1},
    {1, "Files", 150, 80, 0xECF0F1},
    {2, "SysMon", 250, 80, 0xECF0F1},
    {6, "Calc", 350, 80, 0xECF0F1},
    {7, "Editor", 450, 80, 0xECF0F1},
    {8, "Paint", 550, 80, 0xECF0F1}
};

const int NUM_DESKTOP_ICONS = sizeof(desktop_icons) / sizeof(desktop_icons[0]);

void draw_desktop_icons(void) {
    for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
        const desktop_icon_t *ic = &desktop_icons[i];
        // Draw a folder icon (simple visual) and its label
        draw_icon_folder(ic->x, ic->y, ic->color);
        // Place label to the right of the icon
        draw_string(ic->x + 20, ic->y + 4, ic->title, 0xFFFFFF);
    }
}
