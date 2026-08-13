#include "sys.h"

#include "font8x8.h"

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static uint32_t *fb = (uint32_t *)0x78000000ULL;

static uint32_t *backbuffer = NULL;
static uint32_t *wallpaper_buffer = NULL;
static uint32_t *logo_buffer = NULL;

#define WTYPE_TERMINAL  0
#define WTYPE_FILE_BRO  1
#define WTYPE_SYS_MON   2
#define WTYPE_ABOUT     3
#define WTYPE_VIEWER    4
#define WTYPE_SNAKE     5

#define MAX_WINDOWS 8
typedef struct {
    int id;
    int active;
    int type;
    int x, y, w, h;
    char title[64];
    uint32_t bg_color;
    
    // Terminal / App state
    char text[24 * 60];
    int cursor_x, cursor_y;
    
    // For File Browser
    struct dirent entries[32];
    int num_entries;
    char current_dir[128];
    
    // For SysMon
    uint64_t last_update;
    
    // For Snake Game
    int snake_x[64];
    int snake_y[64];
    int snake_len;
    int snake_dir; // 0=Up, 1=Right, 2=Down, 3=Left
    int food_x, food_y;
    int snake_dead;
} window_t;

static window_t windows[MAX_WINDOWS];
static int num_windows = 0;

static int mouse_x = SCREEN_WIDTH / 2;
static int mouse_y = SCREEN_HEIGHT / 2;
static int mouse_btn_down = 0;
static int drag_win = -1;
static int drag_off_x = 0;
static int drag_off_y = 0;
static int menu_open = 0;

static const char kbd_us_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', 
  '9', '0', '-', '=', '\b', 
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 
  0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, 
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 
  '*', 0, ' ', 0 
};
static int shift_pressed = 0;

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
    if (c < 0 || c > 127) return;
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

