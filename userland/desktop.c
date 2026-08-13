#include "sys.h"
#include "font8x8.h"
#include "fcntl.h"

typedef struct {
    uint8_t type;
    uint8_t code;
    int16_t x;
    int16_t y;
    uint16_t reserved;
} input_event_t;

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define PITCH (SCREEN_WIDTH * 4)
#define TITLE_H 24
#define MAX_WINDOWS 16

static inline void strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

static inline void strcat(char *dst, const char *src) {
    while (*dst) dst++;
    while ((*dst++ = *src++));
}

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static inline void memset(void *d, int c, uint64_t n) {
    uint8_t *p = (uint8_t *)d;
    while (n--) *p++ = (uint8_t)c;
}

static inline void memcpy(void *d, const void *s, uint64_t n) {
    uint8_t *dp = (uint8_t *)d;
    const uint8_t *sp = (const uint8_t *)s;
    while (n--) *dp++ = *sp++;
}

#define WTYPE_TERMINAL  0
#define WTYPE_FILE_BRO  1
#define WTYPE_SYS_MON   2
#define WTYPE_ABOUT     3
#define WTYPE_VIEWER    4
#define WTYPE_SNAKE     5
#define WTYPE_CALC      6
#define WTYPE_EDITOR    7
#define WTYPE_PAINT     8
#define WTYPE_PROCMON   9
#define WTYPE_HEXVIEW   10
#define WTYPE_TETRIS    11
#define WTYPE_G2048     12
#define WTYPE_MANDEL    13
#define WTYPE_CLOCK     14
#define WTYPE_FORTUNE   15
#define WTYPE_PONG      16
#define WTYPE_MATRIX    17
#define WTYPE_MEMVIEW   25

typedef struct {
    int id;
    int active;
    int type;
    int x, y, w, h;
    char title[64];
    uint32_t bg_color;

    int minimized;
    int maximized;
    int prev_x, prev_y, prev_w, prev_h;
    int resizing;

    char text[24 * 60];
    int cursor_x, cursor_y;
    char file_path[256];

    struct dirent entries[32];
    int num_entries;
    char current_dir[128];

    uint64_t last_update;

    int snake_x[64];
    int snake_y[64];
    int snake_len;
    int snake_dir;
    int food_x, food_y;
    int snake_dead;

    char calc_disp[24];
    int64_t calc_acc;
    char calc_op;
    int calc_fresh;

    uint32_t *paint_canvas;
    int painting;
    uint32_t paint_color;
    int brush_size;

    signed char tetris_board[10 * 22];
    int tetris_px, tetris_py;
    int tetris_type, tetris_next, tetris_rot;
    int tetris_score, tetris_lines, tetris_over;

    int g2048[16];
    int g2048_score;
    int g2048_over;

    int pong_py, pong_ai;
    int pong_bx, pong_by, pong_vx, pong_vy;
    int pong_s1, pong_s2, pong_over;
    int pong_up, pong_down;

    int hex_fd;
    int hex_offset;
    uint8_t *hex_data;
    int hex_size;

    int proc_scroll;

    int matrix_off[40];
    int matrix_speed[40];
    int matrix_char[40];

    uint32_t *mandel_buf;
    int mandel_ready;

    int fortune_idx;

    char editor_msg[32];
    uint64_t editor_msg_at;

    int tab_state;
} window_t;

static window_t windows[MAX_WINDOWS];
static int num_windows = 0;

static uint32_t *fb = (uint32_t *)0x78000000ULL;
static uint32_t *backbuffer = NULL;
static uint32_t *wallpaper_buffer = NULL;
static uint32_t *logo_buffer = NULL;

static int mouse_x = SCREEN_WIDTH / 2;
static int mouse_y = SCREEN_HEIGHT / 2;
static int mouse_btn_down = 0;
static int drag_win = -1;
static int resize_win = -1;
static int drag_off_x = 0;
static int drag_off_y = 0;
static int menu_open = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static int painting_win = -1;

static uint32_t rng_state = 0x12345678;
static uint32_t rnd(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

static const char KSC_UP = 0x48;
static const char KSC_DOWN = 0x50;
static const char KSC_LEFT = 0x4B;
static const char KSC_RIGHT = 0x4D;
static const char KSC_HOME = 0x47;
static const char KSC_END = 0x4F;
static const char KSC_PGUP = 0x49;
static const char KSC_PGDN = 0x51;
static const char KSC_DEL = 0x53;
static const char KSC_CTRL = 0x1D;
static const char KSC_ALT = 0x38;

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
    if (w <= 0 || h <= 0) return;
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
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int idx = (y + j) * SCREEN_WIDTH + (x + i);
            backbuffer[idx] = blend_color(backbuffer[idx], color, alpha);
        }
    }
}

static void draw_char(int x, int y, char c, uint32_t color) {
    uint8_t uc = (uint8_t)c;
    if (uc > 127) return;
    char *bitmap = font8x8_basic[uc];
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

static void draw_string_limit(int x, int y, const char *s, int max_chars, uint32_t color) {
    int n = 0;
    while (*s && n < max_chars) {
        draw_char(x, y, *s, color);
        x += 8;
        s++;
        n++;
    }
}

static void draw_number(int x, int y, uint64_t num, uint32_t color) {
    char buf[32];
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0 && i < 30) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    buf[i] = 0;
    for (int j = 0; j < i / 2; j++) {
        char t = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = t;
    }
    draw_string(x, y, buf, color);
}

static void num_to_str(int64_t v, char *buf) {
    char tmp[24];
    int i = 0;
    int neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    if (v == 0) tmp[i++] = '0';
    while (v > 0 && i < 22) { tmp[i++] = '0' + (int)(v % 10); v /= 10; }
    int n = 0;
    if (neg) buf[n++] = '-';
    while (i > 0) buf[n++] = tmp[--i];
    buf[n] = 0;
}

static int64_t atoi64(const char *s) {
    int64_t v = 0;
    int neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return neg ? -v : v;
}

static void hex_to_str(uint32_t v, char *buf) {
    const char *dig = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) { buf[i] = dig[v & 0xF]; v >>= 4; }
    buf[8] = 0;
}

static int isqrt(int v) {
    int r = 0;
    int b = 0x4000;
    while (b) {
        int t = r + b;
        if (t * t <= v) r = t;
        b >>= 1;
    }
    return r;
}

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        if (x0 >= 0 && x0 < SCREEN_WIDTH && y0 >= 0 && y0 < SCREEN_HEIGHT)
            backbuffer[y0 * SCREEN_WIDTH + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

static void draw_circle(int cx, int cy, int r, uint32_t color, int fill) {
    for (int dy = -r; dy <= r; dy++) {
        int span = isqrt(r * r - dy * dy);
        if (fill) {
            draw_rect(cx - span, cy + dy, span * 2 + 1, 1, color);
        } else {
            backbuffer[(cy + dy) * SCREEN_WIDTH + (cx - span)] = color;
            backbuffer[(cy + dy) * SCREEN_WIDTH + (cx + span)] = color;
        }
    }
}

static const int SIN_TAB[16] = {0, 195, 383, 556, 707, 831, 924, 981, 1000, 981, 924, 831, 707, 556, 383, 195};

static void sincos(int deg, int *s, int *c) {
    deg = deg % 360;
    if (deg < 0) deg += 360;
    int sign_s = 1, sign_c = 1;
    if (deg >= 180) { sign_s = -1; deg -= 180; }
    if (deg >= 90) { sign_c = -1; deg = 180 - deg; }
    int i = deg / 6;
    int rem = deg - i * 6;
    if (i >= 15) { i = 15; rem = 0; }
    int v = SIN_TAB[i] + ((SIN_TAB[i + 1] - SIN_TAB[i]) * rem) / 6;
    int j = (90 - deg) / 6;
    if (j < 0) j = 0;
    if (j > 15) j = 15;
    *s = v * sign_s;
    *c = SIN_TAB[j] * sign_c;
}

static void draw_drop_shadow(int x, int y, int w, int h) {
    int shadow_size = 8;
    for (int s = 0; s < shadow_size; s++) {
        uint8_t alpha = 40 - (s * 4);
        draw_rect_alpha(x + w + s, y + s, 1, h, 0x000000, alpha);
        draw_rect_alpha(x + s, y + h + s, w, 1, 0x000000, alpha);
    }
    for (int sy = 0; sy < shadow_size; sy++) {
        for (int sx = 0; sx < shadow_size; sx++) {
            uint8_t alpha = 40 - ((sx + sy) * 3);
            if (alpha > 40) alpha = 0;
            draw_rect_alpha(x + w + sx, y + h + sy, 1, 1, 0x000000, alpha);
        }
    }
}

static void draw_cursor(int x, int y) {
    for (int oy = 0; oy < 18; oy++) {
        for (int ox = 0; ox < 18; ox++) {
            int in = 0;
            if (ox <= oy && oy < 11) in = 1;
            if (oy >= 11 && oy <= 16 && ox >= 2 && ox <= 5 && ox <= oy - 8) in = 1;
            if (!in) continue;
            int px = x + ox - 1;
            int py = y + oy - 1;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                backbuffer[py * SCREEN_WIDTH + px] = 0x000000;
            }
        }
    }
    for (int oy = 0; oy < 18; oy++) {
        for (int ox = 0; ox < 18; ox++) {
            int in = 0;
            if (ox <= oy && oy < 11) in = 1;
            if (oy >= 11 && oy <= 16 && ox >= 2 && ox <= 5 && ox <= oy - 8) in = 1;
            if (!in) continue;
            int px = x + ox;
            int py = y + oy;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                backbuffer[py * SCREEN_WIDTH + px] = 0xFFFFFF;
            }
        }
    }
}

static void draw_button(int x, int y, int w, int h, const char *label, uint32_t bg, uint32_t fg) {
    draw_rect(x, y, w, h, bg);
    draw_rect(x, y, w, 1, 0xFFFFFF);
    draw_rect(x, y + h - 1, w, 1, 0x000000);
    draw_rect(x, y, 1, h, 0xFFFFFF);
    draw_rect(x + w - 1, y, 1, h, 0x000000);
    int tw = 0;
    while (label[tw]) tw++;
    tw *= 8;
    draw_string(x + (w - tw) / 2, y + (h - 8) / 2, label, fg);
}

static int top_window(void) {
    return num_windows - 1;
}

static void raise_window(int idx) {
    if (idx < 0 || idx >= num_windows) return;
    window_t temp = windows[idx];
    for (int j = idx; j < num_windows - 1; j++) windows[j] = windows[j + 1];
    windows[num_windows - 1] = temp;
}

static void close_window(int idx) {
    if (idx < 0 || idx >= num_windows) return;
    for (int j = idx; j < num_windows - 1; j++) windows[j] = windows[j + 1];
    num_windows--;
    if (painting_win == idx) painting_win = -1;
}

static void toggle_maximize(window_t *w) {
    if (w->maximized) {
        w->x = w->prev_x; w->y = w->prev_y;
        w->w = w->prev_w; w->h = w->prev_h;
        w->maximized = 0;
    } else {
        w->prev_x = w->x; w->prev_y = w->y;
        w->prev_w = w->w; w->prev_h = w->h;
        w->x = 0; w->y = 0;
        w->w = SCREEN_WIDTH; w->h = SCREEN_HEIGHT - 40;
        w->maximized = 1;
    }
}

static int in_rect(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx < x + w && my >= y && my < y + h;
}

typedef struct {
    const char *name;
    int type;
    int x, y, w, h;
} app_entry_t;

