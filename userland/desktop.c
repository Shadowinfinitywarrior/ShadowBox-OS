#include "sys.h"
#include "../gui/c/fb_draw.h"
#define FB_STRIDE (SCREEN_WIDTH * 4)

#include "font.h"
#include "fcntl.h"
#include "icon.h"
#include "desktop_icons.h"
#include "stat.h"
void draw_desktop_icons(void);

/* Forward declarations for drawing helpers used by the tray/notifications. */
static void draw_rect(int x, int y, int w, int h, uint32_t color);
static void draw_char(int x, int y, char c, uint32_t color);
void draw_string(int x, int y, const char *s, uint32_t color);
static void draw_string_limit(int x, int y, const char *s, int max_chars, uint32_t color);
static void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
static void draw_circle(int cx, int cy, int r, uint32_t color, int fill);


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
#define TITLE_H 32  // Increased title bar height
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

static inline int strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    return n == (size_t)-1 ? 0 : *(unsigned char *)a - *(unsigned char *)b;
}

static inline void memset(void *d, int c, uint64_t n) {
    uint8_t *p = (uint8_t *)d;
    while (n--) *p++ = (uint8_t)c;
}

void memcpy(void *d, const void *s, uint64_t n) {
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
#define PAINT_TOOL_BRUSH    0
#define PAINT_TOOL_PENCIL   1
#define PAINT_TOOL_ERASER   2
#define PAINT_TOOL_FILL     3
#define PAINT_TOOL_LINE     4
#define PAINT_TOOL_RECT     5
#define PAINT_TOOL_ELLIPSE  6
#define PAINT_TOOL_SPRAY    7
#define PAINT_NUM_TOOLS     8

#define WTYPE_PROCMON   9
#define WTYPE_HEXVIEW   10
#define WTYPE_TETRIS    11
#define WTYPE_G2048     12
#define WTYPE_MANDEL    13
#define WTYPE_CLOCK     14
#define WTYPE_FORTUNE   15
#define WTYPE_PONG      16
#define WTYPE_MATRIX    17
#define WTYPE_BROWSER   18
#define WTYPE_SETTINGS  20
#define WTYPE_MEMVIEW   25
#define WTYPE_NOTES     30
#define WTYPE_STOPWATCH 31

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

    /* Terminal enhancements */
    char term_history[16][256];  /* Command history */
    int term_history_count;
    int term_history_pos;
    char term_input[256];        /* Current input line */
    int term_input_pos;

    /* System monitor enhancements */
    uint8_t mem_graph[200];     /* Memory usage history */
    uint8_t cpu_graph[200];     /* CPU usage history */
    int graph_pos;              /* Current position in graphs */
    int proc_count;             /* Number of processes */
    int proc_scroll;            /* Process list scroll position */
    int proc_list_dirty;        /* Whether process list needs refresh */
    uint64_t prev_mem_free;
    uint64_t prev_idle_ticks;

    /* Calculator enhancements */
    int64_t calc_memory;        /* Memory value */
    int calc_scientific_mode;   /* Scientific mode toggle */
    double calc_current;         /* Current value for scientific functions */

    struct dirent entries[64];
    int num_entries;
    char current_dir[128];
    char fb_clipboard[256];      // Clipboard for copy/paste
    int fb_clipboard_valid;      // Whether clipboard has valid data
    char fb_status[120];         // Status bar text (properties etc.)

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
    int paint_tool;            // 0=brush,1=pencil,2=eraser,3=fill,4=line,5=rect,6=ellipse,7=spray
    int paint_x0, paint_y0;    // shape start (canvas coords)
    int paint_x1, paint_y1;    // shape current end (canvas coords)
    uint32_t *paint_undo[6];   // undo snapshots (512*320 each)
    int paint_undo_count;
    int paint_undo_pos;

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

    int matrix_off[40];
    int matrix_speed[40];
    int matrix_char[40];

    uint32_t *mandel_buf;
    int mandel_ready;

    int fortune_idx;

    char editor_msg[32];
    uint64_t editor_msg_at;

    int tab_state;

    int term_buf;
    int term_row;
    int term_col;
    int term_scroll;
    int term_esc_state;
    char term_csi[16];
    int term_csi_len;
    int term_csi_param[8];
    int term_csi_nparam;
    int term_fg;
    int term_pipe_fd;
    int term_write_fd;
    int term_pid;
    int term_eof;

    int file_scroll;
    int fb_sel;                // selected entry index
    int fb_sort;               // 0=name, 1=size
    char fb_hist[8][128];      // back history stack
    int fb_hist_len;
    char fb_fwd[8][128];       // forward history stack
    int fb_fwd_len;
    uint64_t fb_sizes[64];     // file sizes (parallel to entries)
    int fb_double_click;       // last click entry for double-click detection
    uint64_t fb_last_click;

    /* For Settings */
    int settings_category;
    int settings_scroll;
    int settings_toggle[32];
    char settings_text[32][32];

    /* For Browser */
    char url_input[256];         /* address bar text being edited */
    char browser_url[256];       /* currently loaded url */
    char browser_content[4096];  /* rendered page text */
    int browser_scroll;
    int url_edit;                /* 1 = editing the address bar */
    int browser_status;          /* 0 idle, 1 loading, 2 error */
    char browser_hist[8][256];   /* back history stack */
    int browser_hist_len;
    char browser_fwd[8][256];    /* forward history stack */
    int browser_fwd_len;

    char notes_text[512];
    int notes_len;
    int notes_scroll;

    int sw_running;
    uint64_t sw_start;
    uint64_t sw_accum;
    uint64_t sw_laps[8];
    int sw_lap_count;
} window_t;

static void paint_fill(window_t *w, int sx, int sy);
static void draw_line_canvas(uint32_t *canvas, int x0, int y0, int x1, int y1, uint32_t color);

static window_t windows[MAX_WINDOWS];
static int num_windows = 0;

#define TERM_RING_ROWS 200
#define TERM_RING_COLS 60
#define TERM_RING_SIZE (TERM_RING_ROWS * TERM_RING_COLS)
#define TERM_MAX_BUFS 16
static char term_ring_storage[TERM_MAX_BUFS][TERM_RING_SIZE];
static uint8_t term_color_storage[TERM_MAX_BUFS][TERM_RING_SIZE];
static int term_buf_used[TERM_MAX_BUFS];

static inline char *term_ring(window_t *w) { return term_ring_storage[w->term_buf]; }
static inline uint8_t *term_color(window_t *w) { return term_color_storage[w->term_buf]; }

static int term_alloc_buf(void) {
    for (int i = 0; i < TERM_MAX_BUFS; i++) {
        if (!term_buf_used[i]) {
            term_buf_used[i] = 1;
            return i;
        }
    }
    return -1;
}

static void term_free_buf(int idx) {
    if (idx >= 0 && idx < TERM_MAX_BUFS) term_buf_used[idx] = 0;
}

static uint32_t *fb = (uint32_t *)0x78000000ULL;
uint32_t *backbuffer = NULL;
static uint32_t *wallpaper_buffer = NULL;
static uint32_t *logo_buffer = NULL;

static int mouse_x = SCREEN_WIDTH / 2;
static int mouse_y = SCREEN_HEIGHT / 2;
static int mouse_btn_down = 0;
static int drag_win = -1;
static int resize_win = -1;
static int drag_off_x = 0;
static int drag_off_y = 0;
static int drag_active = 0;         /* window actually moving (past threshold) */
static int drag_press_x = 0;
static int drag_press_y = 0;
#define DRAG_THRESHOLD 3
static int menu_open = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static int painting_win = -1;

/* Start menu search and recent apps */
static char search_query[32] = {0};
static int recent_apps[5] = {-1, -1, -1, -1, -1};
static int recent_count = 0;
static int search_focused = 0;
static int menu_category = -1;      /* selected start-menu category, -1 = all */

/* App categories */
typedef enum {
    CAT_SYSTEM,
    CAT_TOOLS,
    CAT_GAMES,
    CAT_MEDIA,
    CAT_OTHER
} app_category_t;

static const char* category_names[] = {
    "System", "Tools", "Games", "Media", "Other"
};

/* ------------------------------------------------------------------ */
/* System tray + notifications (taskbar)                              */
/* ------------------------------------------------------------------ */
static sys_status_t sys_status;
static int tray_popup = 0;            /* 0 none, 1 wifi, 2 bluetooth, 3 bell, 4 mem */
#define TRAY_POPUP_W   260
#define TRAY_POPUP_H   210

#define MAX_TOASTS 4
static sys_notify_t notify_list[8];
static int notify_count = 0;
static uint32_t toast_seen[8];        /* notification ids already shown/consumed */
static int toast_seen_count = 0;
static int toast_active[MAX_TOASTS];  /* index into notify_list, -1 empty */
static uint64_t toast_show_tick[MAX_TOASTS];

static uint64_t last_status_tick = 0;
static uint64_t last_notify_tick = 0;

/* Right-aligned tray icon slot positions (x, width) in taskbar */
static int tray_slot_x(int slot) {   /* slot 0=wifi,1=bt,2=bell,3=mem,4=clock */
    switch (slot) {
        case 0: return SCREEN_WIDTH - 220;
        case 1: return SCREEN_WIDTH - 185;
        case 2: return SCREEN_WIDTH - 150;
        case 3: return SCREEN_WIDTH - 115;
        default: return SCREEN_WIDTH - 80;
    }
}
static int tray_slot_width(int slot) { (void)slot; return 28; }

static int tray_slot_at(int mx) {
    for (int s = 0; s < 4; s++)
        if (mx >= tray_slot_x(s) && mx < tray_slot_x(s) + tray_slot_width(s))
            return s;
    return -1;
}

/* ----- tray icon glyphs (drawn into the 40px taskbar) ----- */
static void draw_wifi_icon(int cx, int cy) {
    int st = sys_status.wifi_state;
    uint32_t col = (st == 4) ? 0x2ECC71 : (st == 5 || st == 1 || st == 2) ? 0xECF0F1 : 0x5D6D7E;
    int fill = (st == 4) ? 1 : 0;
    draw_circle(cx, cy + 4, 3, col, fill);
    draw_circle(cx, cy - 1, 5, col, 0);
    draw_circle(cx, cy - 6, 5, col, 0);
    draw_circle(cx, cy - 12, 6, col, 0);
    if (st != 4) { /* small x to indicate off/disconnected */
        draw_line(cx - 2, cy - 14, cx + 2, cy - 10, 0xE74C3C);
        draw_line(cx + 2, cy - 14, cx - 2, cy - 10, 0xE74C3C);
    }
}

static void draw_bt_icon(int x, int y) {
    uint32_t col = sys_status.bt_available ? 0x3498DB : 0x5D6D7E;
    draw_char(x + 9, y + 8, 'B', col);
    draw_line(x + 13, y + 6, x + 18, y + 14, col);
    draw_line(x + 18, y + 14, x + 13, y + 22, col);
    draw_line(x + 13, y + 6, x + 13, y + 22, col);
    draw_line(x + 13, y + 14, x + 22, y + 8, col);
    draw_line(x + 13, y + 14, x + 22, y + 20, col);
}

static void draw_bell_icon(int x, int y, int count) {
    uint32_t col = count > 0 ? 0xF1C40F : 0x5D6D7E;
    draw_circle(x + 14, y + 6, 8, col, 0);
    draw_rect(x + 8, y + 12, 13, 2, col);
    draw_rect(x + 11, y + 14, 7, 2, col);
    draw_rect(x + 12, y + 16, 5, 2, col);
    if (count > 0) {
        char c = (count < 9) ? ('0' + count) : '9';
        draw_char(x + 10, y + 17, c, 0x000000);
        draw_rect(x + 8, y + 16, 13, 11, 0xE74C3C);
        draw_char(x + 10, y + 17, c, 0xFFFFFF);
    }
}

static void draw_mem_icon(int x, int y) {
    uint64_t pct = 0;
    if (sys_status.mem_total) pct = (sys_status.mem_used * 100) / sys_status.mem_total;
    draw_rect(x + 4, y + 6, 20, 16, 0x111111);
    draw_rect(x + 4, y + 6, 20, 1, 0x34495E);
    draw_rect(x + 4, y + 21, 20, 1, 0x34495E);
    int bar = (int)((pct * 14) / 100);
    uint32_t col = (pct > 85) ? 0xE74C3C : (pct > 60) ? 0xF1C40F : 0x2ECC71;
    if (bar > 0) draw_rect(x + 6, y + 16 - bar, 16, bar, col);
}

/* ----- popup panels ----- */
static int popup_open(void) { return tray_popup != 0; }

static void fmt_ip(uint32_t ip, char *out) {
    int n = 0;
    for (int b = 0; b < 4; b++) {
        int v = (ip >> (24 - b * 8)) & 0xFF;
        if (v >= 100) { out[n++] = '0' + v / 100; }
        if (v >= 10) { out[n++] = '0' + (v / 10) % 10; }
        out[n++] = '0' + v % 10;
        if (b < 3) out[n++] = '.';
    }
    out[n] = 0;
}

static void fmt_mac(const uint8_t *m, char *out) {
    int n = 0;
    static const char *hexd = "0123456789ABCDEF";
    for (int b = 0; b < 6; b++) {
        out[n++] = hexd[m[b] >> 4];
        out[n++] = hexd[m[b] & 0xF];
        if (b < 5) out[n++] = ':';
    }
    out[n] = 0;
}

static int popup_rect(int *x, int *y, int *w, int *h) {
    *x = SCREEN_WIDTH - TRAY_POPUP_W - 8;
    *y = SCREEN_HEIGHT - 40 - TRAY_POPUP_H - 6;
    *w = TRAY_POPUP_W;
    *h = TRAY_POPUP_H;
    return 1;
}

static void popup_close_rect(int x, int y, int w, int *cx, int *cy, int *cw, int *ch) {
    *cx = x + w - 20; *cy = y + 6; *cw = 14; *ch = 14;
}

static void draw_popup_panel(void) {
    if (!popup_open()) return;
    int x, y, w, h;
    popup_rect(&x, &y, &w, &h);
    draw_rect(x - 2, y - 2, w + 4, h + 4, 0x111111);
    draw_rect(x, y, w, h, 0x1C2833);
    draw_rect(x, y, w, 2, 0x3498DB);
    int cx, cy, cw, ch;
    popup_close_rect(x, y, w, &cx, &cy, &cw, &ch);
    draw_rect(cx, cy, cw, ch, 0x2C3E50);
    draw_line(cx + 3, cy + 3, cx + cw - 4, cy + ch - 4, 0xECF0F1);
    draw_line(cx + cw - 4, cy + 3, cx + 3, cy + ch - 4, 0xECF0F1);

    if (tray_popup == 1) { /* Network (wired ethernet + Wi-Fi) */
        draw_string(x + 12, y + 12, "Network", 0x3498DB);
        sb_netinfo_t ni;
        int nok = (sb_netinfo(&ni) == 0);
        int yy = y + 36;
        if (nok && ni.link) {
            char line[40];
            draw_string(x + 12, yy, "Ethernet: Connected", 0x2ECC71); yy += 18;
            fmt_ip(ni.ip, line); draw_string(x + 12, yy, "IP", 0x7F8C8D); draw_string(x + 44, yy, line, 0xECF0F1); yy += 15;
            fmt_ip(ni.gateway, line); draw_string(x + 12, yy, "GW", 0x7F8C8D); draw_string(x + 44, yy, line, 0xECF0F1); yy += 15;
            fmt_ip(ni.dns, line); draw_string(x + 12, yy, "DNS", 0x7F8C8D); draw_string(x + 44, yy, line, 0xECF0F1); yy += 15;
            fmt_mac(ni.mac, line); draw_string(x + 12, yy, "MAC", 0x7F8C8D); draw_string(x + 44, yy, line, 0xECF0F1); yy += 20;
        } else {
            draw_string(x + 12, yy, "Ethernet: Link down", 0xE74C3C); yy += 18;
            draw_string(x + 12, yy, "No wired interface", 0x7F8C8D); yy += 20;
        }
        draw_rect(x + 12, yy, w - 24, 1, 0x2C3E50); yy += 10;
        draw_string(x + 12, yy, "Wi-Fi", 0x7F8C8D);
        const char *st = "Off";
        switch (sys_status.wifi_state) {
            case 1: st = "Scanning..."; break;
            case 2: st = "Connecting..."; break;
            case 3: st = "Associated"; break;
            case 4: st = "Connected"; break;
            case 5: st = "Disconnected"; break;
            default: st = "No adapter"; break;
        }
        draw_string(x + 80, yy, st, 0xECF0F1); yy += 16;
        if (sys_status.wifi_state == 4 && sys_status.wifi_ssid[0]) {
            draw_string(x + 12, yy, sys_status.wifi_ssid, 0x2ECC71); yy += 16;
        }
        draw_string(x + 12, y + h - 16, "Wi-Fi is managed by the kernel", 0x5D6D7E);
    } else if (tray_popup == 2) { /* Bluetooth */
        draw_string(x + 12, y + 12, "Bluetooth", 0x3498DB);
        draw_string(x + 12, y + 36, sys_status.bt_available ? "On" : "No adapter", 0xECF0F1);
        char d[24];
        int i = 0;
        d[i++] = '0' + sys_status.bt_devices % 10;
        d[i++] = ' '; d[i++] = 'd'; d[i++] = 'e'; d[i++] = 'v'; d[i++] = 'i'; d[i++] = 'c'; d[i++] = 'e'; d[i++] = 's'; d[i] = 0;
        draw_string(x + 12, y + 54, d, 0x7F8C8D);
        draw_string(x + 12, y + 78, "No paired devices", 0x5D6D7E);
    } else if (tray_popup == 3) { /* Notifications */
        draw_string(x + 12, y + 12, "Notifications", 0x3498DB);
        int yy = y + 36;
        if (notify_count == 0) {
            draw_string(x + 12, yy, "No notifications", 0x5D6D7E);
        }
        for (int i = 0; i < notify_count && yy + 16 < y + h - 8; i++) {
            sys_notify_t *n = &notify_list[i];
            draw_string_limit(x + 12, yy, n->app_name, 20, 0x3498DB);
            yy += 14;
            draw_string_limit(x + 12, yy, n->summary, 28, 0xECF0F1);
            yy += 16;
        }
        draw_string(x + 12, y + h - 22, "Click a toast to dismiss", 0x5D6D7E);
    } else if (tray_popup == 4) { /* Memory */
        draw_string(x + 12, y + 12, "Memory", 0x3498DB);
        uint64_t used_kb = sys_status.mem_used / 1024;
        uint64_t total_kb = sys_status.mem_total / 1024;
        char line[40];
        int i = 0;
        char num[24];
        /* used KB -> MB */
        used_kb /= 1024; total_kb /= 1024;
        /* print used */
        { uint64_t v = used_kb; int digits = 0; uint64_t t = v; do { digits++; t /= 10; } while (t);
          for (int k = digits - 1; k >= 0; k--) { num[k] = '0' + v % 10; v /= 10; } num[digits] = 0; }
        i = 0;
        for (int k = 0; num[k]; k++) line[i++] = num[k];
        const char *used = " MB used of ";
        for (int k = 0; used[k]; k++) line[i++] = used[k];
        { uint64_t v = total_kb; int digits = 0; uint64_t t = v; do { digits++; t /= 10; } while (t);
          for (int k = digits - 1; k >= 0; k--) { num[k] = '0' + v % 10; v /= 10; } num[digits] = 0; }
        for (int k = 0; num[k]; k++) line[i++] = num[k];
        const char *suffix = " MB";
        for (int k = 0; suffix[k]; k++) line[i++] = suffix[k];
        line[i] = 0;
        draw_string(x + 12, y + 36, line, 0xECF0F1);
        uint64_t pct = sys_status.mem_total ? (sys_status.mem_used * 100) / sys_status.mem_total : 0;
        /* percentage bar */
        draw_rect(x + 12, y + 60, w - 24, 8, 0x111111);
        int bw = (int)((pct * (w - 24)) / 100);
        if (bw > 0) draw_rect(x + 12, y + 60, bw, 8, pct > 85 ? 0xE74C3C : (pct > 60 ? 0xF1C40F : 0x2ECC71));
        /* uptime */
        uint64_t sec = sys_status.uptime_ticks / 100;
        draw_string(x + 12, y + 80, "Uptime", 0x7F8C8D);
        char up[24];
        up[0] = 0;
        i = 0;
        { uint64_t m = sec / 60; uint64_t h = m / 60; m %= 60;
          if (h) { int d0 = '0' + h / 10, d1 = '0' + h % 10; up[i++] = (char)d0; up[i++] = (char)d1; up[i++] = 'h'; }
          up[i++] = (char)('0' + m / 10); up[i++] = (char)('0' + m % 10); up[i++] = 'm'; up[i] = 0; }
        draw_string(x + 76, y + 80, up, 0xECF0F1);
    }
}

/* ----- notification toasts (top-right) ----- */
static int toast_slot_y(int idx) { return 10 + idx * 74; }