static void draw_number(int x, int y, uint64_t num, uint32_t color) {
    char buf[32];
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
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

static void draw_drop_shadow(int x, int y, int w, int h) {
    // Drop shadow effect using alpha blending
    int shadow_size = 10;
    for (int s = 0; s < shadow_size; s++) {
        uint8_t alpha = 50 - (s * 5); // fade out
        draw_rect_alpha(x + w + s, y + s, 1, h, 0x000000, alpha); // Right edge
        draw_rect_alpha(x + s, y + h + s, w, 1, 0x000000, alpha); // Bottom edge
    }
    // Corner
    for(int sy=0; sy<shadow_size; sy++) {
        for(int sx=0; sx<shadow_size; sx++) {
            uint8_t alpha = 50 - ((sx+sy)*2.5);
            if (alpha > 50) alpha = 0;
            draw_rect_alpha(x+w+sx, y+h+sy, 1, 1, 0x000000, alpha);
        }
    }
}

static void draw_cursor(int x, int y) {
    // Draw a clear, larger square cursor for visibility
    // Outer black border
    draw_rect_alpha(x - 2, y - 2, 14, 14, 0x000000, 255);
    // Inner white square
    draw_rect_alpha(x - 1, y - 1, 12, 12, 0xFFFFFF, 255);
}

static void draw_window(window_t *w) {
    if (!w->active) return;
    
    // Draw drop shadow
    draw_drop_shadow(w->x, w->y, w->w, w->h);
    
    // Modern flat title bar colors
    uint32_t bg_color = 0xFFFFFF; // White background
    uint32_t title_bg = (drag_win == w->id || (drag_win == -1 && w->id == num_windows - 1)) ? 0x2980B9 : 0x95A5A6;
    
    // Main window background with slight transparency (Glassmorphism)
    draw_rect_alpha(w->x, w->y, w->w, w->h, bg_color, 240);
    
    // Draw title bar
    draw_rect(w->x, w->y, w->w, 24, title_bg);
    draw_string(w->x + 10, w->y + 8, w->title, 0xFFFFFF);
    
    // Close button (red dot modern style)
    draw_rect(w->x + w->w - 24, w->y, 24, 24, 0xE74C3C);
    draw_string(w->x + w->w - 16, w->y + 8, "x", 0xFFFFFF);
    
    if (w->type == WTYPE_TERMINAL) {
        draw_rect_alpha(w->x, w->y + 24, w->w, w->h - 24, 0x1E1E1E, 230); // dark transparent terminal
        for (int row = 0; row < 24; row++) {
            for (int col = 0; col < 60; col++) {
                char c = w->text[row * 60 + col];
                if (c) draw_char(w->x + 8 + col * 8, w->y + 32 + row * 12, c, 0x00FF00); // Hacker green text
            }
        }
        uint64_t ticks = sys_times(0);
        if ((ticks / 50) % 2 == 0) { 
            draw_rect(w->x + 8 + w->cursor_x * 8, w->y + 32 + w->cursor_y * 12, 8, 12, 0x00FF00);
        }
    } else if (w->type == WTYPE_FILE_BRO) {
        char title_buf[200];
        strcpy(title_buf, "ShadowBox Disk - ");
        strcat(title_buf, w->current_dir);
        draw_string(w->x + 10, w->y + 34, title_buf, 0x333333);
        draw_rect(w->x + 10, w->y + 49, w->w - 20, 1, 0xBDC3C7);
        
        for (int i = 0; i < w->num_entries; i++) {
            // Draw folder/file icon placeholder (if no extension, assume folder)
            uint32_t icon_color = 0x95A5A6; // Default file
            int is_dir = 1;
            for(int k=0; w->entries[i].name[k]; k++) {
                if (w->entries[i].name[k] == '.') is_dir = 0;
            }
            if (is_dir) icon_color = 0xF39C12; // Folder orange
            
            draw_rect(w->x + 10, w->y + 59 + i * 20, 12, 10, icon_color);
            draw_string(w->x + 28, w->y + 60 + i * 20, w->entries[i].name, 0x2C3E50);
        }
    } else if (w->type == WTYPE_SYS_MON) {
        // Update system monitor data periodically (≈0.5 s)
        uint64_t now = sys_times(0);
        if (now - w->last_update > 50) {
            w->last_update = now;
            // Fetch total and used memory (bytes) into w->text buffer (2 uint64_t)
            sys_mem_info((uint64_t*)w->text);
        }
        // Draw system monitor UI
        draw_string(w->x + 10, w->y + 34, "ShadowBox System Monitor", 0x2C3E50);
        // Memory stats (display in MB)
        uint64_t total_mb = ((uint64_t*)w->text)[0] / (1024 * 1024);
        uint64_t used_mb  = ((uint64_t*)w->text)[1] / (1024 * 1024);
        draw_string(w->x + 10, w->y + 54, "Memory Total: ", 0x2980B9);
        draw_number(w->x + 140, w->y + 54, total_mb, 0x2C3E50);
        draw_string(w->x + 200, w->y + 54, "MB", 0x2C3E50);
        draw_string(w->x + 10, w->y + 74, "Memory Used: ", 0x2980B9);
        draw_number(w->x + 140, w->y + 74, used_mb, 0x2C3E50);
        draw_string(w->x + 200, w->y + 74, "MB", 0x2C3E50);
        // Driver list (static)
        draw_string(w->x + 10, w->y + 94, "Drivers Loaded:", 0x2980B9);
        draw_string(w->x + 10, w->y + 109, "- AHCI (SATA)", 0x34495E);
        draw_string(w->x + 10, w->y + 124, "- e1000 & rtl8139 (Net)", 0x34495E);
        draw_string(w->x + 10, w->y + 139, "- Universal Input Manager", 0x34495E);
        draw_string(w->x + 10, w->y + 154, "- ACPI / APIC / IOAPIC", 0x34495E);
        draw_string(w->x + 10, w->y + 169, "- VESA Framebuffer", 0x34495E);
        draw_string(w->x + 10, w->y + 184, "- ShadowBox Compositor", 0x34495E);
    } else if (w->type == WTYPE_ABOUT) {
        draw_rect(w->x + 2, w->y + 22, w->w - 4, w->h - 24, 0xFFFFFF);
        draw_string(w->x + 20, w->y + 50, "ShadowBox OS v6de5495", 0x333333);
        draw_string(w->x + 20, w->y + 70, "By: darkdevil404", 0x333333);
        
        draw_rect(w->x + 100, w->y + 120, 60, 25, 0x2980B9);
        draw_string(w->x + 115, w->y + 127, "OK", 0xFFFFFF);
    } else if (w->type == WTYPE_VIEWER) {
        draw_rect(w->x + 2, w->y + 22, w->w - 4, w->h - 24, 0x1E1E1E);
        if (logo_buffer) {
            int img_w = 256;
            int img_h = 256;
            int start_x = w->x + (w->w - img_w) / 2;
            int start_y = w->y + 22 + (w->h - 24 - img_h) / 2;
            
            for (int iy = 0; iy < img_h; iy++) {
                for (int ix = 0; ix < img_w; ix++) {
                    int px = start_x + ix;
                    int py = start_y + iy;
                    if (px > w->x + 2 && px < w->x + w->w - 2 && py > w->y + 22 && py < w->y + w->h - 2) {
                        backbuffer[py * SCREEN_WIDTH + px] = logo_buffer[iy * img_w + ix];
                    }
                }
            }
        }
    } else if (w->type == WTYPE_SNAKE) {
        draw_rect(w->x + 2, w->y + 22, w->w - 4, w->h - 24, 0x000000); // Black background
        if (w->snake_dead) {
            draw_string(w->x + w->w/2 - 40, w->y + w->h/2 - 10, "GAME OVER", 0xFF0000);
            draw_string(w->x + w->w/2 - 60, w->y + w->h/2 + 10, "Press R to Restart", 0xFFFFFF);
        } else {
            // Draw food
            draw_rect(w->x + 2 + w->food_x * 10, w->y + 22 + w->food_y * 10, 10, 10, 0xE74C3C);
            // Draw snake
            for (int i = 0; i < w->snake_len; i++) {
                uint32_t color = (i == 0) ? 0x2ECC71 : 0x27AE60;
                draw_rect(w->x + 2 + w->snake_x[i] * 10, w->y + 22 + w->snake_y[i] * 10, 10, 10, color);
            }
        }
    }
}

static void draw_start_menu(void) {
    if (!menu_open) return;
    
    int mx = 10;
    int my = SCREEN_HEIGHT - 40 - 220;
    int mw = 200;
    int mh = 220;
    
    // Start menu with shadow and glassmorphism
    draw_drop_shadow(mx, my, mw, mh);
    draw_rect_alpha(mx, my, mw, mh, 0x2C3E50, 230); // dark transparent
    
    // Banner
    draw_string(mx + 20, my + 15, "ShadowBox OS", 0x3498DB);
    draw_rect(mx + 20, my + 30, mw - 40, 1, 0x34495E);
    
    int hover = -1;
    if (mouse_x >= mx && mouse_x <= mx + mw && mouse_y >= my) {
        if (mouse_y >= my + 40 && mouse_y < my + 70) hover = 0;
        else if (mouse_y >= my + 70 && mouse_y < my + 100) hover = 1;
        else if (mouse_y >= my + 100 && mouse_y < my + 130) hover = 2;
        else if (mouse_y >= my + 130 && mouse_y < my + 160) hover = 3;
        else if (mouse_y >= my + 160 && mouse_y < my + 190) hover = 4;
        else if (mouse_y >= my + 190 && mouse_y < my + 220) hover = 5;
    }
    
    if (hover >= 0) {
        draw_rect_alpha(mx, my + 40 + hover * 30, mw, 30, 0x3498DB, 150);
    }
    
    draw_string(mx + 30, my + 50, "Terminal", 0xECF0F1);
    draw_string(mx + 30, my + 80, "File Explorer", 0xECF0F1);
    draw_string(mx + 30, my + 110, "System Monitor", 0xECF0F1);
    draw_string(mx + 30, my + 140, "Image Viewer", 0xECF0F1);
    draw_string(mx + 30, my + 170, "About", 0xECF0F1);
    draw_string(mx + 30, my + 200, "Shutdown", 0xE74C3C);
}

static void draw_desktop(void) {
    // Draw wallpaper
    uint64_t total = SCREEN_WIDTH * SCREEN_HEIGHT;
    for (uint64_t i = 0; i < total; i++) {
        backbuffer[i] = wallpaper_buffer[i];
    }
    
    // Draw all windows
    for (int i = 0; i < num_windows; i++) {
        draw_window(&windows[i]);
    }
    
    draw_start_menu();
    
    // Modern transparent Taskbar
    draw_rect_alpha(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, 0x1C2833, 200);
    draw_rect_alpha(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 1, 0x34495E, 255);
    
    // Start button (flat, hover effect simulated if clicked)
    int start_pushed = menu_open || (mouse_btn_down && mouse_x >= 10 && mouse_x <= 90 && mouse_y >= SCREEN_HEIGHT - 35 && mouse_y <= SCREEN_HEIGHT - 5);
    draw_rect(10, SCREEN_HEIGHT - 35, 80, 30, start_pushed ? 0x2980B9 : 0x3498DB);
    draw_string(30, SCREEN_HEIGHT - 24, "Shadow", 0xFFFFFF);
    
    // Clock panel
    uint64_t t = sys_times(0) / 100;
    char clock_str[16];
    int h = (t / 3600) % 24;
    int m = (t / 60) % 60;
    int s = t % 60;
    clock_str[0] = '0' + (h / 10); clock_str[1] = '0' + (h % 10); clock_str[2] = ':';
    clock_str[3] = '0' + (m / 10); clock_str[4] = '0' + (m % 10); clock_str[5] = ':';
    clock_str[6] = '0' + (s / 10); clock_str[7] = '0' + (s % 10); clock_str[8] = 0;
    draw_string(SCREEN_WIDTH - 90, SCREEN_HEIGHT - 24, clock_str, 0xECF0F1);
    
    draw_cursor(mouse_x, mouse_y);
    draw_rect(0, 0, 20, 20, 0x00FF00); // debug box
    
    for (uint64_t i = 0; i < total; i++) {
        fb[i] = backbuffer[i];
    }
}

static void create_window(int type, const char *title, int x, int y, int w, int h) {
    if (num_windows >= MAX_WINDOWS) return;
    window_t *win = &windows[num_windows];
    win->id = num_windows;
    win->active = 1;
    win->type = type;
    win->x = x; win->y = y; win->w = w; win->h = h;
    int i = 0; while(title[i] && i<63) { win->title[i] = title[i]; i++; } win->title[i] = 0;
    
    if (type == WTYPE_TERMINAL) {
        for (i = 0; i < 24 * 60; i++) win->text[i] = 0;
        win->cursor_x = 0; win->cursor_y = 0;
        char *welcome = "root@shadowbox:~# ";
        int idx = 0;
        while (welcome[idx]) { win->text[idx] = welcome[idx]; idx++; }
        win->cursor_x = idx;
    } else if (type == WTYPE_FILE_BRO) {
        win->num_entries = 0;
        strcpy(win->current_dir, "/");
        int fd = sb_acquire("/", 0);
        if (fd >= 0) {
            struct dirent d;
            while (sys_getdents(fd, &d, 1) == 1 && win->num_entries < 32) {
                int j=0;
                while(d.name[j]) { win->entries[win->num_entries].name[j] = d.name[j]; j++; }
                win->entries[win->num_entries].name[j] = 0;
                win->num_entries++;
            }
            sb_release(fd);
        }
    } else if (type == WTYPE_SNAKE) {
        win->snake_len = 3;
        win->snake_x[0] = 10; win->snake_y[0] = 10;
        win->snake_x[1] = 9;  win->snake_y[1] = 10;
        win->snake_x[2] = 8;  win->snake_y[2] = 10;
        win->snake_dir = 1;
        win->food_x = 15; win->food_y = 10;
        win->snake_dead = 0;
        win->last_update = sys_times(0);
    }
    
    num_windows++;
}

static void handle_kbd(uint8_t scancode) {
    if (scancode == 0x2A || scancode == 0x36) { shift_pressed = 1; return; }
    char c = kbd_us_map[scancode & 0x7F];
    if (!c) return;
    if (shift_pressed && c >= 'a' && c <= 'z') c -= 32;
    
    // WASD keys move the mouse cursor for quick testing
    if (c == 'w') { mouse_y = (mouse_y > 5) ? mouse_y - 5 : 0; }
    else if (c == 's') { mouse_y = (mouse_y < SCREEN_HEIGHT-5) ? mouse_y + 5 : SCREEN_HEIGHT-2; }
    else if (c == 'a') { mouse_x = (mouse_x > 5) ? mouse_x - 5 : 0; }
    else if (c == 'd') { mouse_x = (mouse_x < SCREEN_WIDTH-5) ? mouse_x + 5 : SCREEN_WIDTH-2; }
    
    if (num_windows == 0) return;
    window_t *w = &windows[num_windows - 1]; 
    if (!w->active) return;
    
    if (w->type == WTYPE_SNAKE) {
        if (c == 'w' && w->snake_dir != 2) w->snake_dir = 0;
        if (c == 'd' && w->snake_dir != 3) w->snake_dir = 1;
        if (c == 's' && w->snake_dir != 0) w->snake_dir = 2;
        if (c == 'a' && w->snake_dir != 1) w->snake_dir = 3;
        if (c == 'r' && w->snake_dead) {
            w->snake_len = 3;
            w->snake_x[0] = 10; w->snake_y[0] = 10;
            w->snake_x[1] = 9;  w->snake_y[1] = 10;
            w->snake_x[2] = 8;  w->snake_y[2] = 10;
            w->snake_dir = 1;
            w->snake_dead = 0;
        }
        return;
    }
    
    if (w->type != WTYPE_TERMINAL) return;
    
    if (c == '\b') {
        if (w->cursor_x > 0) {
            w->cursor_x--;
            w->text[w->cursor_y * 60 + w->cursor_x] = 0;
        }
    } else if (c == '\n') {
        w->cursor_x = 0;
        if (w->cursor_y < 23) w->cursor_y++;
    } else {
        w->text[w->cursor_y * 60 + w->cursor_x] = c;
        w->cursor_x++;
        if (w->cursor_x >= 60) {
            w->cursor_x = 0;
            if (w->cursor_y < 23) w->cursor_y++;
        }
    }
}

void _start(void) {
    backbuffer = (uint32_t *)sys_sbrk(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    if ((int64_t)backbuffer < 0 || !backbuffer) {
        syscall1(SB_TERMINATE, 2);
    }
    
    wallpaper_buffer = (uint32_t *)sys_sbrk(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    int wp_fd = sb_acquire("/wallpaper.bmp", 0);
    int loaded = 0;
    if (wp_fd >= 0) {
        uint8_t header[54];
        if (sb_pull(wp_fd, header, 54) == 54) {
            if (header[0] == 'B' && header[1] == 'M') {
                uint32_t offset = *(uint32_t*)&header[10];
                int w = *(int32_t*)&header[18];
                int h = *(int32_t*)&header[22];
                if (offset > 54) {
                    uint8_t dummy[128];
                    int to_skip = offset - 54;
                    while(to_skip > 0) {
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
                        uint8_t b = row_buf[x*3];
                        uint8_t g = row_buf[x*3 + 1];
                        uint8_t r = row_buf[x*3 + 2];
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
    
    // Load Logo (256x256)
    int lg_fd = sb_acquire("/logo.bmp", 0);
    if (lg_fd >= 0) {
        logo_buffer = (uint32_t *)sys_sbrk(256 * 256 * 4);
        uint8_t header[54];
        if (sb_pull(lg_fd, header, 54) == 54 && header[0] == 'B' && header[1] == 'M') {
            uint32_t offset = *(uint32_t*)&header[10];
            int w = *(int32_t*)&header[18];
            int h = *(int32_t*)&header[22];
            
            if (w == 256 && h == 256) {
                if (offset > 54) {
                    uint8_t dummy[128];
                    int to_skip = offset - 54;
                    while(to_skip > 0) {
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
                        uint8_t b = row_buf[x*3];
                        uint8_t g = row_buf[x*3 + 1];
                        uint8_t r = row_buf[x*3 + 2];
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
    
    create_window(WTYPE_ABOUT, "Welcome to ShadowBox OS", 350, 200, 320, 180);
    create_window(WTYPE_SYS_MON, "System Monitor", 100, 100, 350, 200);
    
    // Map the framebuffer into user space
    sys_fb_mmap();
    sb_push(1, "Desktop: fb mapped\n", 19);
    
    draw_desktop();
    sb_push(1, "Desktop: initial draw done\n", 27);
    
    int input_fd = sb_acquire("/dev/input", 0);
    if (input_fd < 0) {
        sb_push(1, "Desktop: failed to open input\n", 31);
        syscall1(SB_TERMINATE, 3);
    }
    sb_push(1, "Desktop: input opened\n", 23);
    
    int prev_buttons = 0;
    
    
    uint64_t last_draw_time = sys_times(0);

    while (1) {
        input_event_t ev;
        if (sb_pull(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == 2) { // movement packet (no buttons)
                mouse_x += ev.x; mouse_y -= ev.y;
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > SCREEN_WIDTH - 2) mouse_x = SCREEN_WIDTH - 2;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > SCREEN_HEIGHT - 2) mouse_y = SCREEN_HEIGHT - 2;
                
                if (drag_win >= 0) {
                    windows[drag_win].x = mouse_x - drag_off_x;
                    windows[drag_win].y = mouse_y - drag_off_y;
                }
    } else if (ev.type == 3) { // button packet
        if (ev.value == 1) {
            mouse_btn_down = 1;
            int handled = 0;
            if (menu_open) {
                int mx = 10, my = SCREEN_HEIGHT - 40 - 230;
                if (mouse_x >= mx && mouse_x <= mx+200 && mouse_y >= my && mouse_y <= my+220) {
                    if (mouse_y >= my+40 && mouse_y < my+70) create_window(WTYPE_TERMINAL, "ShadowBox Terminal", 150, 150, 550, 350);
                    else if (mouse_y >= my+70 && mouse_y < my+100) create_window(WTYPE_FILE_BRO, "File Explorer", 100, 100, 300, 300);
                    else if (mouse_y >= my+100 && mouse_y < my+130) create_window(WTYPE_SYS_MON, "System Monitor", 200, 200, 350, 200);
                    else if (mouse_y >= my+130 && mouse_y < my+160) create_window(WTYPE_VIEWER, "Image Viewer", 220, 220, 300, 300);
                    else if (mouse_y >= my+160 && mouse_y < my+190) create_window(WTYPE_SNAKE, "Snake Game", 260, 200, 304, 324);
                    else if (mouse_y >= my+190 && mouse_y < my+220) create_window(WTYPE_ABOUT, "About ShadowBox", 250, 250, 320, 180);
                    menu_open = 0;
                    handled = 1;
                }
            }
            if (!handled && mouse_y >= SCREEN_HEIGHT - 40) {
                if (mouse_x >= 10 && mouse_x <= 90) {
                    menu_open = !menu_open;
                } else {
                    menu_open = 0;
                }
                handled = 1;
            } else if (!handled) {
                menu_open = 0;
            }
            if (!handled) {
                for (int i = num_windows - 1; i >= 0; i--) {
                    window_t *w = &windows[i];
                    if (mouse_x >= w->x && mouse_x <= w->x + w->w && mouse_y >= w->y && mouse_y <= w->y + 24) {
                        if (mouse_x >= w->x + w->w - 24 && mouse_x <= w->x + w->w && mouse_y >= w->y && mouse_y <= w->y + 24) {
                            for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j+1];
                            num_windows--;
                            drag_win = -1;
                            break;
                        }
                        drag_win = i;
                        drag_off_x = mouse_x - w->x;
                        drag_off_y = mouse_y - w->y;
                        window_t temp = *w;
                        for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j+1];
                        windows[num_windows - 1] = temp;
                        drag_win = num_windows - 1;
                        break;
                    } else if (mouse_x >= w->x && mouse_x <= w->x + w->w && mouse_y >= w->y && mouse_y <= w->y + w->h) {
                        if (w->type == WTYPE_ABOUT && mouse_x >= w->x + 100 && mouse_x <= w->x + 160 && mouse_y >= w->y + 120 && mouse_y <= w->y + 145) {
                            for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j+1];
                            num_windows--;
                            break;
                        }
                        window_t temp = *w;
                        for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j+1];
                        windows[num_windows - 1] = temp;
                        if (temp.type == WTYPE_FILE_BRO && mouse_y >= temp.y + 59) {
                            int idx = (mouse_y - (temp.y + 59)) / 20;
                            if (idx >= 0 && idx < temp.num_entries) {
                                window_t *top = &windows[num_windows - 1];
                                if (strcmp(top->current_dir, "/") != 0) strcat(top->current_dir, "/");
                                strcat(top->current_dir, top->entries[idx].name);
                                top->num_entries = 0;
                                int fd = sb_acquire(top->current_dir, 0);
                                if (fd >= 0) {
                                    struct dirent d;
                                    while (sys_getdents(fd, &d, 1) == 1 && top->num_entries < 32) {
                                        strcpy(top->entries[top->num_entries].name, d.name);
                                        top->num_entries++;
                                    }
                                    sb_release(fd);
                                } else {
                                    strcpy(top->current_dir, "/");
                                    int rfd = sb_acquire("/", 0);
                                    if (rfd >= 0) {
                                        struct dirent d;
                                        while (sys_getdents(rfd, &d, 1) == 1 && top->num_entries < 32) {
                                            strcpy(top->entries[top->num_entries].name, d.name);
                                            top->num_entries++;
                                        }
                                        sb_release(rfd);
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        } else if (ev.value == 0) {
            mouse_btn_down = 0;
            drag_win = -1;
        }
    }
                draw_desktop();
                last_draw_time = now;
            }
            syscall0(SYS_SCHED_YIELD);
        }
    }
}