static const app_entry_t apps[] = {
    {"Terminal", WTYPE_TERMINAL, 150, 150, 560, 360},
    {"Files", WTYPE_FILE_BRO, 100, 100, 330, 330},
    {"System Monitor", WTYPE_SYS_MON, 200, 150, 370, 220},
    {"About", WTYPE_ABOUT, 250, 200, 320, 180},
    {"Image Viewer", WTYPE_VIEWER, 200, 150, 340, 340},
    {"Snake", WTYPE_SNAKE, 260, 150, 324, 344},
    {"Calculator", WTYPE_CALC, 350, 120, 250, 340},
    {"Editor", WTYPE_EDITOR, 150, 100, 650, 430},
    {"Paint", WTYPE_PAINT, 200, 100, 530, 410},
    {"Processes", WTYPE_PROCMON, 250, 100, 450, 330},
    {"Hex Viewer", WTYPE_HEXVIEW, 200, 100, 520, 410},
    {"Tetris", WTYPE_TETRIS, 320, 80, 270, 500},
    {"2048", WTYPE_G2048, 300, 120, 320, 390},
    {"Mandelbrot", WTYPE_MANDEL, 200, 100, 500, 420},
    {"Clock", WTYPE_CLOCK, 420, 200, 230, 240},
    {"Fortune", WTYPE_FORTUNE, 300, 200, 430, 250},
    {"Pong", WTYPE_PONG, 280, 120, 530, 330},
    {"Matrix", WTYPE_MATRIX, 300, 100, 340, 410},
    {"Memory", WTYPE_MEMVIEW, 250, 150, 400, 270},
    {"Shutdown", -1, 0, 0, 0, 0},
};

#define NUM_APPS ((int)(sizeof(apps) / sizeof(apps[0])))

static const int MENU_MX = 10;
static const int MENU_ROW_H = 28;
static const int MENU_COL_W = 200;
static const int MENU_COL_GAP = 8;
static const int MENU_COLS = 2;
static const int MENU_ROWS = (NUM_APPS + 1) / MENU_COLS;
static const int MENU_MW = MENU_COLS * MENU_COL_W + (MENU_COLS - 1) * MENU_COL_GAP;
static const int MENU_MH = 40 + MENU_ROWS * MENU_ROW_H;
static const int MENU_MY = SCREEN_HEIGHT - 40 - MENU_MH;

static int menu_item_at(void) {
    if (!in_rect(mouse_x, mouse_y, MENU_MX, MENU_MY, MENU_MW, MENU_MH)) return -1;
    if (mouse_y < MENU_MY + 40) return -1;
    int row = (mouse_y - (MENU_MY + 40)) / MENU_ROW_H;
    int col = -1;
    if (mouse_x >= MENU_MX && mouse_x < MENU_MX + MENU_COL_W) col = 0;
    else if (mouse_x >= MENU_MX + MENU_COL_W + MENU_COL_GAP &&
             mouse_x < MENU_MX + MENU_COL_W + MENU_COL_GAP + MENU_COL_W) col = 1;
    if (col < 0 || row < 0 || row >= MENU_ROWS) return -1;
    int idx = row * MENU_COLS + col;
    if (idx >= NUM_APPS) return -1;
    return idx;
}

static void text_put(window_t *w, char c) {
    if (w->cursor_x < 60) {
        w->text[w->cursor_y * 60 + w->cursor_x] = c;
        w->cursor_x++;
    } else {
        w->cursor_x = 0;
        if (w->cursor_y < 23) w->cursor_y++;
    }
}

static void text_newline(window_t *w) {
    w->cursor_x = 0;
    if (w->cursor_y < 23) w->cursor_y++;
}

static void text_backspace(window_t *w) {
    if (w->cursor_x > 0) {
        w->cursor_x--;
        w->text[w->cursor_y * 60 + w->cursor_x] = 0;
    } else if (w->cursor_y > 0) {
        w->cursor_y--;
        int end = 59;
        while (end >= 0 && w->text[w->cursor_y * 60 + end] == 0) end--;
        w->cursor_x = end + 1;
    }
}

static void text_delete(window_t *w) {
    int idx = w->cursor_y * 60 + w->cursor_x;
    int row = w->cursor_y;
    int col = w->cursor_x;
    if (col >= 59 || w->text[idx] == 0) return;
    for (int i = col; i < 59; i++) {
        w->text[row * 60 + i] = w->text[row * 60 + i + 1];
    }
    w->text[row * 60 + 59] = 0;
}

static void editor_insert_char(window_t *w, char c) {
    int row = w->cursor_y;
    int col = w->cursor_x;
    if (col >= 59) { col = 59; }
    for (int i = 58; i >= col; i--) {
        w->text[row * 60 + i + 1] = w->text[row * 60 + i];
    }
    w->text[row * 60 + col] = c;
    if (w->cursor_x < 60) w->cursor_x++;
}

static void editor_newline(window_t *w) {
    int row = w->cursor_y;
    int col = w->cursor_x;
    if (row >= 23) return;
    for (int r = 22; r > row; r--) {
        for (int i = 0; i < 60; i++) {
            w->text[(r + 1) * 60 + i] = w->text[r * 60 + i];
        }
    }
    for (int i = 0; i < 60; i++) w->text[(row + 1) * 60 + i] = 0;
    for (int i = col; i < 60; i++) {
        w->text[(row + 1) * 60 + (i - col)] = w->text[row * 60 + i];
        w->text[row * 60 + i] = 0;
    }
    w->cursor_y++;
    w->cursor_x = 0;
}

static int line_len(window_t *w, int row) {
    int len = 0;
    while (len < 60 && w->text[row * 60 + len]) len++;
    return len;
}

static void editor_save(window_t *w) {
    int fd = sb_acquire(w->file_path, 0x40 | 0x1 | 0x200);
    if (fd < 0) {
        strcpy(w->editor_msg, "Save failed!");
        w->editor_msg_at = sys_times(0);
        return;
    }
    char line[62];
    for (int r = 0; r < 24; r++) {
        int len = line_len(w, r);
        for (int i = 0; i < len; i++) line[i] = w->text[r * 60 + i];
        line[len] = '\n';
        sb_push(fd, line, len + 1);
    }
    sb_release(fd);
    strcpy(w->editor_msg, "Saved to ");
    strcat(w->editor_msg, w->file_path);
    w->editor_msg_at = sys_times(0);
}

static void editor_load(window_t *w) {
    for (int i = 0; i < 24 * 60; i++) w->text[i] = 0;
    int fd = sb_acquire(w->file_path, 0);
    if (fd < 0) return;
    uint8_t chunk[512];
    int row = 0, col = 0;
    int total = 0;
    while (1) {
        int n = (int)sb_pull(fd, chunk, 512);
        if (n <= 0) break;
        for (int i = 0; i < n; i++) {
            if (chunk[i] == '\n') {
                row++; col = 0;
                if (row >= 24) { total = 1; break; }
            } else if (chunk[i] >= 32 && chunk[i] < 127) {
                if (col < 60) {
                    w->text[row * 60 + col] = (char)chunk[i];
                    col++;
                }
            }
        }
        if (total) break;
    }
    sb_release(fd);
    w->cursor_y = row;
    w->cursor_x = line_len(w, row);
}

static void terminal_init(window_t *w) {
    for (int i = 0; i < 24 * 60; i++) w->text[i] = 0;
    const char *welcome = "ShadowBox Terminal  |  Ctrl+Alt+T  (apps)\n";
    int idx = 0;
    int row = 0, col = 0;
    while (welcome[idx]) {
        if (welcome[idx] == '\n') { row++; col = 0; }
        else if (col < 60) { w->text[row * 60 + col] = welcome[idx]; col++; }
        idx++;
    }
    const char *prompt = "root@shadowbox:~# ";
    int j = 0;
    while (prompt[j]) { w->text[row * 60 + col] = prompt[j]; col++; j++; }
    w->cursor_y = row;
    w->cursor_x = col;
}

static void filebrowser_refresh(window_t *w) {
    w->num_entries = 0;
    strcpy(w->entries[w->num_entries].name, "..");
    w->num_entries++;
    int fd = sb_acquire(w->current_dir, 0);
    if (fd >= 0) {
        struct dirent d;
        while (sys_getdents(fd, &d, 1) == 1 && w->num_entries < 31) {
            if (strcmp(d.name, ".") == 0 || strcmp(d.name, "..") == 0) continue;
            strcpy(w->entries[w->num_entries].name, d.name);
            w->num_entries++;
        }
        sb_release(fd);
    }
}

static int is_dir_name(const char *n) {
    if (n[0] == '.') return 1;
    for (int k = 0; n[k]; k++) {
        if (n[k] == '.') return 0;
    }
    return 1;
}

static const signed char TET_I[4][4][2] = {
    {{0,1},{1,1},{2,1},{3,1}},
    {{2,0},{2,1},{2,2},{2,3}},
    {{0,2},{1,2},{2,2},{3,2}},
    {{1,0},{1,1},{1,2},{1,3}},
};
static const signed char TET_O[1][4][2] = {
    {{1,0},{2,0},{1,1},{2,1}},
};
static const signed char TET_T[4][4][2] = {
    {{1,0},{0,1},{1,1},{2,1}},
    {{1,0},{1,1},{2,1},{1,2}},
    {{0,1},{1,1},{2,1},{1,2}},
    {{1,0},{0,1},{1,1},{1,2}},
};
static const signed char TET_S[4][4][2] = {
    {{1,0},{2,0},{0,1},{1,1}},
    {{1,0},{1,1},{2,1},{2,2}},
    {{1,1},{2,1},{0,2},{1,2}},
    {{0,0},{0,1},{1,1},{1,2}},
};
static const signed char TET_Z[4][4][2] = {
    {{0,0},{1,0},{1,1},{2,1}},
    {{2,0},{1,1},{2,1},{1,2}},
    {{0,1},{1,1},{1,2},{2,2}},
    {{1,0},{0,1},{1,1},{0,2}},
};
static const signed char TET_J[4][4][2] = {
    {{0,0},{0,1},{1,1},{2,1}},
    {{1,0},{2,0},{1,1},{1,2}},
    {{0,1},{1,1},{2,1},{2,2}},
    {{1,0},{1,1},{0,2},{1,2}},
};
static const signed char TET_L[4][4][2] = {
    {{2,0},{0,1},{1,1},{2,1}},
    {{1,0},{1,1},{1,2},{2,2}},
    {{0,1},{1,1},{2,1},{0,2}},
    {{0,0},{1,0},{1,1},{1,2}},
};

static int tetris_collides(window_t *w, int type, int rot, int px, int py) {
    const signed char (*cells)[2];
    if (type == 1) cells = TET_O[0];
    else if (type == 2) cells = TET_T[rot];
    else if (type == 3) cells = TET_S[rot];
    else if (type == 4) cells = TET_Z[rot];
    else if (type == 5) cells = TET_J[rot];
    else if (type == 6) cells = TET_L[rot];
    else cells = TET_I[rot];
    for (int i = 0; i < 4; i++) {
        int bx = px + cells[i][0];
        int by = py + cells[i][1];
        if (bx < 0 || bx >= 10 || by < 0 || by >= 22) return 1;
        if (by < 22 && w->tetris_board[by * 10 + bx] != 0) return 1;
    }
    return 0;
}

static void tetris_lock(window_t *w) {
    const signed char (*cells)[2];
    int type = w->tetris_type;
    int rot = w->tetris_rot;
    if (type == 1) cells = TET_O[0];
    else if (type == 2) cells = TET_T[rot];
    else if (type == 3) cells = TET_S[rot];
    else if (type == 4) cells = TET_Z[rot];
    else if (type == 5) cells = TET_J[rot];
    else if (type == 6) cells = TET_L[rot];
    else cells = TET_I[rot];
    for (int i = 0; i < 4; i++) {
        int bx = w->tetris_px + cells[i][0];
        int by = w->tetris_py + cells[i][1];
        if (by >= 0 && by < 22 && bx >= 0 && bx < 10) {
            w->tetris_board[by * 10 + bx] = (signed char)(type + 1);
        }
    }
    int lines = 0;
    for (int by = 21; by >= 0; by--) {
        int full = 1;
        for (int bx = 0; bx < 10; bx++) {
            if (w->tetris_board[by * 10 + bx] == 0) { full = 0; break; }
        }
        if (full) {
            for (int r = by; r > 0; r--) {
                for (int bx = 0; bx < 10; bx++) {
                    w->tetris_board[r * 10 + bx] = w->tetris_board[(r - 1) * 10 + bx];
                }
            }
            for (int bx = 0; bx < 10; bx++) w->tetris_board[bx] = 0;
            by++;
            lines++;
        }
    }
    if (lines > 0) {
        w->tetris_lines += lines;
        w->tetris_score += lines * lines * 100;
    }
    w->tetris_type = w->tetris_next;
    w->tetris_next = (int)(rnd() % 7);
    w->tetris_px = 3;
    w->tetris_py = 0;
    w->tetris_rot = 0;
    if (tetris_collides(w, w->tetris_type, 0, w->tetris_px, w->tetris_py)) {
        w->tetris_over = 1;
    }
}