static void draw_toasts(void) {
    for (int i = 0; i < MAX_TOASTS; i++) {
        if (toast_active[i] < 0) continue;
        sys_notify_t *n = &notify_list[toast_active[i]];
        uint64_t now = sys_times(0);
        if (now - toast_show_tick[i] > 800) { /* ~8s auto-expire (100 ticks/s) */
            toast_active[i] = -1;
            sys_notify_dismiss(n->id);
            continue;
        }
        int x = SCREEN_WIDTH - 330;
        int y = toast_slot_y(i);
        int w = 320, h = 66;
        draw_rect(x - 1, y - 1, w + 2, h + 2, 0x111111);
        draw_rect(x, y, w, h, 0x1C2833);
        draw_rect(x, y, w, 2, n->priority >= 2 ? 0xE74C3C : 0x3498DB);
        draw_string(x + 10, y + 8, n->app_name, 0x3498DB);
        draw_string_limit(x + 10, y + 26, n->summary, 36, 0xECF0F1);
        draw_string_limit(x + 10, y + 44, n->body, 36, 0x7F8C8D);
        draw_char(x + w - 20, y + 4, 'x', 0x7F8C8D);
    }
}

static void poll_status(void) {
    uint64_t now = sys_times(0);
    if (now - last_status_tick >= 25) {
        last_status_tick = now;
        sys_sys_status(&sys_status);
    }
    if (now - last_notify_tick >= 10) {
        last_notify_tick = now;
        sys_notify_t tmp[8];
        int n = sys_notify_peek(tmp, 8);
        notify_count = n > 0 ? n : 0;
        for (int i = 0; i < notify_count; i++) notify_list[i] = tmp[i];
        /* Promote unseen notifications to toasts */
        for (int i = 0; i < notify_count; i++) {
            int seen = 0;
            for (int j = 0; j < toast_seen_count; j++)
                if (toast_seen[j] == notify_list[i].id) { seen = 1; break; }
            if (seen) continue;
            for (int j = 0; j < MAX_TOASTS; j++) {
                if (toast_active[j] < 0) {
                    toast_active[j] = i;
                    toast_show_tick[j] = now;
                    break;
                }
            }
            toast_seen[toast_seen_count % 8] = notify_list[i].id;
            if (toast_seen_count < 8) toast_seen_count++;
        }
        /* Drop toasts whose notification was dismissed */
        for (int j = 0; j < MAX_TOASTS; j++) {
            if (toast_active[j] >= 0) {
                int still = 0;
                for (int i = 0; i < notify_count; i++)
                    if (notify_list[i].id == notify_list[toast_active[j]].id) { still = 1; break; }
                if (!still) toast_active[j] = -1;
            }
        }
    }
}



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
static const char KSC_BACKSPACE = 0x0E;
static const char KSC_ESC = 0x01;
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
    const uint8_t *bitmap = &font8x16[uc * 16];
    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < 8; i++) {
            if (bitmap[j] & (1 << (7 - i))) {
                if (x + i >= 0 && x + i < SCREEN_WIDTH && y + j >= 0 && y + j < SCREEN_HEIGHT) {
                    backbuffer[(y + j) * SCREEN_WIDTH + (x + i)] = color;
                }
            }
        }
    }
}

void draw_string(int x, int y, const char *s, uint32_t color) {
    while (*s) {
        draw_char(x, y, *s, color);
        x += 8;
        s++;
    }
}

static void fmt_sw(uint64_t ticks, char *buf) {
    uint64_t centi = ticks;  /* system timer runs at 100 Hz */
    uint64_t cs = centi % 100;
    uint64_t ss = (centi / 100) % 60;
    uint64_t mm = (centi / 6000) % 60;
    uint64_t hh = centi / 360000;
    int n = 0;
    buf[n++] = '0' + (hh / 10) % 10; buf[n++] = '0' + hh % 10; buf[n++] = ':';
    buf[n++] = '0' + (mm / 10) % 10; buf[n++] = '0' + mm % 10; buf[n++] = ':';
    buf[n++] = '0' + (ss / 10) % 10; buf[n++] = '0' + ss % 10; buf[n++] = '.';
    buf[n++] = '0' + (cs / 10) % 10; buf[n++] = '0' + cs % 10; buf[n] = 0;
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
    window_t *w = &windows[idx];
    if (w->type == WTYPE_TERMINAL) {
        if (w->term_pid > 0) {
            sys_kill((uint64_t)w->term_pid, 9);
            int status;
            sys_wait4((uint64_t)w->term_pid, &status, 1);
        }
        if (w->term_write_fd >= 0) sb_release(w->term_write_fd);
        if (w->term_pipe_fd >= 0) sb_release(w->term_pipe_fd);
        term_free_buf(w->term_buf);
        w->term_buf = -1;
    }
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
    const char *icon;   // /icons/<name>.bmp path, NULL/"" for procedural
    app_category_t category;
} app_entry_t;

static const app_entry_t apps[] = {
    {"Terminal", WTYPE_TERMINAL, 150, 150, 560, 360, "/icons/terminal.bmp", CAT_SYSTEM},
    {"Files", WTYPE_FILE_BRO, 100, 100, 330, 330, "/icons/file_explorer.bmp", CAT_SYSTEM},
    {"System Monitor", WTYPE_SYS_MON, 200, 150, 370, 220, "/icons/system_monitor.bmp", CAT_SYSTEM},
    {"Settings", WTYPE_SETTINGS, 250, 200, 620, 420, "/icons/settings.bmp", CAT_SYSTEM},
    {"Image Viewer", WTYPE_VIEWER, 200, 150, 340, 340, "/icons/image_viewer.bmp", CAT_MEDIA},
    {"Snake", WTYPE_SNAKE, 260, 150, 324, 344, "/icons/snake.bmp", CAT_GAMES},
    {"Calculator", WTYPE_CALC, 350, 120, 250, 340, "/icons/calculator.bmp", CAT_TOOLS},
    {"Editor", WTYPE_EDITOR, 150, 100, 650, 430, "/icons/text_editor.bmp", CAT_TOOLS},
    {"Paint", WTYPE_PAINT, 200, 100, 560, 470, "/icons/paint.bmp", CAT_MEDIA},
    {"Processes", WTYPE_PROCMON, 250, 100, 450, 330, "/icons/process_monitor.bmp", CAT_SYSTEM},
    {"Hex Viewer", WTYPE_HEXVIEW, 200, 100, 520, 410, "/icons/hex_viewer.bmp", CAT_TOOLS},
    {"Tetris", WTYPE_TETRIS, 320, 80, 270, 500, "/icons/tetris.bmp", CAT_GAMES},
    {"2048", WTYPE_G2048, 300, 120, 320, 390, "/icons/calculator.bmp", CAT_GAMES},
    {"Mandelbrot", WTYPE_MANDEL, 200, 100, 500, 420, "/icons/mandelbrot.bmp", CAT_MEDIA},
    {"Clock", WTYPE_CLOCK, 420, 200, 230, 240, "/icons/clock.bmp", CAT_TOOLS},
    {"Fortune", WTYPE_FORTUNE, 300, 200, 430, 250, "/icons/fortune.bmp", CAT_TOOLS},
    {"Pong", WTYPE_PONG, 280, 120, 530, 330, "/icons/pong.bmp", CAT_GAMES},
    {"Matrix", WTYPE_MATRIX, 300, 100, 340, 410, "/icons/matrix_rain.bmp", CAT_MEDIA},
    {"Memory", WTYPE_MEMVIEW, 250, 150, 400, 270, "/icons/memory_viewer.bmp", CAT_SYSTEM},
    {"Browser", WTYPE_BROWSER, 200, 120, 640, 460, "/icons/browser.bmp", CAT_TOOLS},
    {"Notes", WTYPE_NOTES, 150, 100, 420, 360, "/icons/text_editor.bmp", CAT_TOOLS},
    {"Stopwatch", WTYPE_STOPWATCH, 280, 140, 300, 360, "/icons/clock.bmp", CAT_TOOLS},
    {"About", WTYPE_ABOUT, 250, 200, 320, 180, "/icons/about.bmp", CAT_SYSTEM},
    {"Shutdown", -1, 0, 0, 0, 0, "/icons/keyboard_power.bmp", CAT_SYSTEM},
};

#define NUM_APPS ((int)(sizeof(apps) / sizeof(apps[0])))

/* Start menu layout (Y offsets relative to MENU_MY) */
static const int MENU_MX = 10;
static const int MENU_COL_W = 48;
static const int MENU_COL_GAP = 12;
static const int MENU_COLS = 4;
static const int MENU_ROWS = (NUM_APPS - 1 + MENU_COLS - 1) / MENU_COLS;
static const int MENU_MW = 20 + MENU_COLS * (MENU_COL_W + MENU_COL_GAP) + MENU_COL_GAP;
#define MENU_SEARCH_Y  56
#define MENU_RECENT_Y  92
#define MENU_CAT_Y     156
#define MENU_GRID_Y    208
#define MENU_ICON_SIZE2 48
#define MENU_ICON_GAP  12
#define MENU_ROW_H     (MENU_ICON_SIZE2 + 20)
#define MENU_MH (MENU_GRID_Y + MENU_ROWS * MENU_ROW_H + 56)
#define MENU_MY (SCREEN_HEIGHT - 48 - MENU_MH)

/* menu_item_at() / handle_menu_click() return codes */
#define MENU_HIT_NONE     -1
#define MENU_HIT_SEARCH   -2
#define MENU_HIT_SHUTDOWN -100
#define MENU_HIT_RESTART  -101
#define MENU_HIT_SUSPEND  -102
#define MENU_HIT_CAT(i)   (-200 + (i))
#define MENU_HIT_RECENT(i) (-300 + (i))

static int menu_app_idx_at_grid(int grid_pos, int *app_idx) {
    int shown = 0;
    for (int i = 0; i < NUM_APPS - 1; i++) {   /* last entry is Shutdown */
        if (menu_category >= 0 && apps[i].category != menu_category) continue;
        if (search_query[0]) {
            const char *app_name = apps[i].name;
            const char *query = search_query;
            int match = 0;
            while (*app_name) {
                const char *a = app_name;
                const char *q = query;
                int temp_match = 1;
                while (*a && *q) {
                    if ((*a | 0x20) != (*q | 0x20)) { temp_match = 0; break; }
                    a++;
                    q++;
                }
                if (temp_match && !*q) { match = 1; break; }
                app_name++;
            }
            if (!match) continue;
        }
        if (shown == grid_pos) { *app_idx = i; return 1; }
        shown++;
    }
    return 0;
}

