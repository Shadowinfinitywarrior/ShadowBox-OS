#include "desktop_icons.h"
#include "../gui/c/fb_draw.h"
#include "sys.h"

#define FB_STRIDE (1024 * 4)
#define ICON_SIZE 32
#define TILE_BG 0x34495E
#define MAX_ICONS 12

extern uint32_t *backbuffer;
extern void draw_string(int x, int y, const char *s, uint32_t color);

const desktop_icon_t desktop_icons[] = {
    {0, "Terminal", 50, 80, 0x2C3E50, "/icons/terminal.bmp"},
    {1, "Files", 150, 80, 0xF39C12, "/icons/file_explorer.bmp"},
    {2, "SysMon", 250, 80, 0x3498DB, "/icons/system_monitor.bmp"},
    {6, "Calc", 350, 80, 0x2980B9, "/icons/calculator.bmp"},
    {7, "Editor", 450, 80, 0x27AE60, "/icons/text_editor.bmp"},
    {8, "Paint", 550, 80, 0xE74C3C, "/icons/paint.bmp"},
    {9, "Settings", 650, 80, 0x9B59B6, "/icons/settings.bmp"},
    {18, "Browser", 50, 140, 0x3498DB, "/icons/browser.bmp"},
    {10, "Snake", 150, 140, 0x27AE60, "/icons/snake.bmp"},
    {11, "Tetris", 250, 140, 0xE67E22, "/icons/tetris.bmp"},
    {12, "Clock", 350, 140, 0x3498DB, "/icons/clock.bmp"},
};

const int NUM_DESKTOP_ICONS = sizeof(desktop_icons) / sizeof(desktop_icons[0]);

static uint32_t icon_buf[MAX_ICONS][ICON_SIZE * ICON_SIZE];
static int icon_ready[MAX_ICONS];

static void icon_rect(int x, int y, int w, int h, uint32_t color) {
    fb_fill_rect(backbuffer, FB_STRIDE, x, y, w, h, color);
}

static void icon_round(int x, int y, int w, int h, uint32_t color) {
    fb_fill_rect_round(backbuffer, FB_STRIDE, x, y, w, h, color, 5);
}

/* Load a 24-bit bottom-up BMP (54-byte header, the same layout wallpaper uses)
 * into `buf` as 32x32 32-bit pixels, top-down. Returns 1 on success. */
int icon_bmp_load(const char *path, uint32_t *buf) {
    if (!path || !buf) return 0;
    int fd = sb_acquire(path, 0);
    if (fd < 0) return 0;

    uint8_t header[54];
    if (sb_pull(fd, header, 54) != 54 || header[0] != 'B' || header[1] != 'M') {
        sb_release(fd);
        return 0;
    }
    uint32_t offset = *(uint32_t *)&header[10];
    int w = *(int32_t *)&header[18];
    int h = *(int32_t *)&header[22];
    if (w != ICON_SIZE || h != ICON_SIZE) {
        sb_release(fd);
        return 0;
    }
    if (offset > 54) {
        uint8_t dummy[128];
        int to_skip = offset - 54;
        while (to_skip > 0) {
            int chunk = to_skip > 128 ? 128 : to_skip;
            sb_pull(fd, dummy, chunk);
            to_skip -= chunk;
        }
    }
    uint8_t row_buf[ICON_SIZE * 3 + 32];
    int row_bytes = (w * 3 + 3) & ~3;
    for (int y = h - 1; y >= 0; y--) {
        if (sb_pull(fd, row_buf, row_bytes) != (uint64_t)row_bytes) break;
        for (int x = 0; x < w; x++) {
            uint8_t b = row_buf[x * 3];
            uint8_t g = row_buf[x * 3 + 1];
            uint8_t r = row_buf[x * 3 + 2];
            buf[y * ICON_SIZE + x] = (r << 16) | (g << 8) | b;
        }
    }
    sb_release(fd);
    return 1;
}

/* Blit a loaded 32x32 icon onto the backbuffer at (x, y), skipping pixels that
 * match the tile background so the rounded tile stays visible. */
void icon_bmp_blit(const uint32_t *buf, int x, int y) {
    if (!buf) return;
    for (int py = 0; py < ICON_SIZE; py++) {
        int dy = y + py;
        if (dy < 0 || dy >= 1024) continue;
        for (int px = 0; px < ICON_SIZE; px++) {
            uint32_t c = buf[py * ICON_SIZE + px];
            if (c == TILE_BG) continue;
            int dx = x + px;
            if (dx < 0 || dx >= 1024) continue;
            backbuffer[dy * 1024 + dx] = c;
        }
    }
}