static void tetris_init(window_t *w) {
    memset(w->tetris_board, 0, 10 * 22);
    w->tetris_score = 0;
    w->tetris_lines = 0;
    w->tetris_over = 0;
    w->tetris_type = (int)(rnd() % 7);
    w->tetris_next = (int)(rnd() % 7);
    w->tetris_px = 3;
    w->tetris_py = 0;
    w->tetris_rot = 0;
    if (tetris_collides(w, w->tetris_type, 0, w->tetris_px, w->tetris_py)) {
        w->tetris_over = 1;
    }
}

static void tetris_move(window_t *w, int dx, int dy) {
    if (w->tetris_over) return;
    if (!tetris_collides(w, w->tetris_type, w->tetris_rot, w->tetris_px + dx, w->tetris_py + dy)) {
        w->tetris_px += dx;
        w->tetris_py += dy;
        if (dy == 1) w->tetris_score += 1;
    } else if (dy == 1) {
        tetris_lock(w);
    }
}

static void tetris_rotate(window_t *w) {
    if (w->tetris_over) return;
    if (w->tetris_type == 1) return;
    int nr = (w->tetris_rot + 1) % 4;
    if (!tetris_collides(w, w->tetris_type, nr, w->tetris_px, w->tetris_py)) {
        w->tetris_rot = nr;
        return;
    }
    if (!tetris_collides(w, w->tetris_type, nr, w->tetris_px - 1, w->tetris_py)) {
        w->tetris_px -= 1;
        w->tetris_rot = nr;
    } else if (!tetris_collides(w, w->tetris_type, nr, w->tetris_px + 1, w->tetris_py)) {
        w->tetris_px += 1;
        w->tetris_rot = nr;
    }
}

static void tetris_drop(window_t *w) {
    if (w->tetris_over) return;
    while (!tetris_collides(w, w->tetris_type, w->tetris_rot, w->tetris_px, w->tetris_py + 1)) {
        w->tetris_py++;
        w->tetris_score += 2;
    }
    tetris_lock(w);
}

static void g2048_init(window_t *w) {
    for (int i = 0; i < 16; i++) w->g2048[i] = 0;
    w->g2048_score = 0;
    w->g2048_over = 0;
    int a = (int)(rnd() % 16);
    int b;
    do { b = (int)(rnd() % 16); } while (a == b);
    w->g2048[a] = (rnd() % 4 == 0) ? 4 : 2;
    w->g2048[b] = (rnd() % 4 == 0) ? 4 : 2;
}

static void g2048_spawn(window_t *w) {
    int empties[16];
    int n = 0;
    for (int i = 0; i < 16; i++) if (w->g2048[i] == 0) empties[n++] = i;
    if (n > 0) {
        int slot = empties[rnd() % n];
        w->g2048[slot] = (rnd() % 4 == 0) ? 4 : 2;
    }
}

static int g2048_can_move(window_t *w) {
    for (int i = 0; i < 16; i++) {
        if (w->g2048[i] == 0) return 1;
        if (i % 4 != 3 && w->g2048[i] == w->g2048[i + 1]) return 1;
        if (i / 4 != 3 && w->g2048[i] == w->g2048[i + 4]) return 1;
    }
    return 0;
}

static void g2048_move(window_t *w, int dir) {
    int changed = 0;
    int score_add = 0;
    for (int pass = 0; pass < 2; pass++) {
        int merged[16];
        for (int i = 0; i < 16; i++) merged[i] = 0;
        for (int line = 0; line < 4; line++) {
            int vals[4];
            for (int k = 0; k < 4; k++) {
                int idx;
                if (dir == 0) idx = line * 4 + k;        // left
                else if (dir == 1) idx = line * 4 + (3 - k); // right
                else if (dir == 2) idx = k * 4 + line;   // up
                else idx = (3 - k) * 4 + line;           // down
                vals[k] = w->g2048[idx];
            }
            int out[4];
            int n = 0;
            for (int k = 0; k < 4; k++) {
                if (vals[k] == 0) continue;
                if (n > 0 && out[n - 1] == vals[k] && merged[line] == 0) {
                    out[n - 1] *= 2;
                    score_add += out[n - 1];
                    merged[line] = 1;
                } else {
                    out[n++] = vals[k];
                }
            }
            while (n < 4) out[n++] = 0;
            for (int k = 0; k < 4; k++) {
                int idx;
                if (dir == 0) idx = line * 4 + k;
                else if (dir == 1) idx = line * 4 + (3 - k);
                else if (dir == 2) idx = k * 4 + line;
                else idx = (3 - k) * 4 + line;
                if (w->g2048[idx] != out[k]) changed = 1;
                w->g2048[idx] = out[k];
            }
        }
        if (changed) break;
    }
    w->g2048_score += score_add;
    if (changed) {
        g2048_spawn(w);
        if (!g2048_can_move(w)) w->g2048_over = 1;
    }
}

static void pong_init(window_t *w) {
    w->pong_py = (w->h - 60) / 2;
    w->pong_ai = (w->h - 60) / 2;
    w->pong_s1 = 0;
    w->pong_s2 = 0;
    w->pong_over = 0;
    w->pong_bx = w->w / 2;
    w->pong_by = w->h / 2;
    w->pong_vx = 0;
    w->pong_vy = 0;
    w->pong_up = 0;
    w->pong_down = 0;
}

static void pong_serve(window_t *w, int dir) {
    w->pong_bx = w->w / 2;
    w->pong_by = w->h / 2;
    w->pong_vx = dir * 2;
    w->pong_vy = (rnd() % 2) ? 2 : -2;
}

static const char *const fortunes[] = {
    "The best way to predict the future is to invent it.",
    "Simplicity is the ultimate sophistication.",
    "In the middle of difficulty lies opportunity.",
    "Talk is cheap. Show me the code.",
    "Software is eating the world.",
    "It works on my machine.",
    "To iterate is human, to recurse is divine.",
    "Premature optimization is the root of all evil.",
    "Debugging is twice as hard as writing the code.",
    "There is no place like 127.0.0.1.",
    "Keep it simple, stupid.",
    "A clever person solves a problem. A wise person avoids it.",
    "Code is like humor. When you have to explain it, it is bad.",
    "First, solve the problem. Then, write the code.",
    "Programs must be written for people to read.",
    "The art of programming is the art of organizing complexity.",
    "Simplicity carried to the extreme becomes elegance.",
    "Do the simplest thing that could possibly work.",
};

static const int NUM_FORTUNES = (int)(sizeof(fortunes) / sizeof(fortunes[0]));

static void fortune_next(window_t *w) {
    w->fortune_idx = (int)(rnd() % NUM_FORTUNES);
}

static void hexview_open(window_t *w) {
    w->hex_offset = 0;
    w->hex_size = 0;
    w->hex_data = NULL;
    int fd = sb_acquire(w->file_path, 0);
    if (fd < 0) return;
    w->hex_fd = fd;
    uint8_t *buf = (uint8_t *)sys_sbrk(48 * 1024);
    if ((int64_t)buf < 0 || !buf) { sb_release(fd); return; }
    w->hex_data = buf;
    int total = 0;
    while (total < 48 * 1024) {
        int n = (int)sb_pull(fd, buf + total, 48 * 1024 - total);
        if (n <= 0) break;
        total += n;
    }
    w->hex_size = total;
    sb_release(fd);
    w->hex_fd = -1;
}

static void mandel_init(window_t *w) {
    int bw = w->w - 20;
    int bh = w->h - TITLE_H - 20;
    w->mandel_buf = (uint32_t *)sys_sbrk((uint64_t)bw * bh * 4);
    w->mandel_ready = 0;
}

static void mandel_compute_rows(window_t *w, int max_rows) {
    int bw = w->w - 20;
    int bh = w->h - TITLE_H - 20;
    if (!w->mandel_buf) return;
    int done = 0;
    for (int y = w->mandel_ready; y < bh && done < max_rows; y++, done++) {
        for (int x = 0; x < bw; x++) {
            int64_t cr = (-210 * 4096) + ((x * 270) * 4096) / (bw > 0 ? bw : 1);
            int64_t ci = (-120 * 4096) + ((y * 240) * 4096) / (bh > 0 ? bh : 1);
            int64_t zr = 0, zi = 0;
            int it = 0;
            for (it = 0; it < 80; it++) {
                int64_t zr2 = zr * zr >> 12;
                int64_t zi2 = zi * zi >> 12;
                if (zr2 + zi2 > (4 << 12)) break;
                int64_t nzr = zr2 - zi2 + cr;
                int64_t nzi = ((2 * zr * zi) >> 12) + ci;
                zr = nzr;
                zi = nzi;
            }
            uint32_t color;
            if (it >= 80) color = 0x000000;
            else {
                int v = it * 8 + 8;
                if (v > 255) v = 255;
                color = ((uint32_t)(v / 2) << 16) | ((uint32_t)(v * 3 / 4) << 8) | (uint32_t)v;
            }
            w->mandel_buf[y * bw + x] = color;
        }
    }
    w->mandel_ready += done;
}

static void paint_init(window_t *w) {
    w->paint_canvas = (uint32_t *)sys_sbrk(512 * 320 * 4);
    w->paint_color = 0x2C3E50;
    w->brush_size = 3;
    w->painting = 0;
}

static void paint_clear(window_t *w) {
    for (int i = 0; i < 512 * 320; i++) w->paint_canvas[i] = 0xFFFFFF;
}

static void paint_dot(window_t *w, int cx, int cy) {
    if (cx < 0 || cx >= 512 || cy < 0 || cy >= 320) return;
    int r = w->brush_size;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int px = cx + dx;
            int py = cy + dy;
            if (px < 0 || px >= 512 || py < 0 || py >= 320) continue;
            if (dx * dx + dy * dy <= r * r) {
                w->paint_canvas[py * 512 + px] = w->paint_color;
            }
        }
    }
}

static void create_window(int type, const char *title, int x, int y, int w, int h) {
    if (num_windows >= MAX_WINDOWS) return;
    window_t *win = &windows[num_windows];
    memset(win, 0, sizeof(window_t));
    win->id = num_windows;
    win->active = 1;
    win->type = type;
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->minimized = 0;
    win->maximized = 0;
    win->brush_size = 3;
    int i = 0;
    while (title[i] && i < 63) { win->title[i] = title[i]; i++; }
    win->title[i] = 0;

    if (type == WTYPE_TERMINAL) {
        terminal_init(win);
    } else if (type == WTYPE_FILE_BRO) {
        strcpy(win->current_dir, "/");
        filebrowser_refresh(win);
    } else if (type == WTYPE_SNAKE) {
        win->snake_len = 3;
        win->snake_x[0] = 10; win->snake_y[0] = 10;
        win->snake_x[1] = 9;  win->snake_y[1] = 10;
        win->snake_x[2] = 8;  win->snake_y[2] = 10;
        win->snake_dir = 1;
        win->food_x = 15; win->food_y = 10;
        win->snake_dead = 0;
        win->last_update = sys_times(0);
    } else if (type == WTYPE_CALC) {
        strcpy(win->calc_disp, "0");
        win->calc_acc = 0;
        win->calc_op = 0;
        win->calc_fresh = 1;
    } else if (type == WTYPE_EDITOR) {
        strcpy(win->file_path, "/tmp/editor.txt");
        editor_load(win);
        win->cursor_x = 0;
        win->cursor_y = 0;
    } else if (type == WTYPE_PAINT) {
        paint_init(win);
        paint_clear(win);
    } else if (type == WTYPE_PROCMON) {
        win->proc_scroll = 0;
    } else if (type == WTYPE_HEXVIEW) {
        strcpy(win->file_path, "/desktop.elf");
        hexview_open(win);
    } else if (type == WTYPE_TETRIS) {
        tetris_init(win);
    } else if (type == WTYPE_G2048) {
        g2048_init(win);
    } else if (type == WTYPE_MANDEL) {
        mandel_init(win);
    } else if (type == WTYPE_CLOCK) {
    } else if (type == WTYPE_FORTUNE) {
        fortune_next(win);
    } else if (type == WTYPE_PONG) {
        pong_init(win);
        pong_serve(win, 1);
    } else if (type == WTYPE_MATRIX) {
        for (int c = 0; c < 40; c++) {
            win->matrix_off[c] = (int)(rnd() % 24);
            win->matrix_speed[c] = 1 + (int)(rnd() % 3);
            win->matrix_char[c] = "ABCDEF0123456789<>!?*$#"[rnd() % 19];
        }
        win->last_update = sys_times(0);
    }

    num_windows++;
}