static int menu_item_at(void) {
    if (!in_rect(mouse_x, mouse_y, MENU_MX, MENU_MY, MENU_MW, MENU_MH)) return MENU_HIT_NONE;

    /* Search bar */
    if (in_rect(mouse_x, mouse_y, MENU_MX + 16, MENU_MY + MENU_SEARCH_Y, MENU_MW - 32, 26)) {
        return MENU_HIT_SEARCH;
    }

    /* Power row (Shutdown | Restart | Suspend) */
    int power_y = MENU_MY + MENU_MH - 40;
    int pw = (MENU_MW - 24) / 3;
    if (mouse_y >= power_y && mouse_y < power_y + 32) {
        int px = MENU_MX + 12;
        if (mouse_x < px + pw) return MENU_HIT_SHUTDOWN;
        if (mouse_x < px + pw * 2) return MENU_HIT_RESTART;
        return MENU_HIT_SUSPEND;
    }

    /* Recent apps row */
    if (mouse_y >= MENU_MY + MENU_RECENT_Y + 24 && mouse_y < MENU_MY + MENU_RECENT_Y + 72) {
        int rx = MENU_MX + 20;
        for (int i = 0; i < recent_count && i < 5; i++) {
            if (mouse_x >= rx && mouse_x < rx + 40) return MENU_HIT_RECENT(i);
            rx += 40;
        }
        return MENU_HIT_NONE;
    }

    /* Category buttons */
    if (mouse_y >= MENU_MY + MENU_CAT_Y + 24 && mouse_y < MENU_MY + MENU_CAT_Y + 52) {
        int cat_btn_w = (MENU_MW - 32) / 5;
        int cx = MENU_MX + 16;
        for (int i = 0; i < 5; i++) {
            if (mouse_x >= cx && mouse_x < cx + cat_btn_w - 4) return MENU_HIT_CAT(i);
            cx += cat_btn_w;
        }
        return MENU_HIT_NONE;
    }

    /* App grid */
    if (mouse_y < MENU_MY + MENU_GRID_Y) return MENU_HIT_NONE;
    int cols = MENU_COLS;
    int icon_size = MENU_ICON_SIZE2;
    int icon_gap = MENU_ICON_GAP;
    int grid_x = MENU_MX + 20;
    int row = (mouse_y - (MENU_MY + MENU_GRID_Y)) / MENU_ROW_H;
    int col = (mouse_x - grid_x) / (icon_size + icon_gap);
    if (col < 0 || row < 0 || row >= MENU_ROWS) return MENU_HIT_NONE;
    int grid_pos = row * cols + col;
    int app_idx = -1;
    if (menu_app_idx_at_grid(grid_pos, &app_idx)) return app_idx;
    return MENU_HIT_NONE;
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

static void term_clear(window_t *w) {
    memset(term_ring(w), 0, TERM_RING_SIZE);
    memset(term_color(w), 0, TERM_RING_SIZE);
    w->term_row = 0;
    w->term_col = 0;
    w->term_scroll = 0;
    w->term_esc_state = 0;
    w->term_fg = 0;
}

static void term_shift_up(window_t *w) {
    memcpy(term_ring(w), term_ring(w) + TERM_RING_COLS, TERM_RING_COLS * (TERM_RING_ROWS - 1));
    memcpy(term_color(w), term_color(w) + TERM_RING_COLS, TERM_RING_COLS * (TERM_RING_ROWS - 1));
    memset(term_ring(w) + TERM_RING_COLS * (TERM_RING_ROWS - 1), 0, TERM_RING_COLS);
    memset(term_color(w) + TERM_RING_COLS * (TERM_RING_ROWS - 1), 0, TERM_RING_COLS);
}

static void term_put_cell(window_t *w, char c) {
    term_ring(w)[w->term_row * TERM_RING_COLS + w->term_col] = c;
    term_color(w)[w->term_row * TERM_RING_COLS + w->term_col] = (uint8_t)w->term_fg;
    w->term_col++;
    if (w->term_col >= 60) {
        w->term_col = 0;
        w->term_row++;
        if (w->term_row >= 200) {
            term_shift_up(w);
            w->term_row = 199;
        }
    }
}

static void term_apply_sgr(window_t *w) {
    for (int i = 0; i < w->term_csi_nparam; i++) {
        int p = w->term_csi_param[i];
        if (p == 0) w->term_fg = 0;
        else if (p >= 30 && p <= 37) w->term_fg = p - 30 + 1;
        else if (p >= 90 && p <= 97) w->term_fg = p - 90 + 1;
        else if (p == 39) w->term_fg = 0;
    }
}

static void term_putc(window_t *w, char c) {
    switch (w->term_esc_state) {
    case 0:
        if (c == '\033') { w->term_esc_state = 1; return; }
        if (c == '\n') {
            w->term_col = 0;
            w->term_row++;
            if (w->term_row >= 200) { term_shift_up(w); w->term_row = 199; }
            return;
        }
        if (c == '\r') { w->term_col = 0; return; }
        if (c == '\b') {
            if (w->term_col > 0) w->term_col--;
            else if (w->term_row > 0) { w->term_row--; w->term_col = 59; }
            return;
        }
        if (c == '\t') {
            int stop = ((w->term_col / 8) + 1) * 8;
            while (w->term_col < stop && w->term_col < 60) term_put_cell(w, ' ');
            return;
        }
        if (c >= 32 && c < 127) { term_put_cell(w, c); return; }
        return;
    case 1:
        if (c == '[') { w->term_esc_state = 2; w->term_csi_len = 0; w->term_csi_nparam = 0; return; }
        if (c == ']') { w->term_esc_state = 3; return; }
        w->term_esc_state = 0;
        return;
    case 2:
        if (c >= '0' && c <= '9') {
            if (w->term_csi_len < 15) w->term_csi[w->term_csi_len++] = c;
            return;
        }
        if (c == ';') {
            if (w->term_csi_nparam < 8) {
                w->term_csi[w->term_csi_len] = 0;
                w->term_csi_param[w->term_csi_nparam++] = (int)atoi64(w->term_csi);
            }
            w->term_csi_len = 0;
            return;
        }
        if (c == '?') return;
        w->term_csi[w->term_csi_len] = 0;
        if (w->term_csi_nparam < 8) {
            w->term_csi_param[w->term_csi_nparam++] = (int)atoi64(w->term_csi);
        }
        if (c == 'm') term_apply_sgr(w);
        else if (c == 'J') {
            if (w->term_csi_nparam && w->term_csi_param[0] == 2) term_clear(w);
        } else if (c == 'H' || c == 'f') {
            w->term_row = 0; w->term_col = 0;
        } else if (c == 'A') { if (w->term_row > 0) w->term_row--; }
        else if (c == 'B') { if (w->term_row < 199) w->term_row++; }
        else if (c == 'C') { if (w->term_col < 59) w->term_col++; }
        else if (c == 'D') { if (w->term_col > 0) w->term_col--; }
        w->term_esc_state = 0;
        return;
    case 3:
        if (c == 0x07) w->term_esc_state = 0;
        else if (c == '\033') w->term_esc_state = 4;
        return;
    case 4:
        w->term_esc_state = 0;
        return;
    }
}

static int update_terminal(window_t *w) {
    if (w->type != WTYPE_TERMINAL) return 0;
    if (w->term_pipe_fd < 0 || w->term_pid <= 0) return 0;
    char buf[256];
    long n;
    int got = 0;
    while ((n = (long)sb_pull(w->term_pipe_fd, buf, sizeof(buf))) > 0) {
        got = 1;
        for (long i = 0; i < n; i++) term_putc(w, buf[i]);
    }
    if (w->term_eof) return got;
    if (n < 0) {
        w->term_eof = 1;
        return got;
    }
    int status;
    if ((long)sys_wait4((uint64_t)w->term_pid, &status, 1) > 0) {
        w->term_eof = 1;
        got = 1;
        const char *msg = "\r\n[process exited]\r\n";
        for (int i = 0; msg[i]; i++) term_putc(w, msg[i]);
    }
    return got;
}

static int update_terminals(void) {
    int got = 0;
    for (int i = 0; i < num_windows; i++) {
        if (update_terminal(&windows[i])) got = 1;
    }
    return got;
}

static void term_forward(window_t *w, char code, char ch) {
    if (w->term_eof) return;
    if (w->term_write_fd < 0) return;
    update_terminal(w);
    char buf[8];
    int n = 0;

    /* Handle command history and tab completion locally */
    if (code == KSC_UP) {
        if (w->term_history_count > 0 && w->term_history_pos < w->term_history_count) {
            w->term_history_pos++;
            if (w->term_history_pos >= w->term_history_count) {
                w->term_history_pos = w->term_history_count - 1;
            }
            strcpy(w->term_input, w->term_history[w->term_history_count - 1 - w->term_history_pos]);
            w->term_input_pos = strlen(w->term_input);
            /* Clear current line and show history */
            sb_push(w->term_write_fd, "\r\033[K", 4);
            sb_push(w->term_write_fd, w->term_input, w->term_input_pos);
        } else {
            buf[n++] = 27; buf[n++] = '['; buf[n++] = 'A';
            sb_push(w->term_write_fd, buf, n);
        }
        return;
    } else if (code == KSC_DOWN) {
        if (w->term_history_pos > 0) {
            w->term_history_pos--;
            if (w->term_history_pos == 0) {
                w->term_input[0] = 0;
                w->term_input_pos = 0;
            } else {
                strcpy(w->term_input, w->term_history[w->term_history_count - w->term_history_pos]);
                w->term_input_pos = strlen(w->term_input);
            }
            sb_push(w->term_write_fd, "\r\033[K", 4);
            sb_push(w->term_write_fd, w->term_input, w->term_input_pos);
        } else {
            buf[n++] = 27; buf[n++] = '['; buf[n++] = 'B';
            sb_push(w->term_write_fd, buf, n);
        }
        return;
    } else if (ch == '\t') {
        /* Simple tab completion for common commands */
        const char *commands[] = {"ls", "cd", "cat", "echo", "pwd", "help", "clear", "exit"};
        int match = -1;
        int partial_len = strlen(w->term_input);
        for (int i = 0; i < 8; i++) {
            if (strncmp(commands[i], w->term_input, partial_len) == 0) {
                if (match == -1) {
                    match = i;
                } else {
                    match = -1; /* Multiple matches */
                    break;
                }
            }
        }
        if (match >= 0) {
            strcpy(w->term_input, commands[match]);
            w->term_input_pos = strlen(w->term_input);
            sb_push(w->term_write_fd, "\r\033[K", 4);
            sb_push(w->term_write_fd, w->term_input, w->term_input_pos);
        }
        return;
    } else if (ch == '\n') {
        /* Add to history if not empty */
        if (w->term_input[0]) {
            if (w->term_history_count < 16) {
                strcpy(w->term_history[w->term_history_count], w->term_input);
                w->term_history_count++;
            } else {
                for (int i = 0; i < 15; i++) {
                    strcpy(w->term_history[i], w->term_history[i + 1]);
                }
                strcpy(w->term_history[15], w->term_input);
            }
            w->term_history_pos = 0;
        }
        w->term_input[0] = 0;
        w->term_input_pos = 0;
        buf[n++] = '\n';
    } else if (ch == '\b' || ch == 127) {
        if (w->term_input_pos > 0) {
            w->term_input_pos--;
            w->term_input[w->term_input_pos] = 0;
        }
        buf[n++] = '\b';
    } else if (ch >= 32 && ch < 127) {
        if (w->term_input_pos < 255) {
            w->term_input[w->term_input_pos++] = ch;
            w->term_input[w->term_input_pos] = 0;
        }
        buf[n++] = ch;
    } else if (code == KSC_RIGHT) { buf[n++] = 27; buf[n++] = '['; buf[n++] = 'C'; }
    else if (code == KSC_LEFT) { buf[n++] = 27; buf[n++] = '['; buf[n++] = 'D'; }
    else if (code == KSC_HOME) { buf[n++] = 27; buf[n++] = '['; buf[n++] = 'H'; }
    else if (code == KSC_END) { buf[n++] = 27; buf[n++] = '['; buf[n++] = 'F'; }
    else if (ctrl_pressed && ch >= 'a' && ch <= 'z') {
        buf[n++] = ch - 'a' + 1;
    } else {
        return;
    }
    sb_push(w->term_write_fd, buf, n);
}

static void terminal_spawn(window_t *w) {
    int in_pipe[2];
    int out_pipe[2];
    if (sys_pipe(in_pipe) != 0 || sys_pipe(out_pipe) != 0) {
        term_free_buf(w->term_buf);
        w->term_buf = -1;
        w->term_pid = -1;
        return;
    }
    uint64_t pid = sb_replicate();
    if ((int64_t)pid == 0) {
        sys_dup2(in_pipe[0], 0);
        sys_dup2(out_pipe[1], 1);
        sys_dup2(out_pipe[1], 2);
        sb_release(in_pipe[0]);
        sb_release(in_pipe[1]);
        sb_release(out_pipe[0]);
        sb_release(out_pipe[1]);
        char *argv[] = {"shell.elf", 0};
        sb_morph("shell.elf", argv, 0);
        sb_terminate(127);
    }
    if ((int64_t)pid < 0) {
        sb_release(in_pipe[0]);
        sb_release(in_pipe[1]);
        sb_release(out_pipe[0]);
        sb_release(out_pipe[1]);
        return;
    }
    w->term_pipe_fd = out_pipe[0];
    w->term_write_fd = in_pipe[1];
    sb_release(in_pipe[0]);
    sb_release(out_pipe[1]);
    w->term_pid = (int)pid;
    w->term_eof = 0;
}

static void filebrowser_refresh(window_t *w);
static void create_window(int type, const char *title, int x, int y, int w, int h);
static int is_dir_name(const char *n);

static void filebrowser_refresh(window_t *w) {
    w->num_entries = 0;
    w->fb_sel = 0;    strcpy(w->entries[w->num_entries].name, "..");
    w->fb_sizes[w->num_entries] = 0;
    w->num_entries++;
    int fd = sb_acquire(w->current_dir, 0);
    if (fd >= 0) {
        struct dirent d;
        while (sys_getdents(fd, &d, sizeof(d)) > 0 && w->num_entries < 63) {
            if (strcmp(d.name, ".") == 0 || strcmp(d.name, "..") == 0) continue;
            w->entries[w->num_entries] = d;
            char full[300];
            strcpy(full, w->current_dir);
            if (strcmp(w->current_dir, "/") != 0) strcat(full, "/");
            strcat(full, d.name);
            struct stat st;
            if (syscall2(SYS_STAT, (uint64_t)full, (uint64_t)&st) == 0) {
                w->fb_sizes[w->num_entries] = st.st_size;
            } else {
                w->fb_sizes[w->num_entries] = 0;
            }
            w->num_entries++;
        }
        sb_release(fd);
    }
    for (int i = 1; i < w->num_entries - 1; i++) {
        for (int j = 1; j < w->num_entries - 1; j++) {
            int dir_a = is_dir_name(w->entries[j].name);
            int dir_b = is_dir_name(w->entries[j + 1].name);
            int swap = 0;
            if (dir_a != dir_b) {
                swap = dir_b;
            } else if (w->fb_sort == 1) {
                swap = w->fb_sizes[j] > w->fb_sizes[j + 1];
            } else {
                swap = strcmp(w->entries[j].name, w->entries[j + 1].name) > 0;
            }
            if (swap) {
                struct dirent t = w->entries[j];
                w->entries[j] = w->entries[j + 1];
                w->entries[j + 1] = t;
                uint64_t ts = w->fb_sizes[j];
                w->fb_sizes[j] = w->fb_sizes[j + 1];
                w->fb_sizes[j + 1] = ts;
            }
        }
    }
}

static void fb_navigate(window_t *w, const char *path) {
    if (strcmp(w->current_dir, path) == 0) return;
    if (w->fb_hist_len < 8) {
        strcpy(w->fb_hist[w->fb_hist_len], w->current_dir);
        w->fb_hist_len++;
    }
    w->fb_fwd_len = 0;
    strcpy(w->current_dir, path);
    w->file_scroll = 0;
    w->fb_sel = 0;
    filebrowser_refresh(w);
}

static void fb_go_up(window_t *w) {
    char parent[128];
    strcpy(parent, w->current_dir);
    char *last = NULL;
    char *slash = parent;
    while (*slash) { if (*slash == '/') last = slash; slash++; }
    if (last && last != parent) *last = 0;
    else strcpy(parent, "/");
    fb_navigate(w, parent);
}

static void fb_go_back(window_t *w) {
    if (w->fb_hist_len == 0) return;
    if (w->fb_fwd_len < 8) {
        strcpy(w->fb_fwd[w->fb_fwd_len], w->current_dir);
        w->fb_fwd_len++;
    }
    w->fb_hist_len--;
    strcpy(w->current_dir, w->fb_hist[w->fb_hist_len]);
    w->file_scroll = 0;
    w->fb_sel = 0;
    filebrowser_refresh(w);
}

static void fb_go_fwd(window_t *w) {
    if (w->fb_fwd_len == 0) return;
    w->fb_fwd_len--;
    if (w->fb_hist_len < 8) {
        strcpy(w->fb_hist[w->fb_hist_len], w->current_dir);
        w->fb_hist_len++;
    }
    strcpy(w->current_dir, w->fb_fwd[w->fb_fwd_len]);
    w->file_scroll = 0;
    w->fb_sel = 0;
    filebrowser_refresh(w);
}

static int fb_is_text_file(const char *name) {
    const char *dot = NULL;
    for (const char *p = name; *p; p++) if (*p == '.') dot = p;
    if (!dot) return 0;
    const char *ext = dot + 1;
    const char *text_exts[] = {"txt", "c", "h", "md", "sh", "log", "cfg", "ini", "json", "asm", "S", "sys", "config"};
    for (int i = 0; i < 13; i++) {
        if (strcmp(ext, text_exts[i]) == 0) return 1;
    }
    return 0;
}

static char pending_path[256];
static int pending_path_set = 0;

static void fb_open_entry(window_t *w) {
    if (w->fb_sel < 0 || w->fb_sel >= w->num_entries) return;
    const char *name = w->entries[w->fb_sel].name;
    if (strcmp(name, "..") == 0) {
        fb_go_up(w);
        return;
    }
    char full[300];
    strcpy(full, w->current_dir);
    if (strcmp(w->current_dir, "/") != 0) strcat(full, "/");
    strcat(full, name);
    if (is_dir_name(name)) {
        fb_navigate(w, full);
        return;
    }
    pending_path_set = 1;
    strcpy(pending_path, full);
    if (fb_is_text_file(name)) {
        create_window(WTYPE_EDITOR, name, 150, 100, 650, 430);
    } else {
        create_window(WTYPE_HEXVIEW, name, 200, 100, 520, 410);
    }
    pending_path_set = 0;
}

static void fb_delete_entry(window_t *w) {
    if (w->fb_sel <= 0 || w->fb_sel >= w->num_entries) return;
    const char *name = w->entries[w->fb_sel].name;
    char full[300];
    strcpy(full, w->current_dir);
    if (strcmp(w->current_dir, "/") != 0) strcat(full, "/");
    strcat(full, name);
    if (is_dir_name(name)) sys_rmdir(full);
    else sys_unlink(full);
    filebrowser_refresh(w);
}

/* Copy the contents of `src` into `dst` (regular files only). */
static void fb_copy_file(const char *src, const char *dst) {
    int src_fd = sb_acquire(src, 0);
    if (src_fd < 0) return;
    int dst_fd = sb_acquire(dst, 0x40 | 0x1 | 0x200); /* CREATE | PUSH | TRUNC */
    if (dst_fd < 0) {
        sb_release(src_fd);
        return;
    }
    char buf[4096];
    while (1) {
        int r = (int)sb_pull(src_fd, buf, sizeof(buf));
        if (r <= 0) break;
        sb_push(dst_fd, buf, r);
    }
    sb_release(src_fd);
    sb_release(dst_fd);
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
    w->paint_tool = PAINT_TOOL_BRUSH;
    w->paint_undo_count = 0;
    w->paint_undo_pos = 0;
    for (int i = 0; i < 6; i++) w->paint_undo[i] = NULL;
}

static void paint_clear(window_t *w) {
    for (int i = 0; i < 512 * 320; i++) w->paint_canvas[i] = 0xFFFFFF;
}

static void paint_save_undo(window_t *w) {
    if (w->paint_undo_count >= 6) {
        w->paint_undo_count = 0;
        w->paint_undo_pos = 0;
    }
    if (!w->paint_undo[w->paint_undo_pos]) {
        w->paint_undo[w->paint_undo_pos] = (uint32_t *)sys_sbrk(512 * 320 * 4);
        if ((int64_t)w->paint_undo[w->paint_undo_pos] < 0) {
            w->paint_undo[w->paint_undo_pos] = NULL;
            return;
        }
    }
    memcpy(w->paint_undo[w->paint_undo_pos], w->paint_canvas, 512 * 320 * 4);
    w->paint_undo_pos = (w->paint_undo_pos + 1) % 6;
    w->paint_undo_count++;
}

static void paint_undo(window_t *w) {
    if (w->paint_undo_count == 0) return;
    w->paint_undo_pos = (w->paint_undo_pos + 5) % 6;
    if (w->paint_undo[w->paint_undo_pos]) {
        memcpy(w->paint_canvas, w->paint_undo[w->paint_undo_pos], 512 * 320 * 4);
    }
    w->paint_undo_count--;
}

static void paint_dot(window_t *w, int cx, int cy) {
    if (cx < 0 || cx >= 512 || cy < 0 || cy >= 320) return;
    if (w->paint_tool == PAINT_TOOL_FILL) {
        paint_fill(w, cx, cy);
        return;
    }
    int r = w->brush_size;
    uint32_t col = w->paint_color;
    if (w->paint_tool == PAINT_TOOL_ERASER) col = 0xFFFFFF;
    if (w->paint_tool == PAINT_TOOL_PENCIL) r = 0;
    if (w->paint_tool == PAINT_TOOL_SPRAY) {
        int n = 8 + r * 2;
        for (int i = 0; i < n; i++) {
            int dx = ((int)rnd() % (2 * r + 1)) - r;
            int dy = ((int)rnd() % (2 * r + 1)) - r;
            int px = cx + dx, py = cy + dy;
            if (px >= 0 && px < 512 && py >= 0 && py < 320) {
                w->paint_canvas[py * 512 + px] = col;
            }
        }
        return;
    }
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int px = cx + dx;
            int py = cy + dy;
            if (px < 0 || px >= 512 || py < 0 || py >= 320) continue;
            if (dx * dx + dy * dy <= r * r) {
                w->paint_canvas[py * 512 + px] = col;
            }
        }
    }
}

static void paint_fill(window_t *w, int sx, int sy) {
    if (sx < 0 || sx >= 512 || sy < 0 || sy >= 320) return;
    uint32_t target = w->paint_canvas[sy * 512 + sx];
    if (target == w->paint_color) return;
    int stack[2048];
    int sp = 0;
    stack[sp++] = sx;
    stack[sp++] = sy;
    while (sp > 0) {
        int y = stack[--sp];
        int x = stack[--sp];
        if (x < 0 || x >= 512 || y < 0 || y >= 320) continue;
        if (w->paint_canvas[y * 512 + x] != target) continue;
        w->paint_canvas[y * 512 + x] = w->paint_color;
        if (sp < 2040) {
            stack[sp++] = x + 1; stack[sp++] = y;
            stack[sp++] = x - 1; stack[sp++] = y;
            stack[sp++] = x; stack[sp++] = y + 1;
            stack[sp++] = x; stack[sp++] = y - 1;
        }
    }
}

static void paint_commit_shape(window_t *w) {
    int x0 = w->paint_x0, y0 = w->paint_y0;
    int x1 = w->paint_x1, y1 = w->paint_y1;
    if (w->paint_tool == PAINT_TOOL_LINE) {
        draw_line_canvas(w->paint_canvas, x0, y0, x1, y1, w->paint_color);
    } else if (w->paint_tool == PAINT_TOOL_RECT) {
        int l = x0 < x1 ? x0 : x1;
        int t = y0 < y1 ? y0 : y1;
        int r = x0 < x1 ? x1 : x0;
        int b = y0 < y1 ? y1 : y0;
        draw_line_canvas(w->paint_canvas, l, t, r, t, w->paint_color);
        draw_line_canvas(w->paint_canvas, l, b, r, b, w->paint_color);
        draw_line_canvas(w->paint_canvas, l, t, l, b, w->paint_color);
        draw_line_canvas(w->paint_canvas, r, t, r, b, w->paint_color);
    } else if (w->paint_tool == PAINT_TOOL_ELLIPSE) {
        int l = x0 < x1 ? x0 : x1;
        int t = y0 < y1 ? y0 : y1;
        int r = x0 < x1 ? x1 : x0;
        int b = y0 < y1 ? y1 : y0;
        int rx = (r - l) / 2;
        int ry = (b - t) / 2;
        int cx = (l + r) / 2;
        int cy = (t + b) / 2;
        if (rx < 1) rx = 1;
        if (ry < 1) ry = 1;
        for (int dy = -ry; dy <= ry; dy++) {
            int span = (rx * isqrt(ry * ry - dy * dy)) / ry;
            draw_line_canvas(w->paint_canvas, cx - span, cy + dy, cx + span, cy + dy, w->paint_color);
        }
    }
}

static void draw_line_canvas(uint32_t *canvas, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        if (x0 >= 0 && x0 < 512 && y0 >= 0 && y0 < 320)
            canvas[y0 * 512 + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

static void paint_save_bmp(window_t *w) {
    int fd = sb_acquire("/paint.bmp", SB_MODE_CREATE | SB_MODE_PUSH | SB_MODE_TRUNC);
    if (fd < 0) return;
    uint8_t hdr[54];
    memset(hdr, 0, 54);
    hdr[0] = 'B'; hdr[1] = 'M';
    uint32_t fsize = 54 + 512 * 320 * 3;
    hdr[2] = fsize & 0xFF; hdr[3] = (fsize >> 8) & 0xFF; hdr[4] = (fsize >> 16) & 0xFF; hdr[5] = (fsize >> 24) & 0xFF;
    hdr[10] = 54;
    hdr[14] = 40;
    hdr[18] = 512 & 0xFF; hdr[19] = (512 >> 8) & 0xFF; hdr[20] = (512 >> 16) & 0xFF; hdr[21] = (512 >> 24) & 0xFF;
    hdr[22] = 320 & 0xFF; hdr[23] = (320 >> 8) & 0xFF; hdr[24] = (320 >> 16) & 0xFF; hdr[25] = (320 >> 24) & 0xFF;
    hdr[26] = 1;
    hdr[28] = 24;
    sb_push(fd, hdr, 54);
    uint8_t row[512 * 3];
    for (int y = 319; y >= 0; y--) {
        for (int x = 0; x < 512; x++) {
            uint32_t c = w->paint_canvas[y * 512 + x];
            row[x * 3 + 0] = c & 0xFF;
            row[x * 3 + 1] = (c >> 8) & 0xFF;
            row[x * 3 + 2] = (c >> 16) & 0xFF;
        }
        sb_push(fd, row, 512 * 3);
    }
    sb_release(fd);
}

static void paint_open_bmp(window_t *w) {
    int fd = sb_acquire("/paint.bmp", 0);
    if (fd < 0) return;
    uint8_t hdr[54];
    if (sb_pull(fd, hdr, 54) != 54 || hdr[0] != 'B' || hdr[1] != 'M') {
        sb_release(fd);
        return;
    }
    int wpx = hdr[18] | (hdr[19] << 8) | (hdr[20] << 16) | (hdr[21] << 24);
    int hpx = hdr[22] | (hdr[23] << 8) | (hdr[24] << 16) | (hdr[25] << 24);
    if (wpx != 512 || hpx != 320) {
        sb_release(fd);
        return;
    }
    uint8_t row[512 * 3];
    for (int y = 319; y >= 0; y--) {
        if (sb_pull(fd, row, 512 * 3) != 512 * 3) break;
        for (int x = 0; x < 512; x++) {
            uint32_t b = row[x * 3];
            uint32_t g = row[x * 3 + 1];
            uint32_t r = row[x * 3 + 2];
            w->paint_canvas[y * 512 + x] = (r << 16) | (g << 8) | b;
        }
    }
    sb_release(fd);
}

/* ------------------------------------------------------------------ */
/* Built-in Browser                                                    */
/* ------------------------------------------------------------------ */
#define BROWSER_AF_INET     1
#define BROWSER_SOCK_STREAM 1
#define BROWSER_SOCKET_CREATE  41
#define BROWSER_SOCKET_CONNECT 42
#define BROWSER_SOCKET_SENDTO  44
#define BROWSER_SOCKET_RECVFROM 45

static uint16_t br_htons(uint16_t x) { return (uint16_t)((x >> 8) | (x << 8)); }
static uint32_t br_htonl(uint32_t x) {
    return ((x >> 24) & 0xFF) | ((x >> 8) & 0xFF00) |
           ((x << 8) & 0xFF0000) | ((x << 24) & 0xFF000000);
}

static uint32_t br_parse_ip(const char *s) {
    uint32_t ip = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t part = 0;
        while (*s && *s != '.') { part = part * 10 + (*s - '0'); s++; }
        ip = (ip << 8) | (part & 0xFF);
        if (*s == '.') s++;
    }
    return ip;
}

/* Fetch http://<ipv4>[:port]/path and store the raw response in `out`.
 * Returns 1 on success, 0 on connection failure, -1 on unsupported https. */
static int browser_http_get(const char *url, char *out, int out_max) {
    if (strncmp(url, "https://", 8) == 0) return -1;
    const char *p = url;
    if (strncmp(p, "http://", 7) == 0) p += 7;

    char host[128];
    int hi = 0;
    while (*p && *p != '/' && *p != ':' && hi < 127) host[hi++] = *p++;
    host[hi] = 0;
    if (!host[0]) return 0;

    uint16_t port = 80;
    if (*p == ':') {
        p++;
        int prt = 0;
        while (*p >= '0' && *p <= '9') { prt = prt * 10 + (*p - '0'); p++; }
        if (prt > 0) port = (uint16_t)prt;
    }
    const char *path = (*p == '/') ? p : "/";

    int fd = (int)syscall3(BROWSER_SOCKET_CREATE, BROWSER_AF_INET, BROWSER_SOCK_STREAM, 0);
    if (fd < 0) return 0;

    struct { uint16_t family; uint16_t port; uint32_t addr; } addr;
    addr.family = BROWSER_AF_INET;
    addr.port = br_htons(port);
    addr.addr = br_htonl(br_parse_ip(host));

    if ((int)syscall3(BROWSER_SOCKET_CONNECT, fd, (uint64_t)&addr, sizeof(addr)) < 0) {
        sb_release(fd);
        return 0;
    }

    char req[1024];
    int len = 0;
    {
        const char *parts[] = { "GET ", path, " HTTP/1.0\r\nHost: ", host,
                                "\r\nUser-Agent: ShadowBox/1.0\r\nAccept: text/html,*/*\r\nConnection: close\r\n\r\n" };
        for (int i = 0; i < 6 && len < (int)sizeof(req) - 1; i++) {
            const char *sp = parts[i];
            while (*sp && len < (int)sizeof(req) - 1) req[len++] = *sp++;
        }
    }
    req[len] = 0;
    if ((int)syscall3(BROWSER_SOCKET_SENDTO, fd, (uint64_t)req, len) != len) {
        sb_release(fd);
        return 0;
    }

    int total = 0;
    char chunk[1024];
    while (total < out_max - 1) {
        int got = (int)syscall3(BROWSER_SOCKET_RECVFROM, fd, (uint64_t)chunk, (uint64_t)sizeof(chunk));
        if (got <= 0) break;
        int n = (total + got < out_max - 1) ? got : out_max - 1 - total;
        memcpy(out + total, chunk, (uint64_t)n);
        total += n;
        if (got < (int)sizeof(chunk)) break;
    }
    out[total] = 0;
    sb_release(fd);
    return 1;
}

/* Strip HTML tags/scripts/styles and wrap text to `max_cols` columns. */
static void browser_render_html(const char *html, char *out, int out_max, int max_cols) {
    int op = 0;
    int col = 0;
    int in_tag = 0;
    int skip = 0;
    char word[256];
    int wn = 0;

#define BR_FLUSH_WORD() do { \
    if (wn > 0) { \
        if (col + wn + (col ? 1 : 0) > max_cols) { \
            if (op < out_max - 1) out[op++] = '\n'; \
            col = 0; \
        } else if (col > 0) { \
            if (op < out_max - 1) out[op++] = ' '; \
            col++; \
        } \
        for (int k = 0; k < wn && op < out_max - 1; k++) out[op++] = word[k]; \
        col += wn; \
        wn = 0; \
    } \
} while (0)

#define BR_NEWLINE() do { \
    BR_FLUSH_WORD(); \
    if (op < out_max - 1) out[op++] = '\n'; \
    col = 0; \
} while (0)

    for (int i = 0; html[i] && op < out_max - 1; i++) {
        char c = html[i];
        if (in_tag) {
            if (c == '>') in_tag = 0;
            continue;
        }
        if (c == '<') {
            if (strncmp(&html[i], "</script", 8) == 0 || strncmp(&html[i], "</SCRIPT", 8) == 0 ||
                strncmp(&html[i], "</style", 7) == 0 || strncmp(&html[i], "</STYLE", 7) == 0) {
                skip = 0;
            } else if (strncmp(&html[i], "<script", 7) == 0 || strncmp(&html[i], "<SCRIPT", 7) == 0 ||
                       strncmp(&html[i], "<style", 6) == 0 || strncmp(&html[i], "<STYLE", 6) == 0) {
                skip = 1;
            } else if (strncmp(&html[i], "<br", 3) == 0 || strncmp(&html[i], "<p", 2) == 0 ||
                       strncmp(&html[i], "<li", 3) == 0 || strncmp(&html[i], "<h", 2) == 0 ||
                       strncmp(&html[i], "<div", 4) == 0 || strncmp(&html[i], "<tr", 3) == 0) {
                BR_NEWLINE();
            }
            in_tag = 1;
            continue;
        }
        if (skip) continue;
        if (c == '&') {
            if (strncmp(&html[i], "&nbsp;", 6) == 0) { i += 5; c = ' '; }
            else if (strncmp(&html[i], "&amp;", 5) == 0) { i += 4; c = '&'; }
            else if (strncmp(&html[i], "&lt;", 4) == 0) { i += 3; c = '<'; }
            else if (strncmp(&html[i], "&gt;", 4) == 0) { i += 3; c = '>'; }
            else if (strncmp(&html[i], "&quot;", 6) == 0) { i += 5; c = '"'; }
            else if (strncmp(&html[i], "&#", 2) == 0) { continue; }
            else continue;
        }
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        if (c == ' ') {
            BR_FLUSH_WORD();
        } else {
            if (wn < (int)sizeof(word) - 1) word[wn++] = c;
            if (wn >= max_cols) BR_FLUSH_WORD();
        }
    }
    BR_FLUSH_WORD();
    out[op] = 0;
