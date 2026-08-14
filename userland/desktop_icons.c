#include "desktop_icons.h"
#include "../gui/c/fb_draw.h"

#define FB_STRIDE (1024 * 4)

extern uint32_t *backbuffer;
extern void draw_string(int x, int y, const char *s, uint32_t color);

const desktop_icon_t desktop_icons[] = {
    {0, "Terminal", 50, 80, 0x2C3E50},
    {1, "Files", 150, 80, 0xF39C12},
    {2, "SysMon", 250, 80, 0x3498DB},
    {6, "Calc", 350, 80, 0x2980B9},
    {7, "Editor", 450, 80, 0x27AE60},
    {8, "Paint", 550, 80, 0xE74C3C},
};

const int NUM_DESKTOP_ICONS = sizeof(desktop_icons) / sizeof(desktop_icons[0]);

static void icon_rect(int x, int y, int w, int h, uint32_t color) {
    fb_fill_rect(backbuffer, FB_STRIDE, x, y, w, h, color);
}

static void icon_round(int x, int y, int w, int h, uint32_t color) {
    fb_fill_rect_round(backbuffer, FB_STRIDE, x, y, w, h, color, 5);
}

static void icon_terminal(int x, int y) {
    icon_rect(x, y, 32, 6, 0x2C3E50);
    draw_string(x + 3, y + 13, ">_", 0x2ECC71);
    draw_string(x + 19, y + 13, "|", 0x1ABC9C);
}

static void icon_folder(int x, int y) {
    icon_rect(x + 2, y, 12, 7, 0xE67E22);
    icon_rect(x, y + 6, 32, 22, 0xF39C12);
    icon_rect(x + 2, y + 8, 28, 18, 0xF5B041);
}

static void icon_sysmon(int x, int y) {
    icon_rect(x + 4, y + 8, 8, 3, 0xE74C3C);
    icon_rect(x + 4, y + 14, 16, 3, 0xF1C40F);
    icon_rect(x + 4, y + 20, 24, 3, 0x2ECC71);
    icon_rect(x + 4, y + 26, 26, 3, 0x95A5A6);
}

static void icon_calc(int x, int y) {
    draw_string(x + 2, y + 10, "8", 0x2C3E50);
    draw_string(x + 14, y + 10, "+", 0x2980B9);
    draw_string(x + 25, y + 10, "=", 0x2980B9);
    draw_string(x + 2, y + 22, "C", 0xE74C3C);
    draw_string(x + 14, y + 22, "9", 0x2C3E50);
}

static void icon_editor(int x, int y) {
    icon_rect(x + 3, y + 4, 26, 3, 0x3498DB);
    icon_rect(x + 3, y + 10, 26, 3, 0xBDC3C7);
    icon_rect(x + 3, y + 16, 26, 3, 0xBDC3C7);
    icon_rect(x + 3, y + 22, 18, 3, 0xBDC3C7);
}

static void icon_paint(int x, int y) {
    uint32_t cols[4] = {0xE74C3C, 0xF1C40F, 0x2ECC71, 0x3498DB};
    for (int i = 0; i < 4; i++) {
        icon_rect(x + 3 + (i % 2) * 15, y + 4 + (i / 2) * 15, 12, 12, cols[i]);
    }
    icon_rect(x + 9, y + 26, 14, 4, 0x9B59B6);
}

void draw_icon_folder(int x, int y, uint32_t color) {
    (void)color;
    icon_folder(x, y);
}

void draw_desktop_icons(void) {
    for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
        const desktop_icon_t *ic = &desktop_icons[i];
        icon_round(ic->x + 2, ic->y + 2, 32, 32, 0x000000);
        icon_round(ic->x, ic->y, 32, 32, 0x34495E);
        switch (ic->type) {
        case 0: icon_terminal(ic->x + 1, ic->y + 1); break;
        case 1: icon_folder(ic->x + 1, ic->y + 1); break;
        case 2: icon_sysmon(ic->x + 1, ic->y + 1); break;
        case 6: icon_calc(ic->x + 1, ic->y + 1); break;
        case 7: icon_editor(ic->x + 1, ic->y + 1); break;
        case 8: icon_paint(ic->x + 1, ic->y + 1); break;
        default: break;
        }
        draw_string(ic->x + 1, ic->y + 38, ic->title, 0x000000);
        draw_string(ic->x, ic->y + 37, ic->title, 0xFFFFFF);
    }
}