static void draw_window_title(window_t *w) {
    int is_top = (top_window() == w->id);
    uint32_t title_bg = (drag_win == w->id || is_top) ? 0x2980B9 : 0x7F8C8D;

    draw_rect(w->x, w->y, w->w, TITLE_H, title_bg);
    draw_string_limit(w->x + 10, w->y + 8, w->title, (w->w - 110) / 8, 0xFFFFFF);

    draw_rect(w->x + w->w - 24, w->y, 24, TITLE_H, 0xE74C3C);
    draw_string(w->x + w->w - 16, w->y + 8, "x", 0xFFFFFF);

    draw_rect(w->x + w->w - 48, w->y, 24, TITLE_H, 0x16A085);
    if (w->maximized) draw_string(w->x + w->w - 42, w->y + 8, "_", 0xFFFFFF);
    else draw_string(w->x + w->w - 42, w->y + 8, "[]", 0xFFFFFF);

    draw_rect(w->x + w->w - 72, w->y, 24, TITLE_H, 0xF39C12);
    draw_string(w->x + w->w - 64, w->y + 8, "-", 0xFFFFFF);
}

static void draw_app_window(window_t *w) {
    int x = w->x;
    int y = w->y;
    int win_w = w->w;
    int win_h = w->h;

    if (w->type == WTYPE_TERMINAL) {
        draw_rect_alpha(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1E1E1E, 235);
        for (int row = 0; row < 24; row++) {
            for (int col = 0; col < 60; col++) {
                char c = w->text[row * 60 + col];
                if (c) draw_char(x + 8 + col * 8, y + TITLE_H + 8 + row * 12, c, 0x00FF00);
            }
        }
        uint64_t ticks = sys_times(0);
        if ((ticks / 50) % 2 == 0) {
            draw_rect(x + 8 + w->cursor_x * 8, y + TITLE_H + 8 + w->cursor_y * 12, 8, 12, 0x00FF00);
        }
    } else if (w->type == WTYPE_FILE_BRO) {
        char title_buf[200];
        strcpy(title_buf, "ShadowBox Disk - ");
        strcat(title_buf, w->current_dir);
        draw_string_limit(x + 10, y + TITLE_H + 10, title_buf, (win_w - 20) / 8, 0x333333);
        draw_rect(x + 10, y + TITLE_H + 24, win_w - 20, 1, 0xBDC3C7);

        int rows = (win_h - TITLE_H - 40) / 20;
        if (rows < 0) rows = 0;
        for (int i = 0; i < w->num_entries && i < rows; i++) {
            uint32_t icon_color = 0x95A5A6;
            if (is_dir_name(w->entries[i].name)) icon_color = 0xF39C12;
            int iy = y + TITLE_H + 34 + i * 20;
            if (mouse_btn_down && in_rect(mouse_x, mouse_y, x + 10, iy, win_w - 20, 20)) {
                draw_rect(x + 10, iy, win_w - 20, 20, 0xD6EAF8);
            }
            draw_rect(x + 10, iy + 2, 12, 10, icon_color);
            draw_string_limit(x + 28, iy + 3, w->entries[i].name, (win_w - 40) / 8, 0x2C3E50);
        }
    } else if (w->type == WTYPE_SYS_MON) {
        uint64_t now = sys_times(0);
        if (now - w->last_update > 50) {
            w->last_update = now;
            sys_mem_info((uint64_t *)w->text);
        }
        draw_string(x + 10, y + TITLE_H + 10, "ShadowBox Kernel Statistics", 0x2C3E50);
        draw_string(x + 10, y + TITLE_H + 30, "Memory Free:", 0x2980B9);
        draw_number(x + 110, y + TITLE_H + 30, ((uint64_t *)w->text)[0] * 4 / 1024, 0x2C3E50);
        draw_string(x + 170, y + TITLE_H + 30, "MB", 0x2C3E50);
        draw_string(x + 10, y + TITLE_H + 50, "Drivers Loaded:", 0x2980B9);
        draw_string(x + 10, y + TITLE_H + 65, "- AHCI (SATA)", 0x34495E);
        draw_string(x + 10, y + TITLE_H + 80, "- e1000 & rtl8139 (Net)", 0x34495E);
        draw_string(x + 10, y + TITLE_H + 95, "- USB (EHCI/XHCI)", 0x34495E);
        draw_string(x + 10, y + TITLE_H + 110, "- ACPI / APIC / IOAPIC", 0x34495E);
        draw_string(x + 10, y + TITLE_H + 125, "- HDA Audio", 0x34495E);
        draw_string(x + 10, y + TITLE_H + 140, "- ShadowBox Compositor", 0x34495E);
    } else if (w->type == WTYPE_ABOUT) {
        draw_rect(x + 2, y + TITLE_H, win_w - 4, win_h - TITLE_H, 0xFFFFFF);
        draw_string(x + 20, y + TITLE_H + 26, "ShadowBox OS v0.2.0", 0x333333);
        draw_string(x + 20, y + TITLE_H + 46, "By: darkdevil404", 0x333333);
        draw_string(x + 20, y + TITLE_H + 66, "x86_64 from-scratch kernel + GUI", 0x555555);
        draw_string(x + 20, y + TITLE_H + 86, "19 built-in applications", 0x555555);
        draw_button(x + 110, y + TITLE_H + 96, 80, 26, "OK", 0x2980B9, 0xFFFFFF);
    } else if (w->type == WTYPE_VIEWER) {
        draw_rect(x + 2, y + TITLE_H, win_w - 4, win_h - TITLE_H, 0x1E1E1E);
        if (logo_buffer) {
            int img_w = 256;
            int img_h = 256;
            int start_x = x + (win_w - img_w) / 2;
            int start_y = y + TITLE_H + (win_h - TITLE_H - img_h) / 2;
            for (int iy = 0; iy < img_h; iy++) {
                for (int ix = 0; ix < img_w; ix++) {
                    int px = start_x + ix;
                    int py = start_y + iy;
                    if (px >= x && px < x + win_w && py >= y + TITLE_H && py < y + win_h) {
                        backbuffer[py * SCREEN_WIDTH + px] = logo_buffer[iy * img_w + ix];
                    }
                }
            }
        }
    } else if (w->type == WTYPE_SNAKE) {
        draw_rect(x + 2, y + TITLE_H, win_w - 4, win_h - TITLE_H, 0x000000);
        if (w->snake_dead) {
            draw_string(x + win_w / 2 - 40, y + win_h / 2 - 10, "GAME OVER", 0xFF0000);
            draw_string(x + win_w / 2 - 70, y + win_h / 2 + 10, "Press R to Restart", 0xFFFFFF);
        } else {
            draw_rect(x + 2 + w->food_x * 10, y + TITLE_H + w->food_y * 10, 10, 10, 0xE74C3C);
            for (int i = 0; i < w->snake_len; i++) {
                uint32_t color = (i == 0) ? 0x2ECC71 : 0x27AE60;
                draw_rect(x + 2 + w->snake_x[i] * 10, y + TITLE_H + w->snake_y[i] * 10, 10, 10, color);
            }
        }
    } else if (w->type == WTYPE_CALC) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x2C3E50);
        draw_rect(x + 10, y + TITLE_H + 10, win_w - 20, 36, 0x1B2631);
        int disp_len = 0;
        while (w->calc_disp[disp_len]) disp_len++;
        draw_string(x + win_w - 20 - disp_len * 8, y + TITLE_H + 22, w->calc_disp, 0x00FF88);
        if (w->calc_op) {
            char opbuf[4];
            opbuf[0] = w->calc_op;
            opbuf[1] = 0;
            draw_string(x + 14, y + TITLE_H + 22, opbuf, 0xF39C12);
        }
        const char *labels[20] = {
            "C", "+/-", "%", "/",
            "7", "8", "9", "*",
            "4", "5", "6", "-",
            "1", "2", "3", "+",
            "0", ".", "=", "<-",
        };
        for (int b = 0; b < 20; b++) {
            int row = b / 4;
            int col = b % 4;
            int bx = x + 10 + col * 54;
            int by = y + TITLE_H + 56 + row * 42;
            int bw = 46;
            if (b == 18) { bx = x + 10 + 2 * 54; bw = 100; }
            uint32_t bg = (b == 16 || b == 17 || b == 18) ? 0x2980B9 : (b < 4 ? 0x34495E : 0x1B2631);
            if (mouse_btn_down && in_rect(mouse_x, mouse_y, bx, by, bw, 34)) {
                bg = 0x5499C7;
            }
            draw_rect(bx, by, bw, 34, bg);
            draw_rect(bx, by, bw, 1, 0x3E5C76);
            int tw = 0;
            while (labels[b][tw]) tw++;
            tw *= 8;
            draw_string(bx + (bw - tw) / 2, by + 13, labels[b], 0xECF0F1);
        }
    } else if (w->type == WTYPE_EDITOR) {
        draw_rect_alpha(x, y + TITLE_H, win_w, win_h - TITLE_H - 22, 0x1E1E1E, 235);
        for (int row = 0; row < 24; row++) {
            for (int col = 0; col < 60; col++) {
                char c = w->text[row * 60 + col];
                if (c) draw_char(x + 8 + col * 8, y + TITLE_H + 8 + row * 12, c, 0xD3D3D3);
            }
        }
        uint64_t ticks = sys_times(0);
        if ((ticks / 50) % 2 == 0) {
            draw_rect(x + 8 + w->cursor_x * 8, y + TITLE_H + 8 + w->cursor_y * 12, 8, 12, 0xFFFFFF);
        }
        draw_rect(x, y + win_h - 22, win_w, 22, 0x34495E);
        char status[300];
        strcpy(status, w->file_path);
        if (w->cursor_y >= 0) {
            strcat(status, "  Ln ");
            char ln[16];
            num_to_str(w->cursor_y + 1, ln);
            strcat(status, ln);
        }
        if (sys_times(0) - w->editor_msg_at < 100 && w->editor_msg[0]) {
            strcat(status, "  |  ");
            strcat(status, w->editor_msg);
        } else {
            strcat(status, "  |  Ctrl+S save");
        }
        draw_string_limit(x + 6, y + win_h - 14, status, (win_w - 12) / 8, 0xECF0F1);
    } else if (w->type == WTYPE_PAINT) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x7F8C8D);
        for (int i = 0; i < 512 * 320; i++) {
            int cx = x + 10 + (i % 512);
            int cy = y + TITLE_H + 10 + (i / 512);
            if (cx >= x && cx < x + win_w && cy >= y + TITLE_H && cy < y + win_h) {
                backbuffer[cy * SCREEN_WIDTH + cx] = w->paint_canvas[i];
            }
        }
        uint32_t palette[8] = {
            0x2C3E50, 0xE74C3C, 0xE67E22, 0xF1C40F,
            0x2ECC71, 0x3498DB, 0x9B59B6, 0xFFFFFF,
        };
        for (int p = 0; p < 8; p++) {
            int px = x + 10 + p * 38;
            int py = y + win_h - 44;
            draw_rect(px, py, 32, 32, palette[p]);
            if (w->paint_color == palette[p]) draw_rect(px - 1, py - 1, 34, 34, 0xFFFFFF);
        }
        draw_button(x + 10 + 8 * 38 + 8, y + win_h - 44, 70, 32, "Clear", 0xE74C3C, 0xFFFFFF);
        draw_button(x + 10 + 8 * 38 + 84, y + win_h - 44, 36, 32, "-", 0x2980B9, 0xFFFFFF);
        draw_string(x + 10 + 8 * 38 + 124, y + win_h - 38, "sz", 0xFFFFFF);
        draw_number(x + 10 + 8 * 38 + 140, y + win_h - 38, w->brush_size, 0xFFFFFF);
        draw_button(x + 10 + 8 * 38 + 170, y + win_h - 44, 36, 32, "+", 0x2980B9, 0xFFFFFF);
    } else if (w->type == WTYPE_PROCMON) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0xF4F6F7);
        struct proc_info procs[32];
        int n = sys_proc_info(procs, 32);
        draw_string(x + 10, y + TITLE_H + 6, "PID    PPID  State  ", 0x2980B9);
        draw_rect(x + 10, y + TITLE_H + 18, win_w - 20, 1, 0xBDC3C7);
        int rows = (win_h - TITLE_H - 40) / 16;
        if (rows < 0) rows = 0;
        for (int i = 0; i < n && i < rows; i++) {
            int iy = y + TITLE_H + 26 + i * 16;
            draw_number(x + 10, iy, procs[i].pid, 0x2C3E50);
            draw_number(x + 60, iy, procs[i].ppid, 0x2C3E50);
            draw_string(x + 110, iy, procs[i].state == 0 ? "run " : "sleep", 0x2C3E50);
        }
        draw_string(x + 10, y + win_h - 18, "Up/Down scroll", 0x95A5A6);
    } else if (w->type == WTYPE_HEXVIEW) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1B2631);
        char pathbuf[200];
        strcpy(pathbuf, "Hex: ");
        strcat(pathbuf, w->file_path);
        draw_string_limit(x + 10, y + TITLE_H + 6, pathbuf, (win_w - 20) / 8, 0xF39C12);
        int rows = (win_h - TITLE_H - 44) / 16;
        if (rows < 0) rows = 0;
        for (int r = 0; r < rows; r++) {
            int off = w->hex_offset + r * 16;
            if (off >= w->hex_size) break;
            int iy = y + TITLE_H + 24 + r * 16;
            char obuf[16];
            hex_to_str((uint32_t)off, obuf);
            draw_string(x + 10, iy, obuf, 0x3498DB);
            for (int c = 0; c < 16; c++) {
                int idx = off + c;
                if (idx >= w->hex_size) break;
                char hb[4];
                hb[0] = "0123456789ABCDEF"[w->hex_data[idx] >> 4];
                hb[1] = "0123456789ABCDEF"[w->hex_data[idx] & 0xF];
                hb[2] = ' ';
                hb[3] = 0;
                draw_string(x + 70 + c * 26, iy, hb, 0xECF0F1);
            }
            char ascii[18];
            for (int c = 0; c < 16; c++) {
                int idx = off + c;
                if (idx >= w->hex_size) break;
                uint8_t b = w->hex_data[idx];
                ascii[c] = (b >= 32 && b < 127) ? (char)b : '.';
            }
            ascii[16] = 0;
            draw_string(x + 10 + 16 * 26 + 20, iy, ascii, 0x58D68F);
        }
    } else if (w->type == WTYPE_TETRIS) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x111111);
        int cell = 20;
        int bx = x + 10;
        int by = y + TITLE_H + 30;
        for (int r = 0; r < 22; r++) {
            for (int c = 0; c < 10; c++) {
                uint32_t col = 0x1A1A1A;
                if (w->tetris_board[r * 10 + c]) {
                    col = 0x3498DB * (w->tetris_board[r * 10 + c] + 1);
                }
                draw_rect(bx + c * cell, by + r * cell, cell - 1, cell - 1, col);
            }
        }
        if (!w->tetris_over) {
            const signed char (*cells)[2];
            int t = w->tetris_type;
            int rot = w->tetris_rot;
            if (t == 1) cells = TET_O[0];
            else if (t == 2) cells = TET_T[rot];
            else if (t == 3) cells = TET_S[rot];
            else if (t == 4) cells = TET_Z[rot];
            else if (t == 5) cells = TET_J[rot];
            else if (t == 6) cells = TET_L[rot];
            else cells = TET_I[rot];
            uint32_t pcol = 0x1F618D;
            for (int i = 0; i < 4; i++) {
                int cx = w->tetris_px + cells[i][0];
                int cy = w->tetris_py + cells[i][1];
                if (cy >= 0) draw_rect(bx + cx * cell, by + cy * cell, cell - 1, cell - 1, pcol);
            }
        }
        draw_string(x + 10, y + TITLE_H + 8, "Score", 0xECF0F1);
        draw_number(x + 60, y + TITLE_H + 8, w->tetris_score, 0xF1C40F);
        if (w->tetris_over) {
            draw_rect(x + 30, y + TITLE_H + 120, win_w - 60, 40, 0xC0392B);
            draw_string(x + 55, y + TITLE_H + 130, "GAME OVER", 0xFFFFFF);
            draw_string(x + 45, y + TITLE_H + 148, "R restart", 0xFFFFFF);
        }
    } else if (w->type == WTYPE_G2048) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x2C3E50);
        draw_string(x + 12, y + TITLE_H + 8, "2048  Score:", 0xECF0F1);
        draw_number(x + 110, y + TITLE_H + 8, w->g2048_score, 0xF1C40F);
        int cell = 64;
        int gap = 6;
        int ox = x + (win_w - 4 * cell - 3 * gap) / 2;
        int oy = y + TITLE_H + 34;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                int v = w->g2048[r * 4 + c];
                uint32_t bg = 0x17202A;
                uint32_t fg = 0xFFFFFF;
                if (v == 2) { bg = 0x1F618D; }
                else if (v == 4) { bg = 0x2980B9; }
                else if (v == 8) { bg = 0x16A085; }
                else if (v == 16) { bg = 0x27AE60; }
                else if (v == 32) { bg = 0xF39C12; }
                else if (v == 64) { bg = 0xE67E22; }
                else if (v == 128) { bg = 0xD35400; }
                else if (v == 256) { bg = 0xE74C3C; }
                else if (v == 512) { bg = 0xC0392B; }
                else if (v == 1024) { bg = 0x8E44AD; }
                else if (v >= 2048) { bg = 0x943126; }
                int cx = ox + c * (cell + gap);
                int cy = oy + r * (cell + gap);
                draw_rect(cx, cy, cell, cell, bg);
                if (v) {
                    char vb[8];
                    num_to_str(v, vb);
                    int len = 0;
                    while (vb[len]) len++;
                    draw_string(cx + (cell - len * 8) / 2, cy + (cell - 8) / 2, vb, fg);
                }
            }
        }
        if (w->g2048_over) {
            draw_rect(x + 40, y + TITLE_H + 130, win_w - 80, 40, 0xC0392B);
            draw_string(x + 80, y + TITLE_H + 140, "GAME OVER", 0xFFFFFF);
            draw_string(x + 70, y + TITLE_H + 156, "R restart", 0xFFFFFF);
        }
    } else if (w->type == WTYPE_MANDEL) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x000000);
        int bw = win_w - 20;
        if (w->mandel_buf) {
            for (int iy = 0; iy < w->mandel_ready; iy++) {
                for (int ix = 0; ix < bw; ix++) {
                    backbuffer[(y + TITLE_H + 10 + iy) * SCREEN_WIDTH + (x + 10 + ix)] =
                        w->mandel_buf[iy * bw + ix];
                }
            }
        }
        if (w->mandel_ready >= win_h - TITLE_H - 20) {
            draw_string(x + 10, y + TITLE_H + 6, "Mandelbrot set", 0xFFFFFF);
        }
    } else if (w->type == WTYPE_CLOCK) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1B2631);
        int cx = x + win_w / 2;
        int cy = y + TITLE_H + (win_h - TITLE_H) / 2;
        int r = (win_w < win_h - TITLE_H ? win_w : win_h - TITLE_H) / 2 - 12;
        draw_circle(cx, cy, r, 0xECF0F1, 0);
        for (int t = 0; t < 12; t++) {
            int deg = t * 30;
            int s, c;
            sincos(deg, &s, &c);
            int x1 = cx + c * r / 1000;
            int y1 = cy - s * r / 1000;
            int x2 = cx + c * (r - 8) / 1000;
            int y2 = cy - s * (r - 8) / 1000;
            draw_line(x1, y1, x2, y2, 0xECF0F1);
        }
        uint64_t tt = sys_times(0) / 100;
        int sec = tt % 60;
        int min = (tt / 60) % 60;
        int hr = (tt / 3600) % 12;
        int s, c;
        sincos(sec * 6, &s, &c);
        draw_line(cx, cy, cx + c * (r - 14) / 1000, cy - s * (r - 14) / 1000, 0xE74C3C);
        sincos(min * 6, &s, &c);
        draw_line(cx, cy, cx + c * (r - 26) / 1000, cy - s * (r - 26) / 1000, 0xECF0F1);
        sincos((hr * 60 + min) / 2, &s, &c);
        draw_line(cx, cy, cx + c * (r - 40) / 1000, cy - s * (r - 40) / 1000, 0xFFFFFF);
        draw_circle(cx, cy, 3, 0xE74C3C, 1);
    } else if (w->type == WTYPE_FORTUNE) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0xF4F6F7);
        draw_string(x + 16, y + TITLE_H + 14, "Fortune Cookie", 0xE67E22);
        draw_rect(x + 16, y + TITLE_H + 30, win_w - 32, 1, 0xBDC3C7);
        const char *f = fortunes[w->fortune_idx];
        int pos = 0;
        while (f[pos]) {
            int len = 0;
            while (f[pos + len] && len < 44) len++;
            draw_string_limit(x + 16, y + TITLE_H + 46 + (pos / 44) * 16, f + pos, len, 0x2C3E50);
            pos += len;
            while (f[pos] == ' ') pos++;
        }
        draw_string(x + 16, y + win_h - 30, "Press N for another", 0x95A5A6);
    } else if (w->type == WTYPE_PONG) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x111111);
        draw_rect(x + win_w / 2 - 1, y + TITLE_H, 2, win_h - TITLE_H, 0x2C3E50);
        draw_rect(x + 24, y + TITLE_H + w->pong_py, 8, 60, 0x00FF88);
        draw_rect(x + win_w - 32, y + TITLE_H + w->pong_ai, 8, 60, 0xE74C3C);
        draw_circle(x + w->pong_bx, y + TITLE_H + w->pong_by, 5, 0xFFFFFF, 1);
        draw_number(x + 60, y + TITLE_H + 8, w->pong_s1, 0xFFFFFF);
        draw_number(x + win_w - 80, y + TITLE_H + 8, w->pong_s2, 0xFFFFFF);
        if (w->pong_over) {
            draw_string(x + win_w / 2 - 60, y + TITLE_H + win_h / 3, "GAME OVER", 0xFF0000);
            draw_string(x + win_w / 2 - 70, y + TITLE_H + win_h / 3 + 20, "Space to play", 0xFFFFFF);
        }
    } else if (w->type == WTYPE_MATRIX) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x000000);
        int cols = 40;
        int char_h = 16;
        for (int c = 0; c < cols; c++) {
            int head = w->matrix_off[c];
            for (int row = 0; row <= head && row < 20; row++) {
                int fade = (head - row);
                uint32_t color = 0x00FF00;
                if (fade > 6) color = 0x006600;
                else if (fade > 3) color = 0x00CC00;
                if (row == head) color = 0xE0FFE0;
                draw_char(x + 8 + c * 8, y + TITLE_H + row * char_h, (char)w->matrix_char[c], color);
            }
        }
    } else if (w->type == WTYPE_MEMVIEW) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0xF4F6F7);
        uint64_t mem[2];
        sys_mem_info(mem);
        uint64_t total_mb = mem[0] * 4 / 1024;
        uint64_t used_mb = mem[1] * 4 / 1024;
        draw_string(x + 14, y + TITLE_H + 12, "System Memory", 0x2C3E50);
        draw_string(x + 14, y + TITLE_H + 36, "Total:", 0x2980B9);
        draw_number(x + 70, y + TITLE_H + 36, total_mb, 0x2C3E50);
        draw_string(x + 120, y + TITLE_H + 36, "MB", 0x2C3E50);
        draw_string(x + 14, y + TITLE_H + 56, "Used:", 0x2980B9);
        draw_number(x + 70, y + TITLE_H + 56, used_mb, 0x2C3E50);
        draw_string(x + 120, y + TITLE_H + 56, "MB", 0x2C3E50);
        draw_string(x + 14, y + TITLE_H + 76, "Free:", 0x2980B9);
        draw_number(x + 70, y + TITLE_H + 76, total_mb - used_mb, 0x2C3E50);
        draw_string(x + 120, y + TITLE_H + 76, "MB", 0x2C3E50);
        int barx = x + 14;
        int bary = y + TITLE_H + 104;
        draw_rect(barx, bary, win_w - 28, 18, 0xBDC3C7);
        int used_frac = 0;
        if (total_mb > 0) used_frac = (int)((used_mb * (win_w - 28)) / total_mb);
        draw_rect(barx, bary, used_frac, 18, 0xE74C3C);
        if (used_frac < 0) used_frac = 0;
        draw_string(x + 14, y + TITLE_H + 136, "Uptime:", 0x2980B9);
        uint64_t t = sys_times(0) / 100;
        draw_number(x + 80, y + TITLE_H + 136, t / 3600, 0x2C3E50);
        draw_string(x + 118, y + TITLE_H + 136, "h", 0x2C3E50);
    }
}