#undef BR_FLUSH_WORD
#undef BR_NEWLINE
}

/* Fetch and render the current browser_url (no history bookkeeping). */
static void browser_load(window_t *w) {
    strcpy(w->url_input, w->browser_url);
    w->url_edit = 0;
    w->browser_status = 1;

    char raw[4096];
    int r = browser_http_get(w->browser_url, raw, sizeof(raw));
    if (r <= 0) {
        if (r == -1) {
            strcpy(w->browser_content,
                "HTTPS is not yet supported.\n\n"
                "Only plain HTTP can be loaded by the built-in browser.\n"
                "Try a URL such as: http://93.184.216.34/");
        } else {
            strcpy(w->browser_content,
                "Could not connect to the server.\n\n"
                "The address must use an IPv4 literal, for example:\n"
                "  http://93.184.216.34/\n\n"
                "Check the network connection and try again.");
        }
        w->browser_status = 2;
        w->browser_scroll = 0;
        return;
    }

    char *body = raw;
    for (int i = 0; raw[i]; i++) {
        if (raw[i] == '\r' && raw[i + 1] == '\n' && raw[i + 2] == '\r' && raw[i + 3] == '\n') {
            body = &raw[i + 4];
            break;
        }
        if (raw[i] == '\n' && raw[i + 1] == '\n') {
            body = &raw[i + 2];
            break;
        }
    }
    int cols = (w->w - 48) / 8;
    if (cols < 20) cols = 20;
    if (cols > 100) cols = 100;
    browser_render_html(body, w->browser_content, (int)sizeof(w->browser_content), cols);
    w->browser_status = 0;
    w->browser_scroll = 0;
}

/* Navigate to a new URL, pushing the current page onto the back stack. */
static void browser_navigate(window_t *w, const char *url) {
    char full[300];
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        strcpy(full, "http://");
        strcat(full, url);
    } else {
        strcpy(full, url);
    }
    if (w->browser_url[0] && strcmp(w->browser_url, full) != 0) {
        if (w->browser_hist_len < 8) {
            strcpy(w->browser_hist[w->browser_hist_len], w->browser_url);
            w->browser_hist_len++;
        }
        w->browser_fwd_len = 0;
    }
    strcpy(w->browser_url, full);
    browser_load(w);
}

static void browser_back(window_t *w) {
    if (w->browser_hist_len == 0) return;
    if (w->browser_fwd_len < 8) {
        strcpy(w->browser_fwd[w->browser_fwd_len], w->browser_url);
        w->browser_fwd_len++;
    }
    w->browser_hist_len--;
    strcpy(w->browser_url, w->browser_hist[w->browser_hist_len]);
    browser_load(w);
}

static void browser_fwd(window_t *w) {
    if (w->browser_fwd_len == 0) return;
    if (w->browser_hist_len < 8) {
        strcpy(w->browser_hist[w->browser_hist_len], w->browser_url);
        w->browser_hist_len++;
    }
    w->browser_fwd_len--;
    strcpy(w->browser_url, w->browser_fwd[w->browser_fwd_len]);
    browser_load(w);
}

static void browser_refresh(window_t *w) {
    if (w->browser_url[0]) browser_load(w);
}

static void browser_init(window_t *w) {
    strcpy(w->url_input, "http://93.184.216.34/");
    strcpy(w->browser_url, "");
    strcpy(w->browser_content,
        "Welcome to ShadowBox Browser\n\n"
        "A privacy-friendly text browser built straight into the desktop.\n\n"
        "Type an address into the URL bar and press Enter (or click Go).\n"
        "Use the Back / Forward / Reload buttons or keyboard arrows to move around.\n\n"
        "Limitations:\n"
        "  - HTTP only (no TLS yet)\n"
        "  - The host must be an IPv4 address\n"
        "  - Text rendering only (no CSS, JavaScript or images)\n\n"
        "Try: http://93.184.216.34/");
    w->browser_scroll = 0;
    w->url_edit = 1;
    w->browser_status = 0;
    w->browser_hist_len = 0;
    w->browser_fwd_len = 0;
}

static void create_window(int type, const char *title, int x, int y, int w, int h) {
    if (num_windows >= MAX_WINDOWS) return;
    window_t *win = &windows[num_windows];
    memset(win, 0, sizeof(window_t));
    win->term_buf = -1;
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
        win->term_buf = term_alloc_buf();
        win->term_pipe_fd = -1;
        win->term_write_fd = -1;
        win->term_pid = -1;
        win->term_eof = 0;
        win->term_history_count = 0;
        win->term_history_pos = 0;
        win->term_input[0] = 0;
        win->term_input_pos = 0;
        if (win->term_buf >= 0) {
            terminal_init(win);
            terminal_spawn(win);
        }
    } else if (type == WTYPE_SYS_MON) {
        win->graph_pos = 0;
        win->proc_count = 0;
        win->proc_scroll = 0;
        win->proc_list_dirty = 1;
        for (int i = 0; i < 200; i++) {
            win->mem_graph[i] = 0;
            win->cpu_graph[i] = 0;
        }
    } else if (type == WTYPE_FILE_BRO) {
        strcpy(win->current_dir, "/");
        win->fb_hist_len = 0;
        win->fb_fwd_len = 0;
        win->fb_sel = 0;
        win->fb_sort = 0;
        win->fb_double_click = -1;
        win->fb_last_click = 0;
        win->fb_clipboard[0] = 0;
        win->fb_clipboard_valid = 0;
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
        if (pending_path_set) strcpy(win->file_path, pending_path);
        else strcpy(win->file_path, "/tmp/editor.txt");
        editor_load(win);
        win->cursor_x = 0;
        win->cursor_y = 0;
    } else if (type == WTYPE_PAINT) {
        paint_init(win);
        paint_clear(win);
    } else if (type == WTYPE_PROCMON) {
        win->proc_scroll = 0;
        win->graph_pos = 0;
        win->prev_mem_free = 0;
        win->prev_idle_ticks = 0;
        win->proc_list_dirty = 1;
        for (int i = 0; i < 200; i++) { win->mem_graph[i] = 0; win->cpu_graph[i] = 0; }
    } else if (type == WTYPE_HEXVIEW) {
        if (pending_path_set) strcpy(win->file_path, pending_path);
        else strcpy(win->file_path, "/desktop.elf");
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
    } else if (type == WTYPE_SETTINGS) {
        win->settings_category = 0;
        win->settings_scroll = 0;
        for (int i = 0; i < 32; i++) {
            win->settings_toggle[i] = 0;
            win->settings_text[i][0] = 0;
        }
        win->settings_toggle[0] = 1; /* dark mode on by default */
        strcpy(win->settings_text[0], "#3498DB");
    } else if (type == WTYPE_BROWSER) {
        browser_init(win);
    } else if (type == WTYPE_NOTES) {
        win->notes_len = 0;
        win->notes_text[0] = 0;
        win->notes_scroll = 0;
    } else if (type == WTYPE_STOPWATCH) {
        win->sw_running = 0;
        win->sw_start = 0;
        win->sw_accum = 0;
        win->sw_lap_count = 0;
    }

    num_windows++;
}

static void draw_window_title(window_t *w) {
    int is_top = (top_window() == w->id);
    uint32_t title_active = 0x2C3E50;   // Dark slate for active window
    uint32_t title_inactive = 0x34495E; // Slightly lighter for inactive
    uint32_t title_bg = (drag_win == w->id || is_top) ? title_active : title_inactive;

    draw_rect(w->x, w->y, w->w, TITLE_H, title_bg);
    draw_string_limit(w->x + 10, w->y + 8, w->title, (w->w - 110) / 8, 0xFFFFFF);

    // Close button (red)
    draw_rect(w->x + w->w - 24, w->y, 24, TITLE_H, 0xE74C3C);
    draw_string(w->x + w->w - 16, w->y + 8, "x", 0xFFFFFF);

    // Maximize/restore button (gray)
    draw_rect(w->x + w->w - 48, w->y, 24, TITLE_H, 0x95A5A6);
    if (w->maximized) draw_string(w->x + w->w - 42, w->y + 8, "_", 0xFFFFFF);
    else draw_string(w->x + w->w - 42, w->y + 8, "[]", 0xFFFFFF);

    // Minimize button (blue)
    draw_rect(w->x + w->w - 72, w->y, 24, TITLE_H, 0x3498DB);
    draw_string(w->x + w->w - 64, w->y + 8, "-", 0xFFFFFF);
}

