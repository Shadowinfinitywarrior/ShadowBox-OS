#include "taskbar.h"
#include "../../userland/sys.h"

#include <stdint.h>
#include <stddef.h>
#include "../../userland/desktop_types.h"
extern char font8x8_basic[128][8];
extern uint32_t *backbuffer;
extern int mouse_x;
extern int mouse_y;
extern int mouse_btn_down;
extern int menu_open;

/* Helper functions copied from desktop.c */
static inline uint32_t blend_color(uint32_t bg, uint32_t fg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    uint32_t rb = bg & 0xFF00FF;
    uint32_t g  = bg & 0x00FF00;
    uint32_t rf = fg & 0xFF00FF;
    uint32_t gf = fg & 0x00FF00;
    rb += ((rf - rb) * alpha) >> 8;
    g  += ((gf - g ) * alpha) >> 8;
    return (rb & 0xFF00FF) | (g & 0x00FF00);
}

static void draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            backbuffer[(y + j) * SCREEN_WIDTH + (x + i)] = color;
        }
    }
}

static void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int idx = (y + j) * SCREEN_WIDTH + (x + i);
            backbuffer[idx] = blend_color(backbuffer[idx], color, alpha);
        }
    }
}

static void draw_char(int x, int y, char c, uint32_t color) {
    if ((uint8_t)c > 127) return;
    char *bitmap = font8x8_basic[(int)c];
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            if (bitmap[j] & (1 << i)) {
                if (x + i >= 0 && x + i < SCREEN_WIDTH && y + j >= 0 && y + j < SCREEN_HEIGHT) {
                    backbuffer[(y + j) * SCREEN_WIDTH + (x + i)] = color;
                }
            }
        }
    }
}

static void draw_string(int x, int y, const char *s, uint32_t color) {
    while (*s) {
        draw_char(x, y, *s, color);
        x += 8;
        s++;
    }
}

void draw_taskbar(void) {
    // Modern transparent Taskbar background
    draw_rect_alpha(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, 0x1C2833, 200);
    draw_rect_alpha(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 1, 0x34495E, 255);

    // Start button
    int start_pushed = menu_open || (mouse_btn_down && mouse_x >= 10 && mouse_x <= 90 && mouse_y >= SCREEN_HEIGHT - 35 && mouse_y <= SCREEN_HEIGHT - 5);
    draw_rect(10, SCREEN_HEIGHT - 35, 80, 30, start_pushed ? 0x2980B9 : 0x3498DB);
    draw_string(30, SCREEN_HEIGHT - 24, "Shadow", 0xFFFFFF);

    // Taskbar app tabs
    int taskbar_x = 100;
    for (int i = 0; i < num_windows; i++) {
        struct window_t *w = &windows[i];
        int bg_color = w->focused ? 0x34495E : 0x2C3E50;
        draw_rect(taskbar_x, SCREEN_HEIGHT - 35, 120, 30, bg_color);
        char short_title[12];
        int len = 0;
        while (w->title[len] && len < 10) {
            short_title[len] = w->title[len];
            len++;
        }
        short_title[len] = 0;
        if (w->title[len]) {
            short_title[8] = '.'; short_title[9] = '.'; short_title[10] = '.'; short_title[11] = 0;
        }
        draw_string(taskbar_x + 10, SCREEN_HEIGHT - 24, short_title, 0xFFFFFF);
        taskbar_x += 125;
        if (taskbar_x > SCREEN_WIDTH - 220) break;
    }

    // Clock panel (dynamic using sys_times)
    uint64_t t = sys_times(0) / 100;
    char clock_str[9];
    clock_str[0] = '0' + (t / 3600) / 10;
    clock_str[1] = '0' + (t / 3600) % 10;
    clock_str[2] = ':';
    clock_str[3] = '0' + ((t / 60) % 60) / 10;
    clock_str[4] = '0' + ((t / 60) % 60) % 10;
    clock_str[5] = ':';
    clock_str[6] = '0' + (t % 60) / 10;
    clock_str[7] = '0' + (t % 60) % 10;
    clock_str[8] = 0;
    draw_string(SCREEN_WIDTH - 90, SCREEN_HEIGHT - 24, clock_str, 0xECF0F1);
}