static void draw_resize_handle(window_t *w) {
    if (w->minimized) return;
    draw_rect(w->x + w->w - 10, w->y + w->h - 10, 10, 10, 0x2980B9);
    draw_line(w->x + w->w - 6, w->y + w->h - 2, w->x + w->w - 2, w->y + w->h - 6, 0xFFFFFF);
    draw_line(w->x + w->w - 6, w->y + w->h - 6, w->x + w->w - 2, w->y + w->h - 6, 0xFFFFFF);
}

static void draw_window(window_t *w) {
    if (!w->active || w->minimized) return;
    draw_drop_shadow(w->x, w->y, w->w, w->h);
    uint32_t bg_color = 0xFFFFFF;
    if (w->type == WTYPE_TERMINAL || w->type == WTYPE_EDITOR ||
        w->type == WTYPE_HEXVIEW || w->type == WTYPE_TETRIS ||
        w->type == WTYPE_SNAKE || w->type == WTYPE_PONG ||
        w->type == WTYPE_MATRIX || w->type == WTYPE_CLOCK ||
        w->type == WTYPE_G2048) {
        bg_color = 0x111111;
    }
    draw_rect_alpha(w->x, w->y, w->w, w->h, bg_color, 245);
    draw_window_title(w);
    draw_app_window(w);
    draw_resize_handle(w);
}