static void draw_app_window(window_t *w);
static void draw_real_clock(int x, int y);
static void draw_app_window(window_t *w) {
    int x = w->x;
    int y = w->y;
    int win_w = w->w;
    int win_h = w->h;

    if (w->type == WTYPE_TERMINAL) {
        draw_rect_alpha(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1E1E1E, 235);
        static const uint32_t term_pal[9] = {
            0xECF0F1, 0xE74C3C, 0x2ECC71, 0xF1C40F,
            0x3498DB, 0x9B59B6, 0x1ABC9C, 0xECF0F1, 0xFFFFFF,
        };
        int total_rows = w->term_row + 1;
        if (total_rows > 200) total_rows = 200;
        int max_scroll = total_rows - 24;
        if (max_scroll < 0) max_scroll = 0;
        if (w->term_scroll > max_scroll) w->term_scroll = max_scroll;
        if (w->term_scroll < 0) w->term_scroll = 0;
        int start = total_rows - 24 - w->term_scroll;
        if (start < 0) start = 0;
        for (int row = 0; row < 24; row++) {
            int r = start + row;
            if (r < 0 || r >= 200) break;
            for (int col = 0; col < 60; col++) {
                char c = term_ring(w)[r * TERM_RING_COLS + col];
                if (c) {
                    int ci = term_color(w)[r * TERM_RING_COLS + col];
                    draw_char(x + 8 + col * 8, y + TITLE_H + 8 + row * 16, c, term_pal[ci & 8]);
                }
            }
        }
        if (!w->term_eof) {
            uint64_t ticks = sys_times(0);
            if ((ticks / 50) % 2 == 0) {
                int crow = w->term_row - start;
                if (crow >= 0 && crow < 24) {
                    draw_rect(x + 8 + w->term_col * 8, y + TITLE_H + 8 + crow * 12, 8, 12, 0x00FF00);
                }
            }
        }
        if (max_scroll > 0) {
            int sb_h = (win_h - TITLE_H) * 24 / total_rows;
            if (sb_h < 16) sb_h = 16;
            if (sb_h > win_h - TITLE_H) sb_h = win_h - TITLE_H;
            int sb_y = ((win_h - TITLE_H) - sb_h) * w->term_scroll / max_scroll;
            draw_rect(x + win_w - 12, y + TITLE_H + sb_y, 8, sb_h, 0x2C3E50);
        }
    } else if (w->type == WTYPE_FILE_BRO) {
        char title_buf[200];
        strcpy(title_buf, "ShadowBox Disk - ");
        strcat(title_buf, w->current_dir);
        draw_string_limit(x + 10, y + TITLE_H + 8, title_buf, (win_w - 20) / 8, 0x333333);

        int toolbar_y = y + TITLE_H + 24;
        draw_rect(x + 8, toolbar_y, win_w - 16, 30, 0xECF0F1);
        draw_rect(x + 8, toolbar_y, win_w - 16, 1, 0xBDC3C7);
        draw_button(x + 10, toolbar_y + 3, 36, 24, "Up", 0x2C3E50, 0xFFFFFF);
        draw_button(x + 50, toolbar_y + 3, 40, 24, "Back", 0x2C3E50, 0xFFFFFF);
        draw_button(x + 94, toolbar_y + 3, 40, 24, "Fwd", 0x2C3E50, 0xFFFFFF);
        draw_button(x + 138, toolbar_y + 3, 44, 24, "Home", 0x2C3E50, 0xFFFFFF);
        draw_button(x + 186, toolbar_y + 3, 44, 24, "Sort", w->fb_sort ? 0x2980B9 : 0x2C3E50, 0xFFFFFF);
        draw_button(x + 234, toolbar_y + 3, 36, 24, "Copy", 0x3498DB, 0xFFFFFF);
        draw_button(x + 274, toolbar_y + 3, 36, 24, "Paste", w->fb_clipboard[0] ? 0x27AE60 : 0x95A5A6, 0xFFFFFF);
        draw_button(x + 314, toolbar_y + 3, 36, 24, "Prop", 0x9B59B6, 0xFFFFFF);
        draw_button(x + 354, toolbar_y + 3, 36, 24, "Del", 0xE74C3C, 0xFFFFFF);
        draw_button(x + 394, toolbar_y + 3, 44, 24, "Rfsh", 0x27AE60, 0xFFFFFF);

        int list_top = y + TITLE_H + 58;
        int rows = (win_h - TITLE_H - 92) / 20;
        if (rows < 0) rows = 0;
        int total_file_rows = w->num_entries;
        int file_max_scroll = total_file_rows - rows;
        if (file_max_scroll < 0) file_max_scroll = 0;
        if (w->file_scroll > file_max_scroll) w->file_scroll = file_max_scroll;
        if (w->file_scroll < 0) w->file_scroll = 0;
        for (int i = w->file_scroll; i < w->num_entries && i < w->file_scroll + rows; i++) {
            uint32_t icon_color = 0x95A5A6;
            if (is_dir_name(w->entries[i].name)) icon_color = 0xF39C12;
            int iy = list_top + (i - w->file_scroll) * 20;
            if (i == w->fb_sel) {
                draw_rect(x + 10, iy, win_w - 24, 20, 0xD6EAF8);
            } else if (mouse_btn_down && in_rect(mouse_x, mouse_y, x + 10, iy, win_w - 24, 20)) {
                draw_rect(x + 10, iy, win_w - 24, 20, 0xEBF5FB);
            }
            draw_rect(x + 10, iy + 2, 12, 10, icon_color);
            draw_string_limit(x + 28, iy + 3, w->entries[i].name, (win_w - 90) / 8, 0x2C3E50);
            if (i > 0 && !is_dir_name(w->entries[i].name)) {
                char sbuf[16];
                num_to_str((int64_t)w->fb_sizes[i], sbuf);
                draw_string_limit(x + win_w - 76, iy + 3, sbuf, 6, 0x7F8C8D);
            }
        }
        if (file_max_scroll > 0) {
            int sb_h = (win_h - TITLE_H - 92) * rows / total_file_rows;
            if (sb_h < 16) sb_h = 16;
            int sb_y = ((win_h - TITLE_H - 92) - sb_h) * w->file_scroll / file_max_scroll;
            draw_rect(x + win_w - 12, list_top + sb_y, 8, sb_h, 0x95A5A6);
        }
        char stbuf[120];
        if (w->fb_status[0]) {
            strcpy(stbuf, w->fb_status);
        } else {
            strcpy(stbuf, w->fb_sort ? "Sort: size   " : "Sort: name   ");
            char nb[16];
            num_to_str(w->num_entries - 1, nb);
            strcat(stbuf, nb);
            strcat(stbuf, " items");
        }
        draw_rect(x, y + win_h - 16, win_w, 16, 0x34495E);
        draw_string_limit(x + 6, y + win_h - 12, stbuf, (win_w - 12) / 8, 0xECF0F1);
    } else if (w->type == WTYPE_SYS_MON) {
        /* --- Modern System Monitor --- */

        /* Background with subtle gradient effect */
        draw_rect(x, y, win_w, TITLE_H, 0x2C3E50);
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1B2631);

        /* Title with icon */
        draw_string(x + 10, y + TITLE_H + 6, "System Monitor", 0xECF0F1);

        /* Divider line */
        draw_rect(x + 10, y + TITLE_H + 22, win_w - 20, 1, 0x34495E);

        uint64_t now = sys_times(0);

        /* Update data every 10 ticks (~200ms) */
        if (now - w->last_update > 10) {
            w->last_update = now;
            w->proc_list_dirty = 1;

            /* Update memory graph */
            uint64_t mem[2];
            sys_mem_info(mem);
            uint64_t total_mb = mem[0] * 4 / 1024;
            uint64_t used_mb = mem[1] * 4 / 1024;
            uint8_t mem_pct = total_mb > 0 ? (uint8_t)((used_mb * 100) / total_mb) : 0;
            w->graph_pos = (w->graph_pos + 1) % 200;
            w->mem_graph[w->graph_pos] = mem_pct;

            /* Update CPU graph (simple heuristic based on process count changes) */
            struct proc_info procs[32];
            int n = sys_proc_info(procs, 32);
            uint64_t total_cpu = (n * 100) / (n + 1); /* rough est */
            if (total_cpu > 100) total_cpu = 100;
            w->cpu_graph[w->graph_pos] = (uint8_t)total_cpu;
        }

        /* --- Memory section --- */
        uint64_t mem[2];
        sys_mem_info(mem);
        uint64_t total_mb = mem[0] * 4 / 1024;
        uint64_t used_mb = mem[1] * 4 / 1024;
        uint64_t free_mb = total_mb - used_mb;
        uint8_t mem_pct = total_mb > 0 ? (uint8_t)((used_mb * 100) / total_mb) : 0;

        draw_string(x + 10, y + TITLE_H + 30, "Memory:", 0xECF0F1);
        draw_number(x + 80, y + TITLE_H + 30, used_mb, 0x3498DB);
        draw_string(x + 118, y + TITLE_H + 30, "MB /", 0x7F8C8D);
        draw_number(x + 154, y + TITLE_H + 30, total_mb, 0x3498DB);
        draw_string(x + 192, y + TITLE_H + 30, "MB  (", 0x7F8C8D);
        draw_number(x + 230, y + TITLE_H + 30, mem_pct, 0x27AE60);
        draw_string(x + 250, y + TITLE_H + 30, "% used)", 0x7F8C8D);

        /* Memory bar */
        int bar_x = x + 10;
        int bar_y = y + TITLE_H + 44;
        int bar_w = win_w - 20;
        draw_rect(bar_x, bar_y, bar_w, 12, 0x2C3E50);
        int fill_w = (mem_pct * bar_w) / 100;
        draw_rect(bar_x, bar_y, fill_w, 12, 0xE74C3C);
        if (fill_w < bar_w - 1) {
            draw_rect(bar_x + fill_w, bar_y, bar_w - fill_w, 12, 0x27AE60);
            draw_rect(bar_x + fill_w, bar_y, 1, 12, 0x2C3E50);
        }
        draw_string(bar_x, bar_y + 18, "Free:", 0x7F8C8D);
        draw_number(bar_x + 32, bar_y + 18, free_mb, 0x3498DB);
        draw_string(bar_x + 66, bar_y + 18, "MB", 0x7F8C8D);

        /* --- CPU section --- */
        draw_string(x + 10, y + TITLE_H + 70, "CPU:", 0xECF0F1);
        draw_number(x + 50, y + TITLE_H + 70, w->cpu_graph[w->graph_pos], 0x3498DB);
        draw_string(x + 70, y + TITLE_H + 70, "% load", 0x7F8C8D);

        /* CPU bar */
        int cpu_bar_y = y + TITLE_H + 84;
        draw_rect(bar_x, cpu_bar_y, bar_w, 12, 0x2C3E50);
        int cpu_fill = (w->cpu_graph[w->graph_pos] * bar_w) / 100;
        draw_rect(bar_x, cpu_bar_y, cpu_fill, 12, 0xF39C12);
        if (cpu_fill < bar_w - 1) {
            draw_rect(bar_x + cpu_fill, cpu_bar_y, bar_w - cpu_fill, 12, 0x27AE60);
            draw_rect(bar_x + cpu_fill, cpu_bar_y, 1, 12, 0x2C3E50);
        }

        /* --- Mini graphs --- */
        int graph_y = y + TITLE_H + 110;
        int graph_h = 40;
        int graph_w = win_w - 20;

        /* Memory graph */
        draw_string(x + 10, graph_y - 6, "Memory History:", 0x95A5A6);
        draw_rect(x + 10, graph_y, graph_w, graph_h, 0x1E293B);
        for (int i = 0; i < graph_w; i++) {
            int idx = (w->graph_pos - i + 200) % 200;
            int h = (w->mem_graph[idx] * graph_h) / 100;
            if (h > 0) {
                draw_rect(x + 10 + i, graph_y + graph_h - h, 1, h, 0x3498DB);
            }
        }

        /* CPU graph */
        int cpu_graph_y = graph_y + graph_h + 20;
        draw_string(x + 10, cpu_graph_y - 6, "CPU History:", 0x95A5A6);
        draw_rect(x + 10, cpu_graph_y, graph_w, graph_h, 0x1E293B);
        for (int i = 0; i < graph_w; i++) {
            int idx = (w->graph_pos - i + 200) % 200;
            int h = (w->cpu_graph[idx] * graph_h) / 100;
            if (h > 0) {
                draw_rect(x + 10 + i, cpu_graph_y + graph_h - h, 1, h, 0xF39C12);
            }
        }

        /* --- Process list --- */
        struct proc_info procs[16];
        int n = sys_proc_info(procs, 16);
        w->proc_count = n;

        /* --- Process list --- */
        int list_y = y + TITLE_H + 110;
        int list_h = win_h - TITLE_H - 130;
        int rows = list_h / 14;
        if (rows < 1) rows = 1;

        /* List header */
        draw_rect(x, list_y - 2, win_w, 14, 0x2C3E50);
        draw_string(x + 6, list_y - 4, "PID  Name", 0xECF0F1);
        draw_string(x + 150, list_y - 4, "State", 0xECF0F1);
        draw_string(x + 200, list_y - 4, "KIll", 0x7F8C8D);
        draw_string(x + win_w - 20, list_y - 4, "K", 0x7F8C8D);

        for (int i = 0; i < rows && (w->proc_scroll + i) < n; i++) {
            int idx = w->proc_scroll + i;
            int row_y = list_y + i * 14;
            struct proc_info *p = &procs[idx];
            int sel = in_rect(mouse_x, mouse_y, x, row_y, win_w, 14);
            if (sel) draw_rect(x, row_y, win_w, 14, 0x34495E);

            draw_number(x + 6, row_y + 2, p->pid, 0xECF0F1);
            draw_string_limit(x + 42, row_y + 2, p->name, 14, 0x3498DB);

            const char *state_str = "unknown";
            if (p->state == 0) state_str = "run ";
            else if (p->state == 1) state_str = "sleep";
            else if (p->state == 2) state_str = "zombie";
            else if (p->state == 3) state_str = "wait ";
            draw_string(x + 150, row_y + 2, state_str, 0x7F8C8D);

            /* Kill button */
            int btn_x = x + win_w - 20;
            uint32_t btn_bg = in_rect(mouse_x, mouse_y, btn_x, row_y + 1, 12, 12) ? 0xC0392B : 0xE74C3C;
            draw_rect(btn_x, row_y + 1, 12, 12, btn_bg);
            draw_char(btn_x + 3, row_y + 3, 'X', 0xFFFFFF);

            /* Memory indicator (cr3 present = has memory) */
            draw_string(x + 200, row_y + 2, p->cr3 ? "yes" : "no ", 0x7F8C8D);
        }

        /* Scrollbar */
        if (n > rows && rows > 0) {
            int sb_h = (list_h * rows) / n;
            if (sb_h < 16) sb_h = 16;
            int sb_y = list_y + (list_h - sb_h) * w->proc_scroll / (n - rows);
            draw_rect(x + win_w - 4, sb_y, 4, sb_h, 0x95A5A6);
        }

        /* Help text */
        draw_string(x + 6, y + win_h - 16, "K: Kill selected | Up/Dn: Scroll | Ctrl+T: Terminate all", 0x7F8C8D);
    } else if (w->type == WTYPE_ABOUT) {
        draw_rect(x + 2, y + TITLE_H, win_w - 4, win_h - TITLE_H, 0xFFFFFF);
        draw_string(x + 20, y + TITLE_H + 26, "ShadowBox OS v0.2.0", 0x333333);
        draw_string(x + 20, y + TITLE_H + 46, "By: darkdevil404", 0x333333);
        draw_string(x + 20, y + TITLE_H + 66, "x86_64 from-scratch kernel + GUI", 0x555555);
        draw_string(x + 20, y + TITLE_H + 86, "22 built-in applications", 0x555555);
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
            "0", "x2", "=", "<-",
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
                if (c) draw_char(x + 8 + col * 8, y + TITLE_H + 8 + row * 16, c, 0xD3D3D3);
            }
        }
        uint64_t ticks = sys_times(0);
        if ((ticks / 50) % 2 == 0) {
            draw_rect(x + 8 + w->cursor_x * 8, y + TITLE_H + 8 + w->cursor_y * 16, 8, 16, 0xFFFFFF);
        }
        draw_rect(x, y + win_h - 22, win_w, 22, 0x34495E);
        char status[300];
        strcpy(status, w->file_path);
        if (w->cursor_y >= 0) {
            strcat(status, "  Ln ");
            char ln[16];
            num_to_str(w->cursor_y + 1, ln);
            strcat(status, ln);
            strcat(status, " Col ");
            char col[16];
            num_to_str(w->cursor_x + 1, col);
            strcat(status, col);
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
        static const uint32_t palette[16] = {
            0x000000, 0xFFFFFF, 0xE74C3C, 0xE67E22,
            0xF1C40F, 0x2ECC71, 0x1ABC9C, 0x3498DB,
            0x2980B9, 0x9B59B6, 0x8E44AD, 0x34495E,
            0x95A5A6, 0xD35400, 0xC0392B, 0x27AE60,
        };
        static const char *tool_names[8] = {"Brush", "Pen", "Erase", "Fill", "Line", "Rect", "Oval", "Spray"};
        int tool_row = y + TITLE_H + 344;
        int pal_row = y + TITLE_H + 380;
        int ctrl_row = y + TITLE_H + 414;
        for (int t = 0; t < 8; t++) {
            int tx = x + 10 + t * 55;
            uint32_t bg = (t == w->paint_tool) ? 0x2C3E50 : 0x34495E;
            draw_button(tx, tool_row, 52, 26, tool_names[t], bg, 0xFFFFFF);
        }
        draw_rect(x + 10, pal_row - 6, 16 * 27, 1, 0x2C3E50);
        for (int p = 0; p < 16; p++) {
            int px = x + 10 + p * 27;
            draw_rect(px, pal_row, 24, 20, palette[p]);
            if (w->paint_color == palette[p]) draw_rect(px - 1, pal_row - 1, 26, 22, 0xFFFFFF);
        }
        draw_button(x + 10, ctrl_row, 48, 26, "Undo", 0x8E44AD, 0xFFFFFF);
        draw_button(x + 62, ctrl_row, 48, 26, "Clear", 0xE74C3C, 0xFFFFFF);
        draw_button(x + 114, ctrl_row, 48, 26, "Save", 0x27AE60, 0xFFFFFF);
        draw_button(x + 166, ctrl_row, 48, 26, "Open", 0x2980B9, 0xFFFFFF);
        draw_button(x + 340, ctrl_row, 26, 26, "-", 0x34495E, 0xFFFFFF);
        draw_string(x + 372, ctrl_row + 9, "sz", 0xFFFFFF);
        draw_number(x + 392, ctrl_row + 9, w->brush_size, 0xFFFFFF);
        draw_button(x + 420, ctrl_row, 26, 26, "+", 0x34495E, 0xFFFFFF);
        char st[48];
        strcpy(st, tool_names[w->paint_tool]);
        strcat(st, "  ");
        if (w->painting) {
            char co[16];
            num_to_str(w->paint_x1, co);
            strcat(st, co);
            strcat(st, ",");
            num_to_str(w->paint_y1, co);
            strcat(st, co);
        }
        draw_string(x + 470, ctrl_row + 9, st, 0xECF0F1);
        if (w->painting && (w->paint_tool == PAINT_TOOL_LINE || w->paint_tool == PAINT_TOOL_RECT || w->paint_tool == PAINT_TOOL_ELLIPSE)) {
            int ox = x + 10, oy = y + TITLE_H + 10;
            if (w->paint_tool == PAINT_TOOL_LINE) {
                draw_line(ox + w->paint_x0, oy + w->paint_y0, ox + w->paint_x1, oy + w->paint_y1, 0x888888);
            } else if (w->paint_tool == PAINT_TOOL_RECT) {
                int l = w->paint_x0 < w->paint_x1 ? w->paint_x0 : w->paint_x1;
                int t = w->paint_y0 < w->paint_y1 ? w->paint_y0 : w->paint_y1;
                int r = w->paint_x0 < w->paint_x1 ? w->paint_x1 : w->paint_x0;
                int b = w->paint_y0 < w->paint_y1 ? w->paint_y1 : w->paint_y0;
                draw_rect(ox + l, oy + t, r - l + 1, 1, 0x888888);
                draw_rect(ox + l, oy + b, r - l + 1, 1, 0x888888);
                draw_rect(ox + l, oy + t, 1, b - t + 1, 0x888888);
                draw_rect(ox + r, oy + t, 1, b - t + 1, 0x888888);
            } else {
                int l = w->paint_x0 < w->paint_x1 ? w->paint_x0 : w->paint_x1;
                int t = w->paint_y0 < w->paint_y1 ? w->paint_y0 : w->paint_y1;
                int r = w->paint_x0 < w->paint_x1 ? w->paint_x1 : w->paint_x0;
                int b = w->paint_y0 < w->paint_y1 ? w->paint_y1 : w->paint_y0;
                int rx = (r - l) / 2, ry = (b - t) / 2;
                int cx = (l + r) / 2, cy = (t + b) / 2;
                if (rx < 1) rx = 1;
                if (ry < 1) ry = 1;
                for (int dy = -ry; dy <= ry; dy++) {
                    int span = (rx * isqrt(ry * ry - dy * dy)) / ry;
                    draw_line(ox + cx - span, oy + cy + dy, ox + cx + span, oy + cy + dy, 0x888888);
                }
            }
        }
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
        draw_rect_alpha(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1E1E2E, 240);
        uint64_t mem[2];
        sys_mem_info(mem);
        uint64_t total_mb = mem[0] * 4 / 1024;
        uint64_t used_mb = mem[1] * 4 / 1024;
        
        int pad = 20;
        draw_string(x + pad, y + TITLE_H + pad, "SYSTEM RESOURCES", 0x89B4FA);
        draw_rect(x + pad, y + TITLE_H + pad + 20, win_w - pad*2, 1, 0x45475A);

        draw_string(x + pad, y + TITLE_H + pad + 40, "Memory Usage", 0xCDD6F4);
        
        int bar_y = y + TITLE_H + pad + 65;
        int bar_w = win_w - pad*2;
        int bar_h = 24;
        draw_rect(x + pad, bar_y, bar_w, bar_h, 0x313244);
        
        int used_frac = 0;
        if (total_mb > 0) used_frac = (int)((used_mb * bar_w) / total_mb);
        if (used_frac > bar_w) used_frac = bar_w;
        if (used_frac < 0) used_frac = 0;
        
        draw_rect(x + pad, bar_y, used_frac, bar_h, 0xF38BA8);
        
        draw_number(x + pad, bar_y + 35, used_mb, 0xF38BA8);
        draw_string(x + pad + 40, bar_y + 35, "MB used of", 0xA6ADC8);
        draw_number(x + pad + 130, bar_y + 35, total_mb, 0xCDD6F4);
        draw_string(x + pad + 170, bar_y + 35, "MB", 0xA6ADC8);

        draw_rect(x + pad, bar_y + 70, win_w - pad*2, 1, 0x45475A);
        draw_string(x + pad, bar_y + 90, "Uptime:", 0x89B4FA);
        uint64_t t = sys_times(0) / 100;
        
        int hours = t / 3600;
        int mins = (t % 3600) / 60;
        int secs = t % 60;
        
        draw_number(x + pad + 70, bar_y + 90, hours, 0xCDD6F4);
        draw_string(x + pad + 100, bar_y + 90, "h", 0xA6ADC8);
        draw_number(x + pad + 120, bar_y + 90, mins, 0xCDD6F4);
        draw_string(x + pad + 150, bar_y + 90, "m", 0xA6ADC8);
        draw_number(x + pad + 170, bar_y + 90, secs, 0xCDD6F4);
        draw_string(x + pad + 200, bar_y + 90, "s", 0xA6ADC8);
    } else if (w->type == WTYPE_SETTINGS) {
        /* Settings panel with sidebar categories */
        draw_rect_alpha(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1E1E2E, 240);

        /* Sidebar */
        int sidebar_w = 140;
        draw_rect_alpha(x, y + TITLE_H, sidebar_w, win_h - TITLE_H, 0x11111B, 240);

        static const char *categories[] = {
            "Appearance", "Display", "Audio", "Network",
            "Power", "Users", "Accessibility", "System", "Time & Date"
        };
        int num_cats = 9;
        int cat_h = 32;
        for (int i = 0; i < num_cats; i++) {
            int cy = y + TITLE_H + 10 + i * cat_h;
            if (cy + cat_h > y + win_h - 40) break;
            
            int hover = in_rect(mouse_x, mouse_y, x + 8, cy, sidebar_w - 16, cat_h - 4);
            uint32_t bg = (w->settings_category == i) ? 0x89B4FA : (hover ? 0x313244 : 0x11111B);
            uint32_t fg = (w->settings_category == i) ? 0x1E1E2E : 0xCDD6F4;
            
            draw_rect(x + 8, cy, sidebar_w - 16, cat_h - 4, bg);
            draw_string_limit(x + 8 + 12, cy + 8, categories[i], 14, fg);
        }

        /* Category content area */
        int content_x = x + sidebar_w + 30;
        int content_y = y + TITLE_H + 30;
        int content_w = win_w - sidebar_w - 60;
        int content_h = win_h - TITLE_H - 60;

        /* Currently selected category name */
        draw_string(content_x, content_y, categories[w->settings_category], 0x89B4FA);
        draw_rect(content_x, content_y + 20, content_w, 1, 0x45475A);

        /* Render settings options based on category */
        int opt_y = content_y + 40;

        if (w->settings_category == 0) {
            /* Appearance */
            const char *opts[] = { "Dark Mode", "Accent Color", "Icon Theme", "Cursor Theme", "Animations" };
            int num_opts = 5;
            for (int i = 0; i < num_opts && opt_y < content_y + content_h - 10; i++) {
                int hover = in_rect(mouse_x, mouse_y, content_x, opt_y - 6, content_w, 24);
                if (hover && mouse_btn_down) {
                    w->settings_toggle[i] = !w->settings_toggle[i];
                }
                
                if (hover) {
                    draw_rect(content_x - 8, opt_y - 6, content_w + 16, 28, 0x313244);
                }
                
                draw_string(content_x, opt_y, opts[i], 0xCDD6F4);
                if (i == 0) {
                    /* Toggle switch */
                    int ts_x = content_x + content_w - 40;
                    uint32_t ts_bg = w->settings_toggle[i] ? 0xA6E3A1 : 0x45475A;
                    draw_rect(ts_x, opt_y - 2, 30, 16, ts_bg);
                    if (w->settings_toggle[i]) {
                        draw_rect(ts_x + 16, opt_y, 12, 12, 0x1E1E2E);
                    } else {
                        draw_rect(ts_x + 2, opt_y, 12, 12, 0xCDD6F4);
                    }
                } else {
                    draw_string(content_x + content_w - 60, opt_y, "Default", 0xA6ADC8);
                }
                opt_y += 32;
            }
        } else if (w->settings_category == 1) {
            /* Display */
            const char *opts[] = { "Resolution", "Brightness", "Night Light", "UI Scale" };
            for (int i = 0; i < 4 && opt_y < content_y + content_h - 10; i++) {
                draw_string(content_x, opt_y, opts[i], 0xCDD6F4);
                draw_string(content_x + content_w - 80, opt_y,
                    i == 0 ? "1024x768" : (i == 1 ? "75%" : (i == 2 ? "off" : "100%")),
                    0x89B4FA);
                opt_y += 32;
            }
        } else if (w->settings_category == 2) {
            /* Audio */
            const char *opts[] = { "Master Volume", "Mute", "Output Device" };
            for (int i = 0; i < 3 && opt_y < content_y + content_h - 10; i++) {
                draw_string(content_x, opt_y, opts[i], 0xCDD6F4);
                if (i == 0) {
                    int vol = 65; /* simulated */
                    draw_rect(content_x + 120, opt_y - 2, 100, 14, 0x45475A);
                    draw_rect(content_x + 120, opt_y - 2, vol, 14, 0xA6E3A1);
                } else {
                    draw_string(content_x + content_w - 60, opt_y,
                        i == 1 ? "off" : "Speakers", 0x89B4FA);
                }
                opt_y += 32;
            }
        } else if (w->settings_category == 3) {
            /* Network */
            const char *opts[] = { "WiFi", "Ethernet", "Proxy", "Hostname" };
            for (int i = 0; i < 4 && opt_y < content_y + content_h - 10; i++) {
                draw_string(content_x, opt_y, opts[i], 0xCDD6F4);
                draw_string(content_x + content_w - 100, opt_y,
                    i == 0 ? "connected" : (i == 1 ? "up" : (i == 2 ? "none" : "shadox-box")),
                    0x27AE60);
                opt_y += 22;
            }
        } else if (w->settings_category == 4) {
            /* Power */
            const char *opts[] = { "Dim Timeout (s)", "Sleep Timeout (s)", "Power Saving" };
            for (int i = 0; i < 3 && opt_y < content_y + content_h - 10; i++) {
                draw_string(content_x, opt_y, opts[i], 0x2C3E50);
                int val_x = content_x + content_w - 50;
                if (i == 0) {
                    draw_number(val_x, opt_y, 30, 0x3498DB);
                } else if (i == 1) {
                    draw_number(val_x, opt_y, 120, 0x3498DB);
                } else {
                    int hover = in_rect(mouse_x, mouse_y, val_x - 4, opt_y - 2, 28, 14);
                    uint32_t bg = w->settings_toggle[i + 10] ? 0x27AE60 : 0x95A5A6;
                    draw_rect(val_x - 4, opt_y - 2, 28, 14, bg);
                    if (hover && mouse_btn_down) w->settings_toggle[i + 10] = !w->settings_toggle[i + 10];
                    if (w->settings_toggle[i + 10]) draw_rect(val_x, opt_y + 2, 6, 6, 0xFFFFFF);
                }
                opt_y += 22;
            }
        } else if (w->settings_category == 5) {
            /* Users */
            draw_string(content_x, opt_y, "Default User: shadox-box", 0x2C3E50);
            opt_y += 22;
            draw_string(content_x, opt_y, "Password: ********", 0x2C3E50);
            opt_y += 22;
            draw_string(content_x, opt_y, "Auto-login: enabled", 0x27AE60);
            opt_y += 22;
            draw_string(content_x, opt_y, "Additional users: none", 0x7F8C8D);
        } else if (w->settings_category == 6) {
            /* Accessibility */
            const char *opts[] = { "Screen Reader", "High Contrast", "Font Scaling", "Sticky Keys" };
            for (int i = 0; i < 4 && opt_y < content_y + content_h - 10; i++) {
                draw_string(content_x, opt_y, opts[i], 0x2C3E50);
                int hover = in_rect(mouse_x, mouse_y, content_x + content_w - 50, opt_y - 2, 28, 14);
                uint32_t bg = w->settings_toggle[i + 20] ? 0x27AE60 : 0x95A5A6;
                int ts_x = content_x + content_w - 50;
                draw_rect(ts_x, opt_y - 2, 28, 14, bg);
                if (hover && mouse_btn_down) w->settings_toggle[i + 20] = !w->settings_toggle[i + 20];
                if (w->settings_toggle[i + 20]) draw_rect(ts_x + 4, opt_y + 2, 6, 6, 0xFFFFFF);
                opt_y += 22;
            }
        } else if (w->settings_category == 7) {
            /* System */
            draw_string(content_x, opt_y, "OS: ShadowBox OS v0.2.0", 0x2C3E50);
            opt_y += 22;
            draw_string(content_x, opt_y, "Kernel: x86_64 from-scratch", 0x2C3E50);
            opt_y += 22;
            draw_string(content_x, opt_y, "Uptime:", 0x2C3E50);
            uint64_t t = sys_times(0) / 100;
            draw_number(content_x + 80, opt_y, t / 3600, 0x3498DB);
            draw_string(content_x + 118, opt_y, "h", 0x3498DB);
            opt_y += 22;
            draw_button(content_x, opt_y, 90, 26, "Restart", 0x9254DE, 0xFFFFFF);
            opt_y += 30;
            draw_button(content_x, opt_y, 90, 26, "Shutdown", 0xE74C3C, 0xFFFFFF);
        } else if (w->settings_category == 8) {
            /* Time & Date */
            int tz = sys_timezone_get();
            draw_string(content_x, opt_y, "Time Zone (minutes from UTC)", 0x2C3E50);
            char tz_str[24];
            int i = 0;
            int tz_neg = (tz < 0);
            if (tz_neg) tz = -tz;
            tz_str[i++] = 'U'; tz_str[i++] = 'T'; tz_str[i++] = 'C';
            tz_str[i++] = tz_neg ? '-' : '+';
            int tz_h = tz / 60, tz_m = tz % 60;
            tz_str[i++] = '0' + (tz_h / 10) % 10;
            tz_str[i++] = '0' + tz_h % 10;
            tz_str[i++] = ':';
            tz_str[i++] = '0' + (tz_m / 10) % 10;
            tz_str[i++] = '0' + tz_m % 10;
            tz_str[i] = 0;
            draw_string(content_x + content_w - 90, opt_y, tz_str, 0x3498DB);
            opt_y += 30;

            int bh = in_rect(mouse_x, mouse_y, content_x, opt_y - 2, 26, 20);
            draw_rect(content_x, opt_y - 2, 26, 20, bh ? 0x89B4FA : 0x45475A);
            draw_char(content_x + 8, opt_y + 1, '-', 0xFFFFFF);
            bh = in_rect(mouse_x, mouse_y, content_x + 32, opt_y - 2, 26, 20);
            draw_rect(content_x + 32, opt_y - 2, 26, 20, bh ? 0x89B4FA : 0x45475A);
            draw_char(content_x + 40, opt_y + 1, '+', 0xFFFFFF);
            bh = in_rect(mouse_x, mouse_y, content_x + 66, opt_y - 2, 64, 20);
            draw_rect(content_x + 66, opt_y - 2, 64, 20, bh ? 0x89B4FA : 0x45475A);
            draw_string(content_x + 76, opt_y, "Reset", 0xFFFFFF);
            opt_y += 30;

            draw_string(content_x, opt_y, "Local time:", 0x2C3E50);
            draw_real_clock(content_x + content_w - 96, opt_y);
            opt_y += 30;

            bh = in_rect(mouse_x, mouse_y, content_x, opt_y - 2, 120, 24);
            draw_rect(content_x, opt_y - 2, 120, 24, bh ? 0x89B4FA : 0x2563EB);
            draw_string(content_x + 14, opt_y, "Sync via NTP", 0xFFFFFF);
        }
    } else if (w->type == WTYPE_BROWSER) {
        int content_top = y + TITLE_H + 60;

        draw_rect(x + 2, y + TITLE_H, win_w - 4, win_h - TITLE_H, 0xECF0F1);

        /* Toolbar */
        draw_rect(x + 4, y + TITLE_H + 4, win_w - 8, 32, 0x34495E);
        draw_button(x + 6, y + TITLE_H + 8, 38, 24, "<-", 0x2C3E50, 0xFFFFFF);
        draw_button(x + 46, y + TITLE_H + 8, 38, 24, "->", 0x2C3E50, 0xFFFFFF);
        draw_button(x + 86, y + TITLE_H + 8, 40, 24, "Rld", 0x2C3E50, 0xFFFFFF);
        draw_button(x + 128, y + TITLE_H + 8, 32, 24, "Home", 0x2C3E50, 0xFFFFFF);

        /* Address bar + Go button */
        int bar_x = x + 164;
        int bar_w = win_w - 168;
        if (bar_w < 70) bar_w = 70;
        draw_rect(bar_x, y + TITLE_H + 8, bar_w - 4, 24, 0xFFFFFF);
        draw_rect(bar_x + bar_w - 44, y + TITLE_H + 8, 40, 24, 0x2980B9);
        draw_string(bar_x + bar_w - 36, y + TITLE_H + 13, "Go", 0xFFFFFF);

        const char *urltext = w->url_edit ? w->url_input : w->browser_url;
        if (!urltext[0]) urltext = "about:home";
        int max_chars = (bar_w - 56) / 8;
        if (max_chars < 0) max_chars = 0;
        draw_string_limit(bar_x + 4, y + TITLE_H + 13, urltext, max_chars, 0x1B2631);
        if (w->url_edit && (sys_times(0) / 50) % 2 == 0) {
            int cpos = 0;
            const char *t = urltext;
            while (t[cpos] && cpos < max_chars) cpos++;
            draw_rect(bar_x + 4 + cpos * 8, y + TITLE_H + 13, 2, 12, 0x3498DB);
        }

        /* Status line */
        const char *status;
        if (w->browser_status == 1) status = "Loading...";
        else if (w->browser_status == 2) status = "Load failed";
        else status = (w->browser_url[0] && strcmp(w->browser_url, "about:home") != 0) ? w->browser_url : "Home";
        draw_string_limit(x + 8, y + TITLE_H + 42, status, (win_w - 16) / 8, 0x7F8C8D);

        /* Page content */
        int rows = (win_h - TITLE_H - 60) / 16;
        if (rows < 0) rows = 0;
        int total_lines = 0;
        for (int i = 0; w->browser_content[i]; i++)
            if (w->browser_content[i] == '\n') total_lines++;
        if (w->browser_content[0]) total_lines++;
        int max_scroll = total_lines - rows;
        if (max_scroll < 0) max_scroll = 0;
        if (w->browser_scroll > max_scroll) w->browser_scroll = max_scroll;
        if (w->browser_scroll < 0) w->browser_scroll = 0;

        int line = 0;
        const char *p = w->browser_content;
        while (*p) {
            int l = 0;
            while (p[l] && p[l] != '\n') l++;
            if (line >= w->browser_scroll && line < w->browser_scroll + rows) {
                int ly = content_top + (line - w->browser_scroll) * 16;
                if (ly + 16 <= y + win_h) {
                    int maxc = (win_w - 24) / 8;
                    draw_string_limit(x + 8, ly, p, maxc, 0x2C3E50);
                }
            }
            p += l;
            if (*p == '\n') p++;
            line++;
        }

        /* Scrollbar */
        if (max_scroll > 0) {
            int sb_h = (rows * 16) * rows / total_lines;
            if (sb_h < 12) sb_h = 12;
            if (sb_h > rows * 16) sb_h = rows * 16;
            int sb_y = content_top + ((rows * 16) - sb_h) * w->browser_scroll / max_scroll;
            draw_rect(x + win_w - 10, sb_y, 6, sb_h, 0x95A5A6);
        }
    } else if (w->type == WTYPE_NOTES) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1E293B);
        draw_string(x + 8, y + TITLE_H + 6, "Notes  [Esc] clear", 0x7C8FA6);
        int maxc = (win_w - 20) / 8;
        if (maxc < 1) maxc = 1;
        int offs[256];
        int nl = 0;
        int i = 0;
        while (i <= w->notes_len && nl < 256) {
            offs[nl++] = i;
            if (i == w->notes_len) break;
            int l = 0;
            while (i + l < w->notes_len && w->notes_text[i + l] != '\n' && l < maxc) l++;
            i += l;
            if (i < w->notes_len && w->notes_text[i] == '\n') i++;
        }
        int rows = (win_h - TITLE_H - 30) / 14;
        if (rows < 0) rows = 0;
        int max_scroll = nl - rows;
        if (max_scroll < 0) max_scroll = 0;
        if (w->notes_scroll > max_scroll) w->notes_scroll = max_scroll;
        if (w->notes_scroll < 0) w->notes_scroll = 0;
        for (int ln = w->notes_scroll; ln < nl && ln < w->notes_scroll + rows; ln++) {
            int ly = y + TITLE_H + 24 + (ln - w->notes_scroll) * 14;
            int st = offs[ln];
            int len = 0;
            while (st + len < w->notes_len && w->notes_text[st + len] != '\n' && len < maxc) len++;
            if (len > 95) len = 95;
            char lb[96];
            for (int k = 0; k < len; k++) lb[k] = w->notes_text[st + k];
            lb[len] = 0;
            draw_string(x + 8, ly, lb, 0xE2E8F0);
        }
    } else if (w->type == WTYPE_STOPWATCH) {
        draw_rect(x, y + TITLE_H, win_w, win_h - TITLE_H, 0x1E293B);
        uint64_t now = sys_times(0);
        uint64_t total = w->sw_accum + (w->sw_running ? (now - w->sw_start) : 0);
        char buf[24];
        fmt_sw(total, buf);
        draw_string(x + win_w / 2 - 50, y + TITLE_H + 28, buf, 0x00FF88);
        draw_string(x + win_w / 2 - 50, y + TITLE_H + 48, w->sw_running ? "[ running ]" : "[ paused ]", 0x7C8FA6);

        int by = y + TITLE_H + 80;
        int bw = 70, bh = 26;
        uint32_t b1 = w->sw_running ? 0xE67E22 : 0x27AE60;
        draw_rect(x + 12, by, bw, bh, b1);
        draw_string(x + 12 + 12, by + 6, w->sw_running ? "Pause" : "Start", 0xFFFFFF);
        draw_rect(x + 12 + bw + 8, by, bw, bh, 0x3498DB);
        draw_string(x + 12 + bw + 8 + 10, by + 6, "Reset", 0xFFFFFF);
        draw_rect(x + 12 + (bw + 8) * 2, by, bw, bh, 0x9B59B6);
        draw_string(x + 12 + (bw + 8) * 2 + 14, by + 6, "Lap", 0xFFFFFF);

        int ly = by + bh + 18;
        draw_string(x + 12, ly, "Laps", 0x7C8FA6);
        ly += 16;
        for (int k = 0; k < w->sw_lap_count && ly < y + win_h - 6; k++) {
            char lb[24];
            fmt_sw(w->sw_laps[k], lb);
            char line[28];
            int n = 0;
            line[n++] = '#';
            line[n++] = '1' + (k % 10);
            line[n++] = ' ';
            for (int m = 0; lb[m]; m++) line[n++] = lb[m];
            line[n] = 0;
            draw_string(x + 12, ly, line, 0xE2E8F0);
            ly += 14;
        }
    }
}