/* Load a 24-bit bottom-up BMP into icon_buf[idx]. Returns 1 on success. */
static int load_icon_bmp(int idx, const char *path) {
    if (idx >= MAX_ICONS || !path) return 0;
    if (!icon_bmp_load(path, icon_buf[idx])) return 0;
    icon_ready[idx] = 1;
    return 1;
}

/* Blit a cached desktop icon, skipping pixels that match the tile background so
 * the rounded tile and desktop wallpaper stay visible around transparent regions. */
static void blit_icon(int idx, int x, int y) {
    if (!icon_ready[idx]) return;
    icon_bmp_blit(icon_buf[idx], x, y);
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

static void icon_browser(int x, int y) {
    icon_rect(x + 1, y + 1, 30, 30, 0x1B2631);
    icon_rect(x + 6, y + 8, 20, 16, 0x3498DB);
    icon_rect(x + 8, y + 10, 16, 3, 0xFFFFFF);
    icon_rect(x + 8, y + 14, 16, 2, 0xBDC3C7);
    icon_rect(x + 8, y + 17, 16, 2, 0xBDC3C7);
    icon_rect(x + 8, y + 20, 10, 2, 0xBDC3C7);
}

static void icon_settings(int x, int y) {
    icon_rect(x + 8, y + 8, 16, 16, 0x95A5A6);
    icon_rect(x + 10, y + 10, 12, 12, 0x7F8C8D);
    icon_rect(x + 14, y + 6, 4, 4, 0x3498DB);
    icon_rect(x + 14, y + 22, 4, 4, 0x3498DB);
    icon_rect(x + 6, y + 14, 4, 4, 0x3498DB);
    icon_rect(x + 22, y + 14, 4, 4, 0x3498DB);
}

static void icon_snake(int x, int y) {
    icon_rect(x + 4, y + 4, 24, 24, 0x27AE60);
    icon_rect(x + 8, y + 8, 4, 4, 0x2ECC71);
    icon_rect(x + 12, y + 8, 4, 4, 0x2ECC71);
    icon_rect(x + 16, y + 12, 4, 4, 0x2ECC71);
    icon_rect(x + 12, y + 16, 4, 4, 0xE74C3C);
}

static void icon_tetris(int x, int y) {
    icon_rect(x + 2, y + 2, 28, 28, 0x2C3E50);
    icon_rect(x + 4, y + 4, 8, 8, 0xE67E22);
    icon_rect(x + 12, y + 4, 8, 8, 0x3498DB);
    icon_rect(x + 20, y + 4, 8, 8, 0x9B59B6);
    icon_rect(x + 4, y + 12, 8, 8, 0x2ECC71);
    icon_rect(x + 12, y + 12, 8, 8, 0xE74C3C);
    icon_rect(x + 20, y + 12, 8, 8, 0xF1C40F);
}

static void icon_clock(int x, int y) {
    icon_round(x + 4, y + 4, 24, 24, 0xECF0F1);
    icon_rect(x + 15, y + 8, 2, 10, 0x2C3E50);
    icon_rect(x + 15, y + 15, 6, 2, 0x2C3E50);
    icon_rect(x + 15, y + 15, 2, 2, 0xE74C3C);
}

static void draw_procedural(int type, int x, int y) {
    switch (type) {
    case 0: icon_terminal(x + 1, y + 1); break;
    case 1: icon_folder(x + 1, y + 1); break;
    case 2: icon_sysmon(x + 1, y + 1); break;
    case 6: icon_calc(x + 1, y + 1); break;
    case 7: icon_editor(x + 1, y + 1); break;
    case 8: icon_paint(x + 1, y + 1); break;
    case 9: icon_settings(x + 1, y + 1); break;
    case 10: icon_snake(x + 1, y + 1); break;
    case 11: icon_tetris(x + 1, y + 1); break;
    case 12: icon_clock(x + 1, y + 1); break;
    case 18: icon_browser(x + 1, y + 1); break;
    default: break;
    }
}

void draw_icon_folder(int x, int y, uint32_t color) {
    (void)color;
    icon_folder(x, y);
}

void draw_desktop_icons(void) {
    for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
        const desktop_icon_t *ic = &desktop_icons[i];
        if (ic->icon && !icon_ready[i]) {
            load_icon_bmp(i, ic->icon);
        }
        icon_round(ic->x + 2, ic->y + 2, 32, 32, 0x000000);
        icon_round(ic->x, ic->y, 32, 32, 0x34495E);
        if (icon_ready[i]) {
            blit_icon(i, ic->x, ic->y);
        } else {
            draw_procedural(ic->type, ic->x, ic->y);
        }
        draw_string(ic->x + 1, ic->y + 38, ic->title, 0x000000);
        draw_string(ic->x, ic->y + 37, ic->title, 0xFFFFFF);
    }
}