static void draw_desktop_icons(void) {
    const char *names[6] = {"Terminal", "Files", "SysMon", "Calc", "Editor", "Paint"};
    uint32_t colors[6] = {0x2ECC71, 0xF39C12, 0x3498DB, 0xE67E22, 0x9B59B6, 0xE74C3C};
    int x = 40;
    int y = 60;
    for (int i = 0; i < 6; i++) {
        if (i == 3) { x = 40; y = 180; }
        if (i == 6) break;
        if (mouse_btn_down && in_rect(mouse_x, mouse_y, x, y, 48, 40)) {
            draw_rect(x - 2, y - 2, 52, 44, 0xFFFFFF);
        }
        draw_rect(x, y, 48, 40, colors[i]);
        draw_rect(x, y, 48, 1, 0xFFFFFF);
        draw_string(x + 20, y + 16, names[i], 0xFFFFFF);
        draw_string(x + 16, y + 46, names[i], 0xFFFFFF);
        x += 130;
    }
}

static void draw_start_menu(void) {
    if (!menu_open) return;
    draw_drop_shadow(MENU_MX, MENU_MY, MENU_MW, MENU_MH);
    draw_rect_alpha(MENU_MX, MENU_MY, MENU_MW, MENU_MH, 0x2C3E50, 235);
    draw_string(MENU_MX + 16, MENU_MY + 12, "ShadowBox OS", 0x3498DB);
    draw_string(MENU_MX + 16, MENU_MY + 24, "19 apps | Ctrl+Alt shortcuts", 0x7F8C8D);
    draw_rect(MENU_MX + 10, MENU_MY + 36, MENU_MW - 20, 1, 0x34495E);

    int hover = menu_item_at();
    for (int i = 0; i < NUM_APPS; i++) {
        int row = i / MENU_COLS;
        int col = i % MENU_COLS;
        int mx = MENU_MX + col * (MENU_COL_W + MENU_COL_GAP);
        int my = MENU_MY + 40 + row * MENU_ROW_H;
        if (i == hover) {
            draw_rect_alpha(mx, my, MENU_COL_W, MENU_ROW_H, 0x3498DB, 150);
        }
        uint32_t fg = 0xECF0F1;
        if (i == NUM_APPS - 1) fg = 0xE74C3C;
        draw_string(mx + 14, my + 10, apps[i].name, fg);
    }
}

static void draw_taskbar(void) {
    draw_rect_alpha(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, 0x1C2833, 210);
    draw_rect_alpha(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 1, 0x34495E, 255);

    int start_pushed = menu_open || (mouse_btn_down && in_rect(mouse_x, mouse_y, 10, SCREEN_HEIGHT - 35, 80, 30));
    draw_rect(10, SCREEN_HEIGHT - 35, 80, 30, start_pushed ? 0x2980B9 : 0x3498DB);
    draw_string(30, SCREEN_HEIGHT - 24, "Shadow", 0xFFFFFF);

    int taskbar_x = 100;
    for (int i = 0; i < num_windows; i++) {
        window_t *w = &windows[i];
        int is_top = (i == top_window());
        uint32_t bg = w->minimized ? 0x1C2833 : (is_top ? 0x2980B9 : 0x2C3E50);
        draw_rect(taskbar_x, SCREEN_HEIGHT - 35, 118, 30, bg);
        draw_rect(taskbar_x, SCREEN_HEIGHT - 35, 118, 2, is_top ? 0xECF0F1 : 0x34495E);
        char short_title[12];
        int len = 0;
        while (w->title[len] && len < 10) { short_title[len] = w->title[len]; len++; }
        short_title[len] = 0;
        if (w->title[len]) { short_title[8] = '.'; short_title[9] = '.'; short_title[10] = '.'; short_title[11] = 0; }
        draw_string(taskbar_x + 8, SCREEN_HEIGHT - 24, short_title, 0xFFFFFF);
        taskbar_x += 122;
        if (taskbar_x > SCREEN_WIDTH - 200) break;
    }

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

static void draw_desktop(void) {
    uint64_t total = SCREEN_WIDTH * SCREEN_HEIGHT;
    for (uint64_t i = 0; i < total; i++) {
        backbuffer[i] = wallpaper_buffer[i];
    }

    draw_desktop_icons();

    for (int i = 0; i < num_windows; i++) {
        draw_window(&windows[i]);
    }

    draw_start_menu();
    draw_taskbar();
    draw_cursor(mouse_x, mouse_y);

    for (uint64_t i = 0; i < total; i++) {
        fb[i] = backbuffer[i];
    }
}

static int taskbar_tab_at(void) {
    int taskbar_x = 100;
    for (int i = 0; i < num_windows; i++) {
        if (in_rect(mouse_x, mouse_y, taskbar_x, SCREEN_HEIGHT - 35, 118, 30)) {
            return i;
        }
        taskbar_x += 122;
        if (taskbar_x > SCREEN_WIDTH - 200) break;
    }
    return -1;
}

static void calc_input(window_t *w, char c) {
    if (c >= '0' && c <= '9') {
        if (w->calc_fresh) {
            w->calc_disp[0] = c;
            w->calc_disp[1] = 0;
            w->calc_fresh = 0;
        } else {
            int len = 0;
            while (w->calc_disp[len]) len++;
            if (len < 16) {
                w->calc_disp[len] = c;
                w->calc_disp[len + 1] = 0;
            }
        }
        return;
    }
    if (c == 'C') {
        strcpy(w->calc_disp, "0");
        w->calc_acc = 0;
        w->calc_op = 0;
        w->calc_fresh = 1;
        return;
    }
    if (c == '<') {
        int len = 0;
        while (w->calc_disp[len]) len++;
        if (len > 1) {
            w->calc_disp[len - 1] = 0;
        } else {
            strcpy(w->calc_disp, "0");
            w->calc_fresh = 1;
        }
        return;
    }
    if (c == 'P') {
        if (!w->calc_fresh) {
            w->calc_acc = -atoi64(w->calc_disp);
            num_to_str(w->calc_acc, w->calc_disp);
            w->calc_fresh = 1;
        }
        return;
    }
    if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
        if (w->calc_op && !w->calc_fresh) {
            int64_t b = atoi64(w->calc_disp);
            int64_t r = w->calc_acc;
            if (w->calc_op == '+') r += b;
            else if (w->calc_op == '-') r -= b;
            else if (w->calc_op == '*') r *= b;
            else if (w->calc_op == '/') { if (b != 0) r /= b; }
            else if (w->calc_op == '%') { if (b != 0) r = w->calc_acc * b / 100; }
            w->calc_acc = r;
            num_to_str(r, w->calc_disp);
        }
        if (c == '=') {
            w->calc_op = 0;
            w->calc_fresh = 1;
        } else {
            w->calc_acc = atoi64(w->calc_disp);
            w->calc_op = c;
            w->calc_fresh = 1;
        }
    }
}

static void calc_click(window_t *w) {
    int row = (mouse_y - (w->y + TITLE_H + 56)) / 42;
    int col = (mouse_x - (w->x + 10)) / 54;
    if (mouse_x >= w->x + 10 + 2 * 54 && mouse_x < w->x + 10 + 2 * 54 + 100 &&
        mouse_y >= w->y + TITLE_H + 56 + 4 * 42 && mouse_y < w->y + TITLE_H + 56 + 4 * 42 + 34) {
        calc_input(w, '=');
        return;
    }
    if (row < 0 || row > 4 || col < 0 || col > 3) return;
    const char *labels[16] = {
        "C", "P", "%", "/",
        "7", "8", "9", "*",
        "4", "5", "6", "-",
        "1", "2", "3", "+",
    };
    if (row == 4) {
        if (col == 0) calc_input(w, '0');
        else if (col == 1) { }
        else if (col == 2) calc_input(w, '=');
        else calc_input(w, '<');
        return;
    }
    char lab = labels[row * 4 + col][0];
    if (lab == 'C' || lab == 'P' || lab == '%' || lab == '/') {
        calc_input(w, lab);
        return;
    }
    calc_input(w, lab);
}

static void handle_app_click(window_t *w, int idx) {
    if (w->type == WTYPE_ABOUT) {
        if (in_rect(mouse_x, mouse_y, w->x + 110, w->y + TITLE_H + 96, 80, 26)) {
            close_window(idx);
        }
    } else if (w->type == WTYPE_FILE_BRO) {
        int iy = w->y + TITLE_H + 34;
        int rows = (w->h - TITLE_H - 40) / 20;
        for (int i = 0; i < w->num_entries && i < rows; i++) {
            if (in_rect(mouse_x, mouse_y, w->x + 10, iy, w->w - 20, 20)) {
                if (i == 0 && strcmp(w->entries[i].name, "..") == 0) {
                    char *slash = w->current_dir;
                    char *last = NULL;
                    while (*slash) { if (*slash == '/') last = slash; slash++; }
                    if (last && last != w->current_dir) *last = 0;
                    else strcpy(w->current_dir, "/");
                    filebrowser_refresh(w);
                    return;
                }
                if (is_dir_name(w->entries[i].name)) {
                    if (strcmp(w->current_dir, "/") != 0) strcat(w->current_dir, "/");
                    strcat(w->current_dir, w->entries[i].name);
                    filebrowser_refresh(w);
                }
                return;
            }
        }
    } else if (w->type == WTYPE_CALC) {
        calc_click(w);
    } else if (w->type == WTYPE_PAINT) {
        int cx = mouse_x - (w->x + 10);
        int cy = mouse_y - (w->y + TITLE_H + 10);
        if (cx >= 0 && cx < 512 && cy >= 0 && cy < 320) {
            w->painting = 1;
            painting_win = idx;
            paint_dot(w, cx, cy);
            return;
        }
        uint32_t palette[8] = {
            0x2C3E50, 0xE74C3C, 0xE67E22, 0xF1C40F,
            0x2ECC71, 0x3498DB, 0x9B59B6, 0xFFFFFF,
        };
        for (int p = 0; p < 8; p++) {
            if (in_rect(mouse_x, mouse_y, w->x + 10 + p * 38, w->y + w->h - 44, 32, 32)) {
                w->paint_color = palette[p];
                return;
            }
        }
        if (in_rect(mouse_x, mouse_y, w->x + 10 + 8 * 38 + 8, w->y + w->h - 44, 70, 32)) {
            paint_clear(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 10 + 8 * 38 + 84, w->y + w->h - 44, 36, 32)) {
            if (w->brush_size > 1) w->brush_size--;
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 10 + 8 * 38 + 170, w->y + w->h - 44, 36, 32)) {
            if (w->brush_size < 12) w->brush_size++;
            return;
        }
    }
}