static void draw_resize_handle(window_t *w) {
    // Draw window border with rounded corners
    // Outer border 1px thick, radius 8
    // Ensure we have a visible border distinct from background
    // Using fb_draw_rect_round for a 1-pixel rounded rectangle
    // Clip to screen automatically handled by fb_draw_rect_round
    // Draw after the resize handle so it appears on top
    // Border color: dark slate (e.g., 0x111111)
    // Note: This will overlay the border over the window area.
    // We'll call a helper after drawing the window.

    if (w->minimized) return;
    draw_rect(w->x + w->w - 10, w->y + w->h - 10, 10, 10, 0x2980B9);
    draw_line(w->x + w->w - 6, w->y + w->h - 2, w->x + w->w - 2, w->y + w->h - 6, 0xFFFFFF);
    draw_line(w->x + w->w - 6, w->y + w->h - 6, w->x + w->w - 2, w->y + w->h - 6, 0xFFFFFF);
    // Rounded border around the window
    fb_draw_rect_round(backbuffer, FB_STRIDE, w->x - 1, w->y - 1, w->w + 2, w->h + 2, 0x111111, 8);
}

static void draw_desktop_icon(int idx, int x, int y) {
    // Draw a simple icon based on idx (0=Terminal,1=Files,2=SysMon,3=Calc,4=Editor,5=Paint)
    switch (idx) {
        case 0: // Terminal
            draw_rect(x+4, y+8, 24, 16, 0x1E1E1E);
            draw_char(x+8, y+12, '>', 0x00FF00);
            break;
        case 1: // Files (folder)
            draw_rect(x+4, y+12, 24, 12, 0xF39C12);
            draw_rect(x+8, y+8, 12, 4, 0xF39C12);
            break;
        case 2: // SysMon gauge
            draw_rect(x+8, y+8, 16, 4, 0x2ECC71);
            draw_rect(x+8, y+16, 16, 4, 0x2980B9);
            break;
        case 3: // Calc pad
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    draw_rect(x+4 + c*8, y+4 + r*8, 6, 6, 0xECF0F1);
                }
            }
            break;
        case 4: // Editor (pencil)
            draw_line(x+8, y+24, x+24, y+8, 0xFFFFFF);
            draw_rect(x+22, y+6, 2, 2, 0x000000);
            break;
        case 5: // Paint palette
            draw_circle(x+16, y+16, 12, 0xE74C3C, 1);
            draw_rect(x+10, y+10, 2, 2, 0x2ECC71);
            draw_rect(x+22, y+10, 2, 2, 0x3498DB);
            draw_rect(x+16, y+22, 2, 2, 0xF1C40F);
            break;
    }
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

/* draw_desktop_icons implementation moved to desktop_icons.c */




/* Draw a simple recognizable icon glyph for each app type */
static void draw_menu_icon(int x, int y, int sz, int app_idx) {
    int cx = x + sz/2, cy = y + sz/2;
    int s = sz;  /* shorthand for size */
    uint32_t fg = 0xFFFFFF;

    switch (app_idx) {
    case 0:  /* Terminal - prompt >_ */
        draw_rect(x+8, y+10, s-16, s-20, 0x1E1E1E);
        draw_rect(x+12, cy-4, 4, 4, 0x10B981);
        draw_rect(x+12, cy-4, 10, 2, 0x10B981);
        break;
    case 1:  /* Files - folder */
        draw_rect(x+6, y+14, s-12, s-22, 0xF59E0B);
        draw_rect(x+10, y+10, 14, 6, 0xF59E0B);
        draw_rect(x+8, y+18, s-16, 2, 0xD97706);
        break;
    case 2:  /* System Monitor - gauge bars */
        draw_rect(x+10, y+12, s-20, 5, 0x10B981);
        draw_rect(x+10, y+20, s-20, 5, 0x3B82F6);
        draw_rect(x+10, y+28, s-28, 5, 0xEF4444);
        break;
    case 3:  /* Settings - gear/cog (simplified as circle+rect) */
        draw_rect(cx-3, y+8, 6, s-16, 0x64748B);
        draw_rect(x+8, cy-3, s-16, 6, 0x64748B);
        draw_rect(cx-5, cy-5, 10, 10, 0x475569);
        break;
    case 4:  /* Image Viewer - landscape */
        draw_rect(x+8, y+10, s-16, s-20, 0x1E293B);
        draw_rect(x+8, y+28, s-16, 12, 0x22C55E);  /* grass */
        draw_rect(cx-2, y+12, 4, 8, 0xF59E0B);     /* sun */
        draw_rect(x+10, y+20, 6, 8, 0x3B82F6);     /* mountain */
        draw_rect(x+20, y+18, 8, 10, 0x3B82F6);
        break;
    case 5:  /* Snake - snake body */
        draw_rect(x+12, y+14, 8, 8, 0x10B981);
        draw_rect(x+20, y+14, 8, 8, 0x22C55E);
        draw_rect(x+20, y+22, 8, 8, 0x22C55E);
        draw_rect(x+28, y+22, 8, 8, 0x22C55E);
        draw_rect(x+12, y+16, 3, 3, 0xFFFFFF); /* eye */
        break;
    case 6:  /* Calculator - grid */
        draw_rect(x+8, y+8, s-16, 10, 0x0F172A);
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 3; c++)
                draw_rect(x+10+c*9, y+22+r*7, 7, 5, 0x334155);
        break;
    case 7:  /* Editor - lines */
        draw_rect(x+8, y+10, s-16, s-20, 0x1E1E1E);
        draw_rect(x+12, y+14, 16, 2, 0x6B7280);
        draw_rect(x+12, y+20, 20, 2, 0x6B7280);
        draw_rect(x+12, y+26, 12, 2, 0x9CA3AF);
        draw_rect(x+12, y+32, 18, 2, 0x9CA3AF);
        break;
    case 8:  /* Paint - palette */
        draw_rect(x+8, y+8, s-16, s-16, 0xF1F5F9);
        draw_rect(x+12, y+12, 6, 6, 0xEF4444);
        draw_rect(x+22, y+12, 6, 6, 0x3B82F6);
        draw_rect(x+32, y+12, 6, 6, 0x22C55E);
        draw_rect(x+12, y+22, 6, 6, 0xF59E0B);
        draw_rect(x+22, y+22, 6, 6, 0x8B5CF6);
        break;
    case 9:  /* Processes - list */
        draw_rect(x+8, y+10, s-16, 4, 0x3B82F6);
        draw_rect(x+8, y+18, s-16, 4, 0x22C55E);
        draw_rect(x+8, y+26, s-16, 4, 0xF59E0B);
        draw_rect(x+8, y+34, s-20, 4, 0x64748B);
        break;
    case 10: /* Hex Viewer - hex grid */
        draw_rect(x+8, y+8, s-16, s-16, 0x1E1E1E);
        draw_rect(x+12, y+12, 4, 4, 0x34D399);
        draw_rect(x+20, y+12, 4, 4, 0x34D399);
        draw_rect(x+28, y+12, 4, 4, 0x34D399);
        draw_rect(x+12, y+20, 4, 4, 0x34D399);
        draw_rect(x+20, y+20, 4, 4, 0x34D399);
        draw_rect(x+12, y+28, 4, 4, 0x34D399);
        break;
    case 11: /* Tetris - blocks */
        draw_rect(x+10, y+12, 8, 8, 0x06B6D4);
        draw_rect(x+18, y+12, 8, 8, 0x06B6D4);
        draw_rect(x+18, y+20, 8, 8, 0x0891B2);
        draw_rect(x+18, y+28, 8, 8, 0x0891B2);
        draw_rect(x+26, y+12, 8, 8, 0xF59E0B);
        draw_rect(x+34, y+12, 8, 8, 0xF59E0B);
        break;
    case 12: /* 2048 - grid tile */
        draw_rect(x+10, y+10, s-20, s-20, 0x78716C);
        draw_rect(x+14, y+14, 12, 12, 0xEDE0C8);
        draw_rect(x+28, y+14, 12, 12, 0xF2B179);
        break;
    case 13: /* Mandelbrot - spiral */
        draw_rect(x+8, y+8, s-16, s-16, 0x0F172A);
        draw_rect(x+14, y+14, 18, 18, 0x2563EB);
        draw_rect(x+18, y+18, 10, 10, 0xFBBF24);
        break;
    case 14: /* Clock - circle */
        draw_rect(cx-1, y+10, 2, 14, 0xF1F5F9);
        draw_rect(cx, cy, 10, 2, 0xF1F5F9);
        draw_rect(cx, cy, 2, 2, 0xEF4444);
        break;
    case 15: /* Fortune - star */
        draw_rect(cx-1, y+10, 4, 24, 0xF59E0B);
        draw_rect(x+12, cy-1, 22, 4, 0xF59E0B);
        draw_rect(x+14, y+14, 18, 4, 0xF59E0B);
        break;
    case 16: /* Pong - paddles+ball */
        draw_rect(x+10, y+14, 3, 16, 0xFFFFFF);
        draw_rect(x+32, y+14, 3, 16, 0xFFFFFF);
        draw_rect(cx-2, cy-2, 4, 4, 0xFFFFFF);
        break;
    case 17: /* Matrix - falling chars */
        draw_rect(x+8, y+8, s-16, s-16, 0x000000);
        draw_rect(x+12, y+10, 2, 8, 0x22C55E);
        draw_rect(x+20, y+16, 2, 8, 0x22C55E);
        draw_rect(x+28, y+12, 2, 8, 0x22C55E);
        draw_rect(x+36, y+18, 2, 8, 0x22C55E);
        draw_rect(x+16, y+26, 2, 6, 0x22C55E);
        draw_rect(x+32, y+28, 2, 6, 0x22C55E);
        break;
    case 18: /* Memory - chip */
        draw_rect(x+10, y+10, s-20, s-20, 0x334155);
        draw_rect(x+14, y+14, s-28, s-28, 0x1E293B);
        draw_rect(x+6, cy-1, 4, 2, 0x64748B);
        draw_rect(x+s-10, cy-1, 4, 2, 0x64748B);
        break;
    case 19: /* Browser - globe */
        draw_rect(x+8, y+8, s-16, s-16, 0x0F172A);
        draw_circle(cx, cy, 10, 0x3B82F6, 0);
        draw_line(x+4, cy, x+s-4, cy, 0x3B82F6);
        draw_line(cx, y+4, cx, y+s-4, 0x3B82F6);
        draw_rect(cx-2, cy-2, 4, 4, 0x22C55E);
        break;
    case 20: /* About - info circle */
        draw_rect(cx-10, y+10, 20, 20, 0x3B82F6);
        draw_rect(cx-1, y+14, 2, 4, 0xFFFFFF);
        draw_rect(cx-1, y+22, 2, 2, 0xFFFFFF);
        break;
    case 21: /* Shutdown - power symbol */
        draw_rect(cx-1, y+10, 2, 16, 0xEF4444);
        draw_rect(x+12, y+12, s-24, 4, 0xEF4444);
        draw_rect(x+12, y+12, 4, 12, 0xE74C3C);
        draw_rect(x+s-16, y+12, 4, 12, 0xE74C3C);
        break;
    default:
        draw_rect(x+8, y+8, s-16, s-16, 0x34495E);
        break;
    }
}