static void handle_menu_click(void) {
    int item = menu_item_at();
    if (item >= 0) {
        menu_open = 0;
        if (item == NUM_APPS - 1) {
            sb_terminate(0);
            return;
        }
        create_window(apps[item].type, apps[item].name, apps[item].x, apps[item].y, apps[item].w, apps[item].h);
    }
}

static int icon_at(void) {
    int xs[6] = {40, 170, 300, 40, 170, 300};
    int ys[6] = {60, 60, 60, 180, 180, 180};
    for (int i = 0; i < 6; i++) {
        if (in_rect(mouse_x, mouse_y, xs[i], ys[i], 48, 40) ||
            in_rect(mouse_x, mouse_y, xs[i], ys[i] + 40, 48, 16)) {
            return i;
        }
    }
    return -1;
}

static uint64_t last_click_time = 0;
static int last_click_icon = -1;
static uint64_t last_click_win = -1;

static void handle_mouse_press(void) {
    mouse_btn_down = 1;
    uint64_t now = sys_times(0);

    if (menu_open) {
        handle_menu_click();
        return;
    }

    if (mouse_y >= SCREEN_HEIGHT - 40) {
        if (in_rect(mouse_x, mouse_y, 10, SCREEN_HEIGHT - 35, 80, 30)) {
            menu_open = !menu_open;
            return;
        }
        int tab = taskbar_tab_at();
        if (tab >= 0) {
            window_t *w = &windows[tab];
            if (w->minimized) {
                w->minimized = 0;
                raise_window(tab);
            } else if (tab == top_window()) {
                w->minimized = 1;
            } else {
                raise_window(tab);
            }
            return;
        }
        menu_open = 0;
        return;
    }

    int ic = icon_at();
    if (ic >= 0) {
        if (last_click_icon == ic && now - last_click_time < 30) {
            int types[6] = {WTYPE_TERMINAL, WTYPE_FILE_BRO, WTYPE_SYS_MON, WTYPE_CALC, WTYPE_EDITOR, WTYPE_PAINT};
            int xs[6] = {200, 120, 220, 350, 150, 200};
            int ys[6] = {150, 120, 180, 120, 100, 100};
            create_window(types[ic], apps[0].name, xs[ic], ys[ic], apps[0].w, apps[0].h);
            if (ic == 0) windows[num_windows - 1].title[0] = 'T';
            last_click_icon = -1;
        } else {
            last_click_icon = ic;
            last_click_time = now;
        }
        return;
    }
    last_click_icon = -1;

    for (int i = num_windows - 1; i >= 0; i--) {
        window_t *w = &windows[i];
        if (w->minimized) continue;

        if (in_rect(mouse_x, mouse_y, w->x, w->y, w->w, TITLE_H)) {
            if (in_rect(mouse_x, mouse_y, w->x + w->w - 24, w->y, 24, TITLE_H)) {
                close_window(i);
                return;
            }
            if (in_rect(mouse_x, mouse_y, w->x + w->w - 48, w->y, 24, TITLE_H)) {
                toggle_maximize(w);
                raise_window(i);
                return;
            }
            if (in_rect(mouse_x, mouse_y, w->x + w->w - 72, w->y, 24, TITLE_H)) {
                w->minimized = 1;
                return;
            }
            if (i != top_window()) raise_window(i);
            w = &windows[top_window()];
            if (last_click_win == w->id && now - last_click_time < 30) {
                toggle_maximize(w);
                last_click_win = -1;
                return;
            }
            last_click_win = w->id;
            last_click_time = now;
            drag_win = top_window();
            drag_off_x = mouse_x - w->x;
            drag_off_y = mouse_y - w->y;
            return;
        }

        if (in_rect(mouse_x, mouse_y, w->x, w->y, w->w, w->h)) {
            if (in_rect(mouse_x, mouse_y, w->x + w->w - 10, w->y + w->h - 10, 10, 10)) {
                resize_win = i;
                raise_window(i);
                return;
            }
            if (i != top_window()) raise_window(i);
            w = &windows[top_window()];
            handle_app_click(w, top_window());
            return;
        }
    }

    menu_open = 0;
}

static void handle_mouse_release(void) {
    mouse_btn_down = 0;
    if (drag_win >= 0) drag_win = -1;
    if (resize_win >= 0) resize_win = -1;
    if (painting_win >= 0) {
        windows[painting_win].painting = 0;
        painting_win = -1;
    }
}

static void handle_snake_key(window_t *w, char ch, char code) {
    if (ch == 'w' || code == KSC_UP) { if (w->snake_dir != 2) w->snake_dir = 0; }
    if (ch == 'd' || code == KSC_RIGHT) { if (w->snake_dir != 3) w->snake_dir = 1; }
    if (ch == 's' || code == KSC_DOWN) { if (w->snake_dir != 0) w->snake_dir = 2; }
    if (ch == 'a' || code == KSC_LEFT) { if (w->snake_dir != 1) w->snake_dir = 3; }
    if ((ch == 'r' || ch == 'R') && w->snake_dead) {
        w->snake_len = 3;
        w->snake_x[0] = 10; w->snake_y[0] = 10;
        w->snake_x[1] = 9;  w->snake_y[1] = 10;
        w->snake_x[2] = 8;  w->snake_y[2] = 10;
        w->snake_dir = 1;
        w->food_x = 15; w->food_y = 10;
        w->snake_dead = 0;
    }
}

static void handle_key(char code, char ch) {
    if (alt_pressed && code == 0x3E && num_windows > 0) {
        close_window(top_window());
        return;
    }
    if (alt_pressed && code == 0x0F && num_windows > 1) {
        window_t tmp = windows[num_windows - 1];
        for (int i = num_windows - 1; i > 0; i--) windows[i] = windows[i - 1];
        windows[0] = tmp;
        for (int i = 0; i < num_windows; i++) windows[i].id = i;
        return;
    }
    if (ctrl_pressed && alt_pressed && ch >= 'a' && ch <= 'z') {
        int types[26];
        for (int i = 0; i < 26; i++) types[i] = -1;
        types['t' - 'a'] = WTYPE_TERMINAL;
        types['f' - 'a'] = WTYPE_FILE_BRO;
        types['m' - 'a'] = WTYPE_SYS_MON;
        types['v' - 'a'] = WTYPE_VIEWER;
        types['c' - 'a'] = WTYPE_CALC;
        types['e' - 'a'] = WTYPE_EDITOR;
        types['p' - 'a'] = WTYPE_PAINT;
        types['x' - 'a'] = WTYPE_PROCMON;
        types['h' - 'a'] = WTYPE_HEXVIEW;
        types['y' - 'a'] = WTYPE_TETRIS;
        types['g' - 'a'] = WTYPE_G2048;
        types['b' - 'a'] = WTYPE_MANDEL;
        types['k' - 'a'] = WTYPE_CLOCK;
        types['o' - 'a'] = WTYPE_FORTUNE;
        types['a' - 'a'] = WTYPE_PONG;
        types['r' - 'a'] = WTYPE_MATRIX;
        types['s' - 'a'] = WTYPE_SNAKE;
        types['w' - 'a'] = WTYPE_MEMVIEW;
        types['u' - 'a'] = WTYPE_ABOUT;
        int t = types[ch - 'a'];
        if (t >= 0) {
            for (int i = 0; i < NUM_APPS; i++) {
                if (apps[i].type == t) {
                    create_window(t, apps[i].name, apps[i].x, apps[i].y, apps[i].w, apps[i].h);
                    break;
                }
            }
        }
        return;
    }

    if (num_windows == 0) return;
    int idx = top_window();
    window_t *w = &windows[idx];
    if (w->minimized) return;

    if (w->type == WTYPE_SNAKE) {
        handle_snake_key(w, ch, code);
        return;
    }
    if (w->type == WTYPE_TETRIS) {
        if (code == KSC_LEFT) tetris_move(w, -1, 0);
        else if (code == KSC_RIGHT) tetris_move(w, 1, 0);
        else if (code == KSC_DOWN) tetris_move(w, 0, 1);
        else if (code == KSC_UP) tetris_rotate(w);
        else if (ch == ' ') tetris_drop(w);
        else if (ch == 'r' || ch == 'R') tetris_init(w);
        return;
    }
    if (w->type == WTYPE_G2048) {
        if (code == KSC_LEFT) g2048_move(w, 0);
        else if (code == KSC_RIGHT) g2048_move(w, 1);
        else if (code == KSC_UP) g2048_move(w, 2);
        else if (code == KSC_DOWN) g2048_move(w, 3);
        else if (ch == 'r' || ch == 'R') g2048_init(w);
        return;
    }
    if (w->type == WTYPE_PONG) {
        if (ch == 'w' || ch == 'W' || code == KSC_UP) { w->pong_up = 1; w->pong_down = 0; }
        if (ch == 's' || ch == 'S' || code == KSC_DOWN) { w->pong_down = 1; w->pong_up = 0; }
        if (ch == ' ' && w->pong_over) {
            pong_init(w);
            pong_serve(w, 1);
        }
        return;
    }
    if (w->type == WTYPE_FORTUNE) {
        if (ch == 'n' || ch == 'N') fortune_next(w);
        return;
    }
    if (w->type == WTYPE_PROCMON) {
        if (code == KSC_UP) w->proc_scroll--;
        else if (code == KSC_DOWN) w->proc_scroll++;
        if (w->proc_scroll < 0) w->proc_scroll = 0;
        return;
    }
    if (w->type == WTYPE_HEXVIEW) {
        if (code == KSC_PGUP) w->hex_offset -= 16 * 8;
        else if (code == KSC_PGDN) w->hex_offset += 16 * 8;
        else if (code == KSC_UP) w->hex_offset -= 16;
        else if (code == KSC_DOWN) w->hex_offset += 16;
        if (w->hex_offset < 0) w->hex_offset = 0;
        if (w->hex_offset > w->hex_size) w->hex_offset = w->hex_size;
        return;
    }
    if (w->type == WTYPE_CALC) {
        calc_input(w, ch);
        return;
    }

    if (w->type == WTYPE_TERMINAL || w->type == WTYPE_EDITOR) {
        if (ctrl_pressed && ch == 's') {
            editor_save(w);
            return;
        }
        if (ch == '\n') {
            if (w->type == WTYPE_EDITOR) editor_newline(w);
            else text_newline(w);
        } else if (ch == '\b') {
            text_backspace(w);
        } else if (code == KSC_DEL) {
            text_delete(w);
        } else if (code == KSC_LEFT) {
            if (w->cursor_x > 0) w->cursor_x--;
        } else if (code == KSC_RIGHT) {
            if (w->cursor_x < line_len(w, w->cursor_y)) w->cursor_x++;
        } else if (code == KSC_UP) {
            if (w->cursor_y > 0) w->cursor_y--;
        } else if (code == KSC_DOWN) {
            if (w->cursor_y < 23) w->cursor_y++;
        } else if (code == KSC_HOME) {
            w->cursor_x = 0;
        } else if (code == KSC_END) {
            w->cursor_x = line_len(w, w->cursor_y);
        } else if (code == KSC_PGUP) {
            w->cursor_y = 0;
        } else if (code == KSC_PGDN) {
            w->cursor_y = 23;
        } else if (ctrl_pressed && ch == 'l') {
            for (int i = 0; i < 24 * 60; i++) w->text[i] = 0;
            w->cursor_x = 0;
            w->cursor_y = 0;
        } else if (ch >= 32 && ch < 127) {
            if (w->type == WTYPE_EDITOR) editor_insert_char(w, ch);
            else text_put(w, ch);
        }
        return;
    }
}

static void update_games(void) {
    uint64_t now = sys_times(0);
    for (int i = 0; i < num_windows; i++) {
        window_t *w = &windows[i];

        if (w->type == WTYPE_SYS_MON) {
            if (now - w->last_update > 50) {
                w->last_update = now;
            }
        } else if (w->type == WTYPE_SNAKE && !w->snake_dead && !w->minimized) {
            if (now - w->last_update > 15) {
                w->last_update = now;
                for (int j = w->snake_len - 1; j > 0; j--) {
                    w->snake_x[j] = w->snake_x[j - 1];
                    w->snake_y[j] = w->snake_y[j - 1];
                }
                if (w->snake_dir == 0) w->snake_y[0]--;
                if (w->snake_dir == 1) w->snake_x[0]++;
                if (w->snake_dir == 2) w->snake_y[0]++;
                if (w->snake_dir == 3) w->snake_x[0]--;
                int max_x = (w->w - 4) / 10;
                int max_y = (w->h - TITLE_H) / 10;
                if (w->snake_x[0] < 0 || w->snake_x[0] >= max_x ||
                    w->snake_y[0] < 0 || w->snake_y[0] >= max_y) {
                    w->snake_dead = 1;
                }
                for (int j = 1; j < w->snake_len; j++) {
                    if (w->snake_x[0] == w->snake_x[j] && w->snake_y[0] == w->snake_y[j]) {
                        w->snake_dead = 1;
                    }
                }
                if (w->snake_x[0] == w->food_x && w->snake_y[0] == w->food_y) {
                    if (w->snake_len < 64) w->snake_len++;
                    w->food_x = (int)(rnd() % max_x);
                    w->food_y = (int)(rnd() % max_y);
                }
            }
        } else if (w->type == WTYPE_TETRIS && !w->tetris_over && !w->minimized) {
            int speed = 30 - w->tetris_lines / 10;
            if (speed < 10) speed = 10;
            if (now - w->last_update > speed) {
                w->last_update = now;
                tetris_move(w, 0, 1);
            }
        } else if (w->type == WTYPE_PONG && !w->pong_over && !w->minimized) {
            if (now - w->last_update > 2) {
                w->last_update = now;
                if (w->pong_up && w->pong_py > 0) w->pong_py -= 3;
                if (w->pong_down && w->pong_py < w->h - 60 - TITLE_H) w->pong_py += 3;
                if (w->pong_by < w->pong_ai + 30 && w->pong_ai > 0) w->pong_ai -= 2;
                else if (w->pong_by > w->pong_ai + 30 && w->pong_ai < w->h - 60 - TITLE_H) w->pong_ai += 2;
                w->pong_bx += w->pong_vx;
                w->pong_by += w->pong_vy;
                if (w->pong_by < 10 || w->pong_by > w->h - TITLE_H - 10) w->pong_vy = -w->pong_vy;
                if (w->pong_vx < 0 && w->pong_bx > 24 && w->pong_bx < 34 &&
                    w->pong_by > w->pong_py - 5 && w->pong_by < w->pong_py + 65) {
                    w->pong_vx = -w->pong_vx;
                    w->pong_vy = ((w->pong_by - (w->pong_py + 30)) * 3) / 40;
                    if (w->pong_vy == 0) w->pong_vy = (rnd() % 2) ? 2 : -2;
                }
                if (w->pong_vx > 0 && w->pong_bx > w->w - 42 && w->pong_bx < w->w - 30 &&
                    w->pong_by > w->pong_ai - 5 && w->pong_by < w->pong_ai + 65) {
                    w->pong_vx = -w->pong_vx;
                    w->pong_vy = ((w->pong_by - (w->pong_ai + 30)) * 3) / 40;
                    if (w->pong_vy == 0) w->pong_vy = (rnd() % 2) ? 2 : -2;
                }
                if (w->pong_bx < -10) {
                    w->pong_s2++;
                    if (w->pong_s2 >= 7) w->pong_over = 1;
                    else pong_serve(w, 1);
                } else if (w->pong_bx > w->w + 10) {
                    w->pong_s1++;
                    if (w->pong_s1 >= 7) w->pong_over = 1;
                    else pong_serve(w, -1);
                }
            }
        } else if (w->type == WTYPE_MATRIX && !w->minimized) {
            if (now - w->last_update > 2) {
                w->last_update = now;
                for (int c = 0; c < 40; c++) {
                    if ((int)(rnd() % 4) == 0) {
                        w->matrix_off[c]++;
                        w->matrix_char[c] = "ABCDEF0123456789<>!?*$#"[rnd() % 19];
                        if (w->matrix_off[c] > 24) w->matrix_off[c] = 0;
                    }
                }
            }
        } else if (w->type == WTYPE_MANDEL && !w->minimized) {
            int bh = w->h - TITLE_H - 20;
            if (w->mandel_buf && w->mandel_ready < bh) {
                mandel_compute_rows(w, 16);
            }
        }
    }
}

void _start(void) {
    if (syscall0(SYS_FB_MMAP) < 0) syscall1(SB_TERMINATE, 1);

    backbuffer = (uint32_t *)sys_sbrk(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    if ((int64_t)backbuffer < 0 || !backbuffer) {
        syscall1(SB_TERMINATE, 2);
    }

    wallpaper_buffer = (uint32_t *)sys_sbrk(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    if ((int64_t)wallpaper_buffer < 0 || !wallpaper_buffer) {
        syscall1(SB_TERMINATE, 2);
    }
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint32_t rb = (70 + (y * 70 / SCREEN_HEIGHT)) << 16;
        uint32_t g = (30 + (y * 40 / SCREEN_HEIGHT)) << 8;
        uint32_t b = (150 + (y * 150 / SCREEN_HEIGHT));
        uint32_t color = rb | g | b;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            wallpaper_buffer[y * SCREEN_WIDTH + x] = color;
        }
    }

    int wp_fd = sb_acquire("/wallpaper.bmp", 0);
    int loaded = 0;
    if (wp_fd >= 0) {
        uint8_t header[54];
        if (sb_pull(wp_fd, header, 54) == 54) {
            if (header[0] == 'B' && header[1] == 'M') {
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
                loaded = 1;
            }
        }
        sb_release(wp_fd);
    }

    int lg_fd = sb_acquire("/logo.bmp", 0);
    if (lg_fd >= 0) {
        logo_buffer = (uint32_t *)sys_sbrk(256 * 256 * 4);
        uint8_t header[54];
        if (sb_pull(lg_fd, header, 54) == 54 && header[0] == 'B' && header[1] == 'M') {
            uint32_t offset = *(uint32_t *)&header[10];
            int w = *(int32_t *)&header[18];
            int h = *(int32_t *)&header[22];
            if (w == 256 && h == 256) {
                if (offset > 54) {
                    uint8_t dummy[128];
                    int to_skip = offset - 54;
                    while (to_skip > 0) {
                        int chunk = to_skip > 128 ? 128 : to_skip;
                        sb_pull(lg_fd, dummy, chunk);
                        to_skip -= chunk;
                    }
                }
                uint8_t row_buf[256 * 3 + 32];
                int row_bytes = (w * 3 + 3) & ~3;
                for (int y = h - 1; y >= 0; y--) {
                    sb_pull(lg_fd, row_buf, row_bytes);
                    for (int x = 0; x < w; x++) {
                        uint8_t b = row_buf[x * 3];
                        uint8_t g = row_buf[x * 3 + 1];
                        uint8_t r = row_buf[x * 3 + 2];
                        logo_buffer[y * w + x] = (r << 16) | (g << 8) | b;
                    }
                }
            }
        }
        sb_release(lg_fd);
    }
    if (!loaded) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            uint32_t rb = (50 + (y * 50 / SCREEN_HEIGHT)) << 16;
            uint32_t g = (10 + (y * 30 / SCREEN_HEIGHT)) << 8;
            uint32_t b = (100 + (y * 100 / SCREEN_HEIGHT));
            uint32_t color = rb | g | b;
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                wallpaper_buffer[y * SCREEN_WIDTH + x] = color;
            }
        }
    }

    rng_state = (uint32_t)sys_times(0) ^ 0x9E3779B9;

    create_window(WTYPE_ABOUT, "Welcome to ShadowBox OS", 350, 200, 320, 180);
    create_window(WTYPE_SYS_MON, "System Monitor", 100, 100, 370, 220);

    draw_desktop();

    int input_fd = sb_acquire("/dev/input", O_NONBLOCK);
    if (input_fd < 0) syscall1(SB_TERMINATE, 1);

    uint64_t last_draw_time = sys_times(0);
    int dirty = 1;

    while (1) {
        input_event_t ev;
        while (sb_pull(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == 2) {
                mouse_x += ev.x;
                mouse_y += ev.y;
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > SCREEN_WIDTH - 2) mouse_x = SCREEN_WIDTH - 2;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > SCREEN_HEIGHT - 2) mouse_y = SCREEN_HEIGHT - 2;

                if (drag_win >= 0) {
                    windows[drag_win].x = mouse_x - drag_off_x;
                    windows[drag_win].y = mouse_y - drag_off_y;
                    if (windows[drag_win].x < -windows[drag_win].w + 40) windows[drag_win].x = -windows[drag_win].w + 40;
                    if (windows[drag_win].y < 0) windows[drag_win].y = 0;
                }
                if (resize_win >= 0) {
                    window_t *w = &windows[resize_win];
                    int nw = mouse_x - w->x;
                    int nh = mouse_y - w->y;
                    if (nw < 120) nw = 120;
                    if (nh < 80) nh = 80;
                    w->w = nw;
                    w->h = nh;
                    if (w->type == WTYPE_TETRIS) {
                        int cell = 20;
                        w->w = 10 + cell * 10 + 10;
                        w->h = TITLE_H + 30 + cell * 22 + 4;
                    }
                }
                if (painting_win >= 0) {
                    window_t *w = &windows[painting_win];
                    if (w->type == WTYPE_PAINT && w->painting) {
                        int cx = mouse_x - (w->x + 10);
                        int cy = mouse_y - (w->y + TITLE_H + 10);
                        paint_dot(w, cx, cy);
                    }
                }
                dirty = 1;
            } else if (ev.type == 3) {
                if (ev.code == 0) {
                    if (ev.x) {
                        handle_mouse_press();
                    } else {
                        handle_mouse_release();
                    }
                    dirty = 1;
                }
            } else if (ev.type == 0) {
                char ch = (char)ev.x;
                if (ev.code == KSC_CTRL) ctrl_pressed = 1;
                if (ev.code == KSC_ALT) alt_pressed = 1;
                handle_key((char)ev.code, ch);
                dirty = 1;
            } else if (ev.type == 1) {
                if (ev.code == KSC_CTRL) ctrl_pressed = 0;
                if (ev.code == KSC_ALT) alt_pressed = 0;
                if (num_windows > 0) {
                    window_t *tw = &windows[top_window()];
                    if (tw->type == WTYPE_PONG) {
                        if (ev.code == KSC_UP || ev.code == 0x11) tw->pong_up = 0;
                        if (ev.code == KSC_DOWN || ev.code == 0x1F) tw->pong_down = 0;
                    }
                }
                dirty = 1;
            }
        }

        uint64_t now = sys_times(0);

        if ((now / 50) != (last_draw_time / 50)) dirty = 1;
        if ((now / 100) != (last_draw_time / 100)) dirty = 1;

        update_games();
        for (int i = 0; i < num_windows; i++) {
            if (windows[i].type == WTYPE_SNAKE && !windows[i].snake_dead) dirty = 1;
        }

        if (dirty && (now - last_draw_time >= 3)) {
            draw_desktop();
            last_draw_time = now;
            dirty = 0;
        }

        syscall0(SYS_SCHED_YIELD);
    }
}