#define MENU_ICON_SIZE 32
static uint32_t menu_icon_buf[NUM_APPS][MENU_ICON_SIZE * MENU_ICON_SIZE];
static uint8_t  menu_icon_state[NUM_APPS]; /* 0=pending, 1=loaded, 2=missing */

static void draw_start_menu(void) {
    if (!menu_open) return;
    draw_drop_shadow(MENU_MX, MENU_MY, MENU_MW, MENU_MH);
    draw_rect_alpha(MENU_MX, MENU_MY, MENU_MW, MENU_MH, 0x0F1419, 250);

    /* Top accent bar with gradient effect */
    draw_rect(MENU_MX, MENU_MY, MENU_MW, 4, 0x2563EB);

    /* Header with dynamic app count */
    draw_string(MENU_MX + 20, MENU_MY + 12, "ShadowBox OS", 0x60A5FA);
    {
        char hdr[32];
        num_to_str(NUM_APPS - 1, hdr);
        strcat(hdr, " apps");
        draw_string(MENU_MX + 20, MENU_MY + 30, hdr, 0x94A3B8);
    }
    draw_rect(MENU_MX + 12, MENU_MY + 48, MENU_MW - 24, 1, 0x1E293B);

    /* Search bar */
    int search_y = MENU_MY + MENU_SEARCH_Y;
    draw_rect(MENU_MX + 16, search_y, MENU_MW - 32, 26, 0x1E293B);
    draw_rect(MENU_MX + 16, search_y, MENU_MW - 32, 2, search_focused ? 0x3B82F6 : 0x334155);
    if (search_query[0]) {
        draw_string(MENU_MX + 24, search_y + 7, search_query, 0xE2E8F0);
    } else {
        draw_string(MENU_MX + 24, search_y + 7, "Search apps...", 0x64748B);
    }

    /* Recent apps section (clickable) */
    int recent_y = MENU_MY + MENU_RECENT_Y;
    draw_string(MENU_MX + 20, recent_y, "Recent", 0x94A3B8);
    draw_rect(MENU_MX + 12, recent_y + 20, MENU_MW - 24, 1, 0x1E293B);

    int recent_icon_size = 32;
    int recent_x = MENU_MX + 20;
    int hover = menu_item_at();
    for (int i = 0; i < recent_count && i < 5; i++) {
        int app_idx = recent_apps[i];
        if (app_idx >= 0 && app_idx < NUM_APPS - 1) {
            int ix = recent_x + i * (recent_icon_size + 8);
            int iy = recent_y + 26;
            if (hover == MENU_HIT_RECENT(i)) {
                draw_rect_alpha(ix - 3, iy - 3, recent_icon_size + 6, recent_icon_size + 26, 0x2563EB, 60);
            }
            draw_rect(ix, iy, recent_icon_size, recent_icon_size, 0x1E293B);
            draw_rect(ix, iy, recent_icon_size, 2, 0x3B82F6);
            if (app_idx < (int)(sizeof(menu_icon_buf) / sizeof(menu_icon_buf[0])) && menu_icon_state[app_idx] == 1) {
                icon_bmp_blit(menu_icon_buf[app_idx], ix + (recent_icon_size - MENU_ICON_SIZE) / 2, iy + (recent_icon_size - MENU_ICON_SIZE) / 2);
            } else {
                draw_menu_icon(ix, iy, recent_icon_size, app_idx);
            }
            draw_string_limit(ix, iy + recent_icon_size + 26, apps[app_idx].name, 4, 0xE2E8F0);
        }
    }

    /* Categories section (clickable filters) */
    int cat_y = MENU_MY + MENU_CAT_Y;
    draw_string(MENU_MX + 20, cat_y, "Categories", 0x94A3B8);
    draw_rect(MENU_MX + 12, cat_y + 20, MENU_MW - 24, 1, 0x1E293B);

    int cat_btn_w = (MENU_MW - 32) / 5;
    for (int i = 0; i < 5; i++) {
        int cx = MENU_MX + 16 + i * cat_btn_w;
        int cy = cat_y + 26;
        int selected = (menu_category == i);
        int cat_hover = (hover == MENU_HIT_CAT(i));
        uint32_t bg = selected ? 0x2563EB : (cat_hover ? 0x2D3748 : 0x1E293B);
        draw_rect(cx, cy, cat_btn_w - 4, 24, bg);
        draw_rect(cx, cy, cat_btn_w - 4, 2, selected ? 0x60A5FA : 0x334155);
        draw_string_limit(cx + 4, cy + 8, category_names[i], (cat_btn_w - 8) / 8, selected ? 0xFFFFFF : 0xE2E8F0);
    }
    if (menu_category >= 0) {
        char reset[24];
        strcpy(reset, "Showing: ");
        strcat(reset, category_names[menu_category]);
        strcat(reset, "  (click to clear)");
        draw_string_limit(MENU_MX + 20, cat_y + 54, reset, (MENU_MW - 24) / 8, 0x60A5FA);
    }

    /* Grid of app icons (filtered by category and search) */
    int grid_x = MENU_MX + 20;
    int grid_y = MENU_MY + MENU_GRID_Y;
    int icon_size = MENU_ICON_SIZE2;
    int icon_gap = MENU_ICON_GAP;
    int cols = MENU_COLS;
    int row_h = MENU_ROW_H;

    int displayed = 0;
    for (int i = 0; i < NUM_APPS - 1; i++) {
        if (menu_category >= 0 && apps[i].category != menu_category) continue;
        if (search_query[0]) {
            const char *app_name = apps[i].name;
            const char *query = search_query;
            int match = 0;
            while (*app_name) {
                const char *a = app_name;
                const char *q = query;
                int temp_match = 1;
                while (*a && *q) {
                    if ((*a | 0x20) != (*q | 0x20)) {
                        temp_match = 0;
                        break;
                    }
                    a++;
                    q++;
                }
                if (temp_match && !*q) {
                    match = 1;
                    break;
                }
                app_name++;
            }
            if (!match) continue;
        }

        int row = displayed / cols;
        int col = displayed % cols;
        int ix = grid_x + col * (icon_size + icon_gap);
        int iy = grid_y + row * row_h;

        if (iy + icon_size + 18 > MENU_MY + MENU_MH - 44) break;
        displayed++;

        /* Hover highlight */
        if (i == hover) {
            draw_rect_alpha(ix - 3, iy - 3, icon_size + 6, icon_size + 22, 0x2563EB, 60);
        }

        /* Icon background */
        draw_rect(ix, iy, icon_size, icon_size, 0x1E293B);
        draw_rect(ix, iy, icon_size, 3, 0x3B82F6);

        /* App icon image or glyph */
        if (apps[i].icon && apps[i].icon[0]) {
            if (menu_icon_state[i] == 0) {
                menu_icon_state[i] = icon_bmp_load(apps[i].icon, menu_icon_buf[i]) ? 1 : 2;
            }
            if (menu_icon_state[i] == 1) {
                int off = (icon_size - MENU_ICON_SIZE) / 2;
                icon_bmp_blit(menu_icon_buf[i], ix + off, iy + off);
            } else {
                draw_menu_icon(ix, iy, icon_size, i);
            }
        } else {
            draw_menu_icon(ix, iy, icon_size, i);
        }

        /* App name label - centered under icon */
        int name_len = 0;
        { const char *s = apps[i].name; while (*s) { name_len++; s++; } }
        int label_w = name_len * 8;
        int label_x = ix + (icon_size - label_w) / 2;
        if (label_x < ix) label_x = ix;
        draw_string_limit(label_x, iy + icon_size + 5, apps[i].name, icon_size / 8, 0xE2E8F0);
    }
    if (displayed == 0) {
        draw_string(MENU_MX + 20, grid_y + 10, "No matching apps", 0x64748B);
    }

    /* Power options at bottom */
    int power_y = MENU_MY + MENU_MH - 40;
    int pw = (MENU_MW - 24) / 3;
    const char *power_labels[3] = { "Shutdown", "Restart", "Suspend" };
    uint32_t power_colors[3] = { 0xDC2626, 0x2563EB, 0x8B5CF6 };
    for (int i = 0; i < 3; i++) {
        int px = MENU_MX + 12 + i * pw;
        int active = (hover == MENU_HIT_SHUTDOWN + i);
        draw_rect(px, power_y, pw - 2, 32, active ? power_colors[i] : 0x1E293B);
        draw_string(px + 8, power_y + 10, power_labels[i], active ? 0xFFFFFF : 0xE2E8F0);
    }
}

static void draw_real_clock(int x, int y) {
    uint64_t tb[2];
    if (sys_gettimeofday((uint64_t *)tb) != 0) return;
    int64_t local = (int64_t)tb[0] + (int64_t)sys_timezone_get() * 60;
    if (local < 0) local = 0;
    uint64_t t = (uint64_t)local;

    char clock_str[9];
    clock_str[0] = '0' + (((t / 3600) % 24) / 10);
    clock_str[1] = '0' + (((t / 3600) % 24) % 10);
    clock_str[2] = ':';
    clock_str[3] = '0' + (((t / 60) % 60) / 10);
    clock_str[4] = '0' + (((t / 60) % 60) % 10);
    clock_str[5] = ':';
    clock_str[6] = '0' + ((t % 60) / 10);
    clock_str[7] = '0' + ((t % 60) % 10);
    clock_str[8] = 0;
    draw_string(x, y, clock_str, 0xE2E8F0);

    uint64_t days = t / 86400;
    int year = 1970;
    while (days >= 365) {
        int leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        uint64_t d = leap ? 366 : 365;
        if (days < d) break;
        days -= d;
        year++;
    }
    static const int dims[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    static const char *mnames[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
    int mon = 0;
    while (mon < 12) {
        int d = dims[mon];
        if (mon == 1 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) d = 29;
        if (days < (uint64_t)d) break;
        days -= (uint64_t)d;
        mon++;
    }
    char date_str[16];
    int n = 0;
    const char *mn = mnames[mon];
    while (mn[n]) { date_str[n] = mn[n]; n++; }
    date_str[n++] = ' ';
    int day = (int)days + 1;
    date_str[n++] = '0' + day / 10;
    date_str[n++] = '0' + day % 10;
    date_str[n++] = ' ';
    date_str[n++] = '0' + (year / 1000) % 10;
    date_str[n++] = '0' + (year / 100) % 10;
    date_str[n++] = '0' + (year / 10) % 10;
    date_str[n++] = '0' + year % 10;
    date_str[n] = 0;
    draw_string(x, y + 15, date_str, 0x7C8FA6);
}

static void draw_taskbar(void) {
    draw_rect_alpha(0, SCREEN_HEIGHT - 48, SCREEN_WIDTH, 48, 0x0F1419, 240);
    draw_rect_alpha(0, SCREEN_HEIGHT - 48, SCREEN_WIDTH, 1, 0x1E2736, 255);

    int start_pushed = menu_open || (mouse_btn_down && in_rect(mouse_x, mouse_y, 12, SCREEN_HEIGHT - 42, 90, 36));
    draw_rect(12, SCREEN_HEIGHT - 42, 90, 36, start_pushed ? 0x1E5A8A : 0x2563EB);
    draw_string(32, SCREEN_HEIGHT - 28, "Shadow", 0xFFFFFF);

    int taskbar_x = 112;
    for (int i = 0; i < num_windows; i++) {
        window_t *w = &windows[i];
        int is_top = (i == top_window());
        uint32_t bg = w->minimized ? 0x1E2736 : (is_top ? 0x1E5A8A : 0x2D3748);
        draw_rect(taskbar_x, SCREEN_HEIGHT - 42, 130, 36, bg);
        draw_rect(taskbar_x, SCREEN_HEIGHT - 42, 130, 3, is_top ? 0x60A5FA : 0x4A5568);
        char short_title[12];
        int len = 0;
        while (w->title[len] && len < 10) { short_title[len] = w->title[len]; len++; }
        short_title[len] = 0;
        if (w->title[len]) { short_title[8] = '.'; short_title[9] = '.'; short_title[10] = '.'; short_title[11] = 0; }
        draw_string(taskbar_x + 10, SCREEN_HEIGHT - 26, short_title, 0xFFFFFF);
        taskbar_x += 135;
        if (taskbar_x + 130 > SCREEN_WIDTH - 220) break;
    }

    /* System tray (right side) */
    poll_status();

    int tray_y = SCREEN_HEIGHT - 40;
    /* Mem gauge */
    draw_mem_icon(tray_slot_x(3) + 4, tray_y);
    /* Notification bell */
    int bell_count = 0;
    for (int i = 0; i < MAX_TOASTS; i++) if (toast_active[i] >= 0) bell_count++;
    draw_bell_icon(tray_slot_x(2), tray_y, bell_count);
    /* Bluetooth */
    draw_bt_icon(tray_slot_x(1) + 2, tray_y);
    /* Wi-Fi */
    draw_wifi_icon(tray_slot_x(0) + 14, tray_y + 10);

    /* Highlight the open tray icon */
    if (tray_popup > 0) {
        int sx = tray_slot_x(tray_popup - 1);
        draw_rect(sx - 2, SCREEN_HEIGHT - 44, 32, 40, 0x2563EB);
    }

    draw_real_clock(tray_slot_x(4), SCREEN_HEIGHT - 30);

    draw_popup_panel();
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
    draw_toasts();
    draw_cursor(mouse_x, mouse_y);

    for (uint64_t i = 0; i < total; i++) {
        fb[i] = backbuffer[i];
    }
}

static int taskbar_tab_at(void) {
    int taskbar_x = 112;
    for (int i = 0; i < num_windows; i++) {
        if (in_rect(mouse_x, mouse_y, taskbar_x, SCREEN_HEIGHT - 42, 130, 36)) {
            return i;
        }
        taskbar_x += 135;
        if (taskbar_x + 130 > SCREEN_WIDTH - 220) break;
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
    if (c == 'q') {
        int64_t v = atoi64(w->calc_disp);
        v = v * v;
        num_to_str(v, w->calc_disp);
        w->calc_fresh = 1;
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
        else if (col == 1) calc_input(w, 'q'); /* x squared */
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

static void handle_settings_click(window_t *w);

static void handle_app_click(window_t *w, int idx) {
    if (w->type == WTYPE_ABOUT) {
        if (in_rect(mouse_x, mouse_y, w->x + 110, w->y + TITLE_H + 96, 80, 26)) {
            close_window(idx);
        }
    } else if (w->type == WTYPE_FILE_BRO) {
        int toolbar_y = w->y + TITLE_H + 24;
        if (in_rect(mouse_x, mouse_y, w->x + 10, toolbar_y + 3, 36, 24)) {
            fb_go_up(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 50, toolbar_y + 3, 40, 24)) {
            fb_go_back(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 94, toolbar_y + 3, 40, 24)) {
            fb_go_fwd(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 138, toolbar_y + 3, 44, 24)) {
            fb_navigate(w, "/");
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 186, toolbar_y + 3, 44, 24)) {
            w->fb_sort = !w->fb_sort;
            filebrowser_refresh(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 234, toolbar_y + 3, 36, 24)) {
            /* Copy selected file/directory to clipboard */
            if (w->fb_sel >= 0 && w->fb_sel < w->num_entries) {
                strcpy(w->fb_clipboard, w->current_dir);
                if (w->current_dir[strlen(w->current_dir) - 1] != '/') {
                    strcat(w->fb_clipboard, "/");
                }
                strcat(w->fb_clipboard, w->entries[w->fb_sel].name);
                w->fb_clipboard_valid = 1;
            }
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 274, toolbar_y + 3, 36, 24)) {
            /* Paste from clipboard: copy the file into the current directory */
            if (w->fb_clipboard_valid && w->fb_clipboard[0]) {
                const char *src = w->fb_clipboard;
                char dst[300];
                strcpy(dst, w->current_dir);
                if (strcmp(w->current_dir, "/") != 0) strcat(dst, "/");
                const char *base = src;
                for (const char *p = src; *p; p++) if (*p == '/') base = p + 1;
                strcat(dst, base);
                if (strcmp(dst, src) != 0) {
                    fb_copy_file(src, dst);
                    filebrowser_refresh(w);
                }
                w->fb_clipboard_valid = 0;
                w->fb_clipboard[0] = 0;
            }
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 314, toolbar_y + 3, 36, 24)) {
            /* Show file properties in the status bar */
            if (w->fb_sel > 0 && w->fb_sel < w->num_entries) {
                char st[120];
                strcpy(st, w->entries[w->fb_sel].name);
                strcat(st, is_dir_name(w->entries[w->fb_sel].name) ? "  [dir]" : "  [file]");
                if (w->fb_sizes[w->fb_sel] > 0) {
                    strcat(st, "  ");
                    char nb[16];
                    num_to_str(w->fb_sizes[w->fb_sel], nb);
                    strcat(st, nb);
                    strcat(st, " bytes");
                }
                strcpy(w->fb_status, st);
            }
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 354, toolbar_y + 3, 36, 24)) {
            fb_delete_entry(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 394, toolbar_y + 3, 44, 24)) {
            filebrowser_refresh(w);
            return;
        }
        int list_top = w->y + TITLE_H + 58;
        int rows = (w->h - TITLE_H - 92) / 20;
        for (int i = w->file_scroll; i < w->num_entries && i < w->file_scroll + rows; i++) {
            if (in_rect(mouse_x, mouse_y, w->x + 10, list_top + (i - w->file_scroll) * 20, w->w - 24, 20)) {
                w->fb_sel = i;
                uint64_t now = sys_times(0);
                if (w->fb_double_click == i && now - w->fb_last_click < 30) {
                    w->fb_double_click = -1;
                    fb_open_entry(w);
                } else {
                    w->fb_double_click = i;
                    w->fb_last_click = now;
                }
                return;
            }
        }
    } else if (w->type == WTYPE_CALC) {
        calc_click(w);
    } else if (w->type == WTYPE_PAINT) {
        int tool_row = w->y + TITLE_H + 344;
        int pal_row = w->y + TITLE_H + 380;
        int ctrl_row = w->y + TITLE_H + 414;
        static const uint32_t palette[16] = {
            0x000000, 0xFFFFFF, 0xE74C3C, 0xE67E22,
            0xF1C40F, 0x2ECC71, 0x1ABC9C, 0x3498DB,
            0x2980B9, 0x9B59B6, 0x8E44AD, 0x34495E,
            0x95A5A6, 0xD35400, 0xC0392B, 0x27AE60,
        };
        for (int t = 0; t < 8; t++) {
            if (in_rect(mouse_x, mouse_y, w->x + 10 + t * 55, tool_row, 52, 26)) {
                w->paint_tool = t;
                return;
            }
        }
        for (int p = 0; p < 16; p++) {
            if (in_rect(mouse_x, mouse_y, w->x + 10 + p * 27, pal_row, 24, 20)) {
                w->paint_color = palette[p];
                return;
            }
        }
        if (in_rect(mouse_x, mouse_y, w->x + 10, ctrl_row, 48, 26)) {
            paint_undo(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 62, ctrl_row, 48, 26)) {
            paint_save_undo(w);
            paint_clear(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 114, ctrl_row, 48, 26)) {
            paint_save_bmp(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 166, ctrl_row, 48, 26)) {
            paint_open_bmp(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 340, ctrl_row, 26, 26)) {
            if (w->brush_size > 1) w->brush_size--;
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 420, ctrl_row, 26, 26)) {
            if (w->brush_size < 24) w->brush_size++;
            return;
        }
        int cx = mouse_x - (w->x + 10);
        int cy = mouse_y - (w->y + TITLE_H + 10);
        if (cx >= 0 && cx < 512 && cy >= 0 && cy < 320) {
            paint_save_undo(w);
            w->painting = 1;
            painting_win = idx;
            w->paint_x0 = cx;
            w->paint_y0 = cy;
            w->paint_x1 = cx;
            w->paint_y1 = cy;
            paint_dot(w, cx, cy);
            return;
        }
    } else if (w->type == WTYPE_PROCMON) {
        struct proc_info procs[32];
        int n = sys_proc_info(procs, 32);
        if (n > 0) {
            int list_y = w->y + TITLE_H + 110;
            int rows = (w->h - TITLE_H - 130) / 14;
            int hover_row = (mouse_y - list_y) / 14;
            int pidx = w->proc_scroll + hover_row;
            if (pidx >= 0 && pidx < n && pidx < 32) {
                int row_y = list_y + hover_row * 14;
                int btn_x = w->x + w->w - 20;
                if (in_rect(mouse_x, mouse_y, btn_x, row_y + 1, 12, 12)) {
                    sys_kill((uint64_t)procs[pidx].pid, 9);
                }
            }
        }
    } else if (w->type == WTYPE_SETTINGS) {
        handle_settings_click(w);
    } else if (w->type == WTYPE_BROWSER) {
        int tby = w->y + TITLE_H + 8;
        if (in_rect(mouse_x, mouse_y, w->x + 6, tby, 38, 24)) {
            browser_back(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 46, tby, 38, 24)) {
            browser_fwd(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 86, tby, 40, 24)) {
            browser_refresh(w);
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 128, tby, 32, 24)) {
            browser_navigate(w, "http://93.184.216.34/");
            return;
        }
        int bar_x = w->x + 164;
        int bar_w = w->w - 168;
        if (bar_w < 70) bar_w = 70;
        if (in_rect(mouse_x, mouse_y, bar_x + bar_w - 44, tby, 40, 24)) {
            if (w->url_input[0]) browser_navigate(w, w->url_input);
            return;
        }
        if (in_rect(mouse_x, mouse_y, bar_x, tby, bar_w - 44, 24)) {
            w->url_edit = 1;
            strcpy(w->url_input, w->browser_url);
            return;
        }
    } else if (w->type == WTYPE_STOPWATCH) {
        int by = w->y + TITLE_H + 80;
        int bw = 70, bh = 26;
        if (in_rect(mouse_x, mouse_y, w->x + 12, by, bw, bh)) {
            if (w->sw_running) {
                w->sw_accum += sys_times(0) - w->sw_start;
                w->sw_running = 0;
            } else {
                w->sw_start = sys_times(0);
                w->sw_running = 1;
            }
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 12 + bw + 8, by, bw, bh)) {
            w->sw_running = 0;
            w->sw_accum = 0;
            w->sw_lap_count = 0;
            return;
        }
        if (in_rect(mouse_x, mouse_y, w->x + 12 + (bw + 8) * 2, by, bw, bh)) {
            if (w->sw_lap_count < 8) {
                uint64_t now = sys_times(0);
                w->sw_laps[w->sw_lap_count++] = w->sw_accum + (w->sw_running ? (now - w->sw_start) : 0);
            }
            return;
        }
    }
}

static void handle_settings_click(window_t *w) {
    int x = w->x;
    int y = w->y;
    int win_w = w->w;
    int win_h = w->h;

    /* Check sidebar clicks */
    int sidebar_w = 140;
    int cat_h = 28;
    for (int i = 0; i < 9; i++) {
        int cy = y + TITLE_H + 10 + i * cat_h;
        if (cy + cat_h > y + win_h - 40) break;
        if (in_rect(mouse_x, mouse_y, x + 8, cy, sidebar_w - 16, cat_h - 2)) {
            w->settings_category = i;
            return;
        }
    }

    /* Check content area clicks for toggles */
    int content_x = x + sidebar_w + 20;
    int content_y = y + TITLE_H + 20;
    int content_w = win_w - sidebar_w - 40;
    int content_h = win_h - TITLE_H - 40;

    if (w->settings_category == 0) {
        /* Appearance - toggle dark mode */
        int opt_y = content_y + 28;
        int ts_x = content_x + content_w - 50;
        if (in_rect(mouse_x, mouse_y, ts_x, opt_y - 2, 28, 14)) {
            w->settings_toggle[0] = !w->settings_toggle[0];
        }
    } else if (w->settings_category == 4) {
        /* Power - power saving toggle */
        int opt_y = content_y + 28 + 2 * 22;
        int val_x = content_x + content_w - 50;
        if (in_rect(mouse_x, mouse_y, val_x - 4, opt_y - 2, 28, 14)) {
            w->settings_toggle[12] = !w->settings_toggle[12];
        }
    } else if (w->settings_category == 6) {
        /* Accessibility toggles */
        int opt_y = content_y + 28;
        for (int i = 0; i < 4 && opt_y < content_y + content_h - 10; i++) {
            int ts_x = content_x + content_w - 50;
            if (in_rect(mouse_x, mouse_y, ts_x, opt_y - 2, 28, 14) && mouse_btn_down) {
                w->settings_toggle[i + 20] = !w->settings_toggle[i + 20];
            }
            opt_y += 22;
        }
    } else if (w->settings_category == 8) {
        /* Time & Date: timezone - / + / reset and NTP sync */
        int cx = x + 140 + 30;
        int oy = y + TITLE_H + 70 + 30;
        if (in_rect(mouse_x, mouse_y, cx, oy - 2, 26, 20)) {
            int v = sys_timezone_get();
            sys_timezone_set(v - 30);
            return;
        }
        if (in_rect(mouse_x, mouse_y, cx + 32, oy - 2, 26, 20)) {
            int v = sys_timezone_get();
            sys_timezone_set(v + 30);
            return;
        }
        if (in_rect(mouse_x, mouse_y, cx + 66, oy - 2, 64, 20)) {
            sys_timezone_set(0);
            return;
        }
        if (in_rect(mouse_x, mouse_y, cx, oy + 30, 120, 24)) {
            int64_t offset = 0;
            sb_ntp_sync(0x0A000203, &offset);
            return;
        }
    }
}

static void add_recent_app(int idx) {
    for (int j = recent_count - 1; j >= 0; j--) {
        if (recent_apps[j] == idx) {
            for (int k = j; k < recent_count - 1; k++) {
                recent_apps[k] = recent_apps[k + 1];
            }
            recent_apps[recent_count - 1] = -1;
            recent_count--;
            break;
        }
    }
    if (recent_count < 5) {
        recent_apps[recent_count++] = idx;
    } else {
        for (int j = 0; j < 4; j++) {
            recent_apps[j] = recent_apps[j + 1];
        }
        recent_apps[4] = idx;
    }
}

static void launch_app(int idx) {
    if (idx < 0 || idx >= NUM_APPS - 1) return;
    create_window(apps[idx].type, apps[idx].name, apps[idx].x, apps[idx].y, apps[idx].w, apps[idx].h);
    add_recent_app(idx);
}

static void handle_menu_click(void) {
    int item = menu_item_at();
    if (item == MENU_HIT_NONE) {
        menu_open = 0;
        search_focused = 0;
        menu_category = -1;
        return;
    }
    if (item == MENU_HIT_SEARCH) {
        search_focused = 1;
        return;
    }
    if (item == MENU_HIT_SHUTDOWN) {
        sys_power(1);
        return;
    }
    if (item == MENU_HIT_RESTART) {
        sys_power(0);
        return;
    }
    if (item == MENU_HIT_SUSPEND) {
        sys_power(2);
        return;
    }
    if (item >= MENU_HIT_CAT(0) && item <= MENU_HIT_CAT(4)) {
        int cat = item + 200;
        if (menu_category == cat) {
            menu_category = -1; /* click active category again to clear filter */
        } else {
            menu_category = cat;
        }
        return;
    }
    if (item >= MENU_HIT_RECENT(0) && item <= MENU_HIT_RECENT(4)) {
        int ri = item + 300;
        if (ri >= 0 && ri < recent_count) {
            int idx = recent_apps[ri];
            if (idx >= 0 && idx < NUM_APPS - 1) {
                menu_open = 0;
                search_focused = 0;
                search_query[0] = 0;
                menu_category = -1;
                launch_app(idx);
            }
        }
        return;
    }
    if (item >= 0 && item < NUM_APPS - 1) {
        menu_open = 0;
        search_focused = 0;
        search_query[0] = 0;
        menu_category = -1;
        launch_app(item);
    }
}

static int icon_at(void) {
    for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
        if (in_rect(mouse_x, mouse_y, desktop_icons[i].x - 4, desktop_icons[i].y - 4, 40, 52)) {
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

    /* Notification toasts - click to dismiss */
    for (int i = 0; i < MAX_TOASTS; i++) {
        if (toast_active[i] < 0) continue;
        int x = SCREEN_WIDTH - 330, y = toast_slot_y(i);
        if (in_rect(mouse_x, mouse_y, x - 1, y - 1, 322, 68)) {
            sys_notify_dismiss(notify_list[toast_active[i]].id);
            toast_active[i] = -1;
            tray_popup = 0;
            return;
        }
    }

    /* System tray popup */
    if (popup_open()) {
        int x, y, w, h;
        popup_rect(&x, &y, &w, &h);
        int cx, cy, cw, ch;
        popup_close_rect(x, y, w, &cx, &cy, &cw, &ch);
        if (in_rect(mouse_x, mouse_y, x, y, w, h)) {
            if (in_rect(mouse_x, mouse_y, cx, cy, cw, ch)) {
                tray_popup = 0;
                return;
            }
            if (tray_popup == 3) {
                int yy = y + 36;
                for (int i = 0; i < notify_count && yy + 16 < y + h - 8; i++) {
                    if (in_rect(mouse_x, mouse_y, x + 8, yy, w - 16, 30)) {
                        sys_notify_dismiss(notify_list[i].id);
                        for (int j = i; j < notify_count - 1; j++) notify_list[j] = notify_list[j + 1];
                        notify_count--;
                        for (int t = 0; t < MAX_TOASTS; t++)
                            if (toast_active[t] > i) toast_active[t]--;
                        break;
                    }
                    yy += 30;
                }
            }
            return; /* click consumed by popup */
        }
        tray_popup = 0; /* click outside closes the popup */
    }

    if (menu_open) {
        handle_menu_click();
        return;
    }

    if (mouse_y >= SCREEN_HEIGHT - 48) {
        if (in_rect(mouse_x, mouse_y, 12, SCREEN_HEIGHT - 42, 90, 36)) {
            menu_open = !menu_open;
            return;
        }
        int tslot = tray_slot_at(mouse_x);
        if (tslot >= 0) {
            tray_popup = (tray_popup == tslot + 1) ? 0 : tslot + 1;
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
            int type = desktop_icons[ic].type;
            for (int a = 0; a < NUM_APPS - 1; a++) {
                if (apps[a].type == type) {
                    launch_app(a);
                    break;
                }
            }
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
            drag_active = 0;
            drag_press_x = mouse_x;
            drag_press_y = mouse_y;
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
    drag_active = 0;
    if (resize_win >= 0) resize_win = -1;
    if (painting_win >= 0) {
        window_t *w = &windows[painting_win];
        w->painting = 0;
        if (w->type == WTYPE_PAINT &&
            (w->paint_tool == PAINT_TOOL_LINE || w->paint_tool == PAINT_TOOL_RECT || w->paint_tool == PAINT_TOOL_ELLIPSE) &&
            (w->paint_x1 != w->paint_x0 || w->paint_y1 != w->paint_y0)) {
            paint_commit_shape(w);
        }
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
    /* Handle search input when menu is open and search is focused */
    if (menu_open && search_focused) {
        if (ch == '\b' || code == KSC_BACKSPACE) {
            int len = 0;
            while (search_query[len]) len++;
            if (len > 0) {
                search_query[len - 1] = 0;
            }
        } else if (ch >= ' ' && ch <= '~' && ch < 127) {
            int len = 0;
            while (search_query[len]) len++;
            if (len < 31) {
                search_query[len] = ch;
                search_query[len + 1] = 0;
            }
        } else if (code == KSC_ESC) {
            search_focused = 0;
            search_query[0] = 0;
            menu_category = -1;
        }
        return;
    }

    /* Keep only Ctrl+Alt+T to open a terminal */
    if (ctrl_pressed && alt_pressed && (ch == 't' || ch == 'T')) {
        for (int i = 0; i < NUM_APPS - 1; i++) {
            if (apps[i].type == WTYPE_TERMINAL) {
                launch_app(i);
                break;
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
    if (w->type == WTYPE_FILE_BRO) {
        if (code == KSC_DOWN) {
            if (w->fb_sel < w->num_entries - 1) {
                w->fb_sel++;
                int rows = (w->h - TITLE_H - 92) / 20;
                if (w->fb_sel >= w->file_scroll + rows) w->file_scroll++;
            }
        } else if (code == KSC_UP) {
            if (w->fb_sel > 0) {
                w->fb_sel--;
                if (w->fb_sel < w->file_scroll) w->file_scroll--;
            }
        } else if (ch == '\n') {
            if (w->fb_sel >= 0) fb_open_entry(w);
        } else if (ch == '\b' || code == 0x0E) {
            fb_go_up(w);
        }
        return;
    }
    if (w->type == WTYPE_FORTUNE) {
        if (ch == 'n' || ch == 'N') fortune_next(w);
        return;
    }
    if (w->type == WTYPE_PROCMON) {
        if (code == KSC_UP) {
            w->proc_scroll--;
            if (w->proc_scroll < 0) w->proc_scroll = 0;
        }
        else if (code == KSC_DOWN) {
            struct proc_info procs[32];
            int n = sys_proc_info(procs, 32);
            int rows = (w->h - TITLE_H - 130) / 14;
            if (n > rows && w->proc_scroll < n - rows) w->proc_scroll++;
        }
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

    if (w->type == WTYPE_NOTES) {
        if (code == KSC_ESC) {
            w->notes_len = 0;
            w->notes_text[0] = 0;
            w->notes_scroll = 0;
        } else if (code == KSC_UP) {
            if (w->notes_scroll > 0) w->notes_scroll--;
        } else if (code == KSC_DOWN) {
            w->notes_scroll++;
        } else if (code == KSC_PGUP) {
            w->notes_scroll -= 8;
        } else if (code == KSC_PGDN) {
            w->notes_scroll += 8;
        } else if (ch == '\b') {
            if (w->notes_len > 0) {
                w->notes_len--;
                w->notes_text[w->notes_len] = 0;
            }
        } else if (ch == '\n') {
            if (w->notes_len < 510) {
                w->notes_text[w->notes_len++] = '\n';
                w->notes_text[w->notes_len] = 0;
            }
        } else if (ch >= 32 && ch < 127) {
            if (w->notes_len < 510) {
                w->notes_text[w->notes_len++] = ch;
                w->notes_text[w->notes_len] = 0;
            }
        }
        return;
    }

    if (w->type == WTYPE_STOPWATCH) {
        if (ch == ' ') {
            if (w->sw_running) {
                w->sw_accum += sys_times(0) - w->sw_start;
                w->sw_running = 0;
            } else {
                w->sw_start = sys_times(0);
                w->sw_running = 1;
            }
        } else if (ch == 'r' || ch == 'R') {
            w->sw_running = 0;
            w->sw_accum = 0;
            w->sw_lap_count = 0;
        } else if (ch == 'l' || ch == 'L') {
            if (w->sw_lap_count < 8) {
                uint64_t now = sys_times(0);
                w->sw_laps[w->sw_lap_count++] = w->sw_accum + (w->sw_running ? (now - w->sw_start) : 0);
            }
        }
        return;
    }

    if (w->type == WTYPE_BROWSER) {
        if (w->url_edit) {
            if (ch == '\n') {
                w->url_edit = 0;
                if (w->url_input[0]) browser_navigate(w, w->url_input);
            } else if (ch == '\b') {
                int l = 0;
                while (w->url_input[l]) l++;
                if (l > 0) w->url_input[l - 1] = 0;
            } else if (ch >= 32 && ch < 127) {
                int l = 0;
                while (w->url_input[l]) l++;
                if (l < (int)sizeof(w->url_input) - 1) {
                    w->url_input[l] = ch;
                    w->url_input[l + 1] = 0;
                }
            } else if (code == KSC_UP) {
                if (w->browser_scroll > 0) w->browser_scroll--;
            } else if (code == KSC_DOWN) {
                w->browser_scroll++;
            }
        } else {
            if (ch == 'g' || ch == 'G' || ch == 'l' || ch == 'L') {
                w->url_edit = 1;
                strcpy(w->url_input, w->browser_url);
            } else if (code == KSC_UP) {
                if (w->browser_scroll > 0) w->browser_scroll--;
            } else if (code == KSC_DOWN) {
                w->browser_scroll++;
            } else if (code == KSC_PGUP) {
                w->browser_scroll -= 10;
                if (w->browser_scroll < 0) w->browser_scroll = 0;
            } else if (code == KSC_PGDN) {
                w->browser_scroll += 10;
            } else if (ch == 'r' || ch == 'R') {
                browser_refresh(w);
            }
        }
        return;
    }

    if (w->type == WTYPE_TERMINAL) {
        term_forward(w, code, ch);
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

static void scroll_top_window(int amount) {
    if (num_windows == 0) return;
    window_t *w = &windows[top_window()];
    if (w->type == WTYPE_TERMINAL) {
        w->term_scroll += amount;
        if (w->term_scroll < 0) w->term_scroll = 0;
    } else if (w->type == WTYPE_FILE_BRO) {
        w->file_scroll += amount;
        if (w->file_scroll < 0) w->file_scroll = 0;
    } else if (w->type == WTYPE_PROCMON) {
        w->proc_scroll += amount;
        if (w->proc_scroll < 0) w->proc_scroll = 0;
    } else if (w->type == WTYPE_HEXVIEW) {
        w->hex_offset += amount * 16;
        if (w->hex_offset < 0) w->hex_offset = 0;
    } else if (w->type == WTYPE_BROWSER) {
        w->browser_scroll += amount;
        if (w->browser_scroll < 0) w->browser_scroll = 0;
    }
}

static void cycle_windows(int dir) {
    if (num_windows < 2) return;
    window_t tmp = windows[num_windows - 1];
    if (dir > 0) {
        for (int i = num_windows - 1; i > 0; i--) windows[i] = windows[i - 1];
        windows[0] = tmp;
    } else {
        for (int i = 0; i < num_windows - 1; i++) windows[i] = windows[i + 1];
        windows[num_windows - 1] = tmp;
    }
    for (int i = 0; i < num_windows; i++) windows[i].id = i;
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
                    if (!drag_active) {
                        int dx = mouse_x - drag_press_x;
                        int dy = mouse_y - drag_press_y;
                        if (dx < -DRAG_THRESHOLD || dx > DRAG_THRESHOLD ||
                            dy < -DRAG_THRESHOLD || dy > DRAG_THRESHOLD) {
                            drag_active = 1;
                        }
                    }
                    if (drag_active) {
                        windows[drag_win].x = mouse_x - drag_off_x;
                        windows[drag_win].y = mouse_y - drag_off_y;
                        /* Keep the window fully on-screen and above the taskbar */
                        if (windows[drag_win].x < 0) windows[drag_win].x = 0;
                        if (windows[drag_win].y < 0) windows[drag_win].y = 0;
                        int mx = SCREEN_WIDTH - windows[drag_win].w;
                        if (mx < 0) mx = 0;
                        int my = (SCREEN_HEIGHT - 48) - windows[drag_win].h;
                        if (my < 0) my = 0;
                        if (windows[drag_win].x > mx) windows[drag_win].x = mx;
                        if (windows[drag_win].y > my) windows[drag_win].y = my;
                    }
                }
                if (resize_win >= 0) {
                    window_t *w = &windows[resize_win];
                    int nw = mouse_x - w->x;
                    int nh = mouse_y - w->y;
                    if (nw < 120) nw = 120;
                    if (nh < 80) nh = 80;
                    /* Cap resize at screen bounds (above taskbar) */
                    int max_w = SCREEN_WIDTH - w->x;
                    if (max_w < 120) max_w = 120;
                    int max_h = (SCREEN_HEIGHT - 48) - w->y;
                    if (max_h < 80) max_h = 80;
                    if (nw > max_w) nw = max_w;
                    if (nh > max_h) nh = max_h;
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
                        if (cx < 0) cx = 0;
                        if (cx > 511) cx = 511;
                        if (cy < 0) cy = 0;
                        if (cy > 319) cy = 319;
                        if (w->paint_tool == PAINT_TOOL_LINE || w->paint_tool == PAINT_TOOL_RECT || w->paint_tool == PAINT_TOOL_ELLIPSE) {
                            w->paint_x1 = cx;
                            w->paint_y1 = cy;
                        } else if (w->paint_tool != PAINT_TOOL_FILL) {
                            paint_dot(w, cx, cy);
                        }
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
                } else if (ev.code == 3) {
                    scroll_top_window(ev.y);
                    dirty = 1;
                }
            } else if (ev.type == 4) {
                if (ev.code == 0) {
                    scroll_top_window(ev.y > 0 ? -1 : 1);
                    dirty = 1;
                } else if (ev.code == 2) {
                    cycle_windows(ev.x >= 0 ? 1 : -1);
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

        if (update_terminals()) dirty = 1;

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
