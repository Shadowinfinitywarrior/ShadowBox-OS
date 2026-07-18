#include "fb.h"
#include "kernel.h"
#include "task.h"
#include "keyboard.h"
#include "pic.h"
#include "io.h"

void input_push(uint8_t type, uint8_t code, int16_t x, int16_t y);

static const uint8_t font8x8[96][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x18, 0x3c, 0x3c, 0x18, 0x18, 0x00, 0x18, 0x00},
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x36, 0x36, 0x7f, 0x36, 0x7f, 0x36, 0x36, 0x00},
    {0x0c, 0x3e, 0x03, 0x1e, 0x30, 0x1f, 0x0c, 0x00},
    {0x00, 0x66, 0x6c, 0x38, 0x1c, 0x6c, 0x66, 0x00},
    {0x38, 0x6c, 0x38, 0x76, 0xdc, 0xcc, 0x76, 0x00},
    {0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00},
    {0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00},
    {0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00},
    {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x08},
    {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    {0x00, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00},
    {0x3e, 0x66, 0x6e, 0x76, 0x66, 0x66, 0x3e, 0x00},
    {0x18, 0x1c, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00},
    {0x3e, 0x66, 0x06, 0x0c, 0x30, 0x60, 0x7e, 0x00},
    {0x3e, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3e, 0x00},
    {0x06, 0x0e, 0x1e, 0x66, 0x7f, 0x06, 0x06, 0x00},
    {0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3e, 0x00},
    {0x1c, 0x30, 0x60, 0x7c, 0x66, 0x66, 0x3e, 0x00},
    {0x7e, 0x66, 0x06, 0x0c, 0x18, 0x18, 0x18, 0x00},
    {0x3e, 0x66, 0x66, 0x3e, 0x66, 0x66, 0x3e, 0x00},
    {0x3e, 0x66, 0x66, 0x3e, 0x06, 0x0c, 0x38, 0x00},
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x08, 0x00},
    {0x0c, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0c, 0x00},
    {0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00},
    {0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x30, 0x00},
    {0x3e, 0x66, 0x06, 0x0c, 0x18, 0x00, 0x18, 0x00},
    {0x3e, 0x66, 0x6f, 0x7b, 0x73, 0x60, 0x3e, 0x00},
    {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00},
    {0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00},
    {0x3e, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3e, 0x00},
    {0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00},
    {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x7e, 0x00},
    {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x00},
    {0x3e, 0x66, 0x60, 0x6e, 0x66, 0x66, 0x3e, 0x00},
    {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00},
    {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00},
    {0x06, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00},
    {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00},
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00},
    {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00},
    {0x66, 0x6e, 0x76, 0x66, 0x66, 0x66, 0x66, 0x00},
    {0x3e, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00},
    {0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00},
    {0x3e, 0x66, 0x66, 0x66, 0x6a, 0x6c, 0x36, 0x00},
    {0x7c, 0x66, 0x66, 0x7c, 0x78, 0x6c, 0x66, 0x00},
    {0x3e, 0x66, 0x30, 0x1c, 0x06, 0x66, 0x3e, 0x00},
    {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00},
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00},
    {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00},
    {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00},
    {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00},
    {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00},
    {0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00},
    {0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x00},
    {0x3c, 0x06, 0x06, 0x06, 0x06, 0x06, 0x3c, 0x00},
    {0x18, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00},
    {0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x3e, 0x06, 0x3e, 0x66, 0x3e, 0x00},
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x00},
    {0x00, 0x00, 0x3e, 0x60, 0x60, 0x66, 0x3e, 0x00},
    {0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00},
    {0x00, 0x00, 0x3e, 0x66, 0x7e, 0x60, 0x3e, 0x00},
    {0x1c, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x00},
    {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x3c},
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00},
    {0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    {0x0c, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x38},
    {0x60, 0x66, 0x6c, 0x78, 0x70, 0x6c, 0x66, 0x00},
    {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x1c, 0x00},
    {0x00, 0x00, 0x66, 0x7f, 0x6b, 0x63, 0x63, 0x00},
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00},
    {0x00, 0x00, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00},
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60},
    {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x06},
    {0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x00},
    {0x00, 0x00, 0x3e, 0x60, 0x3e, 0x06, 0x7c, 0x00},
    {0x30, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x1c, 0x00},
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00},
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00},
    {0x00, 0x00, 0x63, 0x6b, 0x7f, 0x36, 0x22, 0x00},
    {0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00},
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x3c},
    {0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00},
    {0x0c, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0c, 0x00},
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},
    {0x30, 0x18, 0x18, 0x0c, 0x18, 0x18, 0x30, 0x00},
    {0x3b, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};


// Mouse cursor bitmap (16x16)
// '.' = transparent, '#' = black, 'X' = white
static const char cursor_bitmap[16][16] = {{
    "X...............",
    "XX..............",
    "XXX.............",
    "XXXX............",
    "XXXXX...........",
    "XXXXXX..........",
    "XXXXXXX.........",
    "XXXXXXXX........",
    "XXXXXXXXX.......",
    "XXXXXX..........",
    "XX.XXX..........",
    "X...XX..........",
    "....XX..........",
    ".....X..........",
    "................",
    "................"
}};

// Mouse state
static int mouse_x = 100;
static int mouse_y = 100;
static uint8_t mouse_buttons = 0;
static uint32_t cursor_saved_bg[256];
static int cursor_saved_x = -1;
static int cursor_saved_y = -1;

// Screen size
static int screen_width = 1024;
static int screen_height = 768;

// Font rendering functions
void fb_draw_char(int x, int y, char c, uint32_t color, uint32_t bg, int use_bg) {{
    if (c < 32 || c > 127) c = 32;
    int idx = c - 32;
    for (int row = 0; row < 8; row++) {{
        uint8_t glyph_row = font8x8[idx][row];
        for (int col = 0; col < 8; col++) {{
            if (glyph_row & (1 << (7 - col))) {{
                fb_putpixel(x + col, y + row, color);
            }} else if (use_bg) {{
                fb_putpixel(x + col, y + row, bg);
            }}
        }}
    }}
}}

void fb_draw_string(int x, int y, const char *s, uint32_t color, uint32_t bg, int use_bg) {{
    int cur_x = x;
    while (*s) {{
        fb_draw_char(cur_x, y, *s, color, bg, use_bg);
        cur_x += 8;
        s++;
    }}
}}

// Drawing shapes
void fb_draw_rect(int x, int y, int w, int h, uint32_t color) {{
    for (int cy = y; cy < y + h; cy++) {{
        for (int cx = x; cx < x + w; cx++) {{
            fb_putpixel(cx, cy, color);
        }}
    }}
}}

void fb_draw_rect_outline(int x, int y, int w, int h, uint32_t color) {{
    for (int cx = x; cx < x + w; cx++) {{
        fb_putpixel(cx, y, color);
        fb_putpixel(cx, y + h - 1, color);
    }}
    for (int cy = y; cy < y + h; cy++) {{
        fb_putpixel(x, cy, color);
        fb_putpixel(x + w - 1, cy, color);
    }}
}}

// Save and restore mouse cursor background
static void save_cursor_bg(int x, int y) {{
    cursor_saved_x = x;
    cursor_saved_y = y;
    for (int cy = 0; cy < 16; cy++) {{
        for (int cx = 0; cx < 16; cx++) {{
            // read pixel and save it
            // We don't have a direct fb_getpixel, so we read directly from the mapped address
            // fb_addr is defined static in fb.c, so we can access it if we export it or read from the physical address.
            // Let's assume we map the framebuffer address locally!
            // Wait, we can get it from fb_tag! Let's export the framebuffer address.
        }}
    }}
}}

// Let's get the framebuffer address from the fb module.
extern uint8_t *fb_get_addr(void);
extern uint32_t fb_get_width(void);
extern uint32_t fb_get_height(void);
extern uint32_t fb_get_pitch(void);
extern uint8_t fb_get_bpp(void);

static uint32_t get_pixel_raw(int x, int y) {{
    uint8_t *fb_addr = fb_get_addr();
    uint32_t pitch = fb_get_pitch();
    uint8_t bpp = fb_get_bpp();
    if (!fb_addr) return 0;
    if (x < 0 || x >= (int)fb_get_width() || y < 0 || y >= (int)fb_get_height()) return 0;
    uint32_t offset = y * pitch + x * (bpp / 8);
    return *(uint32_t*)(fb_addr + offset);
}}

static void put_pixel_raw(int x, int y, uint32_t color) {{
    uint8_t *fb_addr = fb_get_addr();
    uint32_t pitch = fb_get_pitch();
    uint8_t bpp = fb_get_bpp();
    if (!fb_addr) return;
    if (x < 0 || x >= (int)fb_get_width() || y < 0 || y >= (int)fb_get_height()) return;
    uint32_t offset = y * pitch + x * (bpp / 8);
    *(uint32_t*)(fb_addr + offset) = color;
}}

static void save_cursor(int x, int y) {{
    cursor_saved_x = x;
    cursor_saved_y = y;
    for (int cy = 0; cy < 16; cy++) {{
        for (int cx = 0; cx < 16; cx++) {{
            cursor_saved_bg[cy * 16 + cx] = get_pixel_raw(x + cx, y + cy);
        }}
    }}
}}

static void restore_cursor(void) {{
    if (cursor_saved_x == -1) return;
    for (int cy = 0; cy < 16; cy++) {{
        for (int cx = 0; cx < 16; cx++) {{
            put_pixel_raw(cursor_saved_x + cx, cursor_saved_y + cy, cursor_saved_bg[cy * 16 + cx]);
        }}
    }}
}}

static void draw_cursor(int x, int y) {{
    for (int cy = 0; cy < 16; cy++) {{
        for (int cx = 0; cx < 16; cx++) {{
            char pixel = cursor_bitmap[cy][cx];
            if (pixel == 'X') {{
                put_pixel_raw(x + cx, y + cy, 0xFFFFFFFF); // White
            }} else if (pixel == '#') {{
                put_pixel_raw(x + cx, y + cy, 0x00000000); // Black
            }}
        }}
    }}
}}

// Window definitions
typedef struct window {{
    int id;
    char title[64];
    int x, y;
    int w, h;
    int active;
    void (*draw_content)(struct window *self);
    int dragging;
    int drag_offset_x;
    int drag_offset_y;
}} window_t;

#define MAX_WINDOWS 4
static window_t windows[MAX_WINDOWS];
static int num_windows = 0;

// Text Editor Buffer
static char text_editor_buf[1024] = "Type here using your keyboard!\n- Supports backspace\n- Fully interactive\n";
static int text_editor_len = 78;

// Calculator State
static int calc_value = 0;
static char calc_display[32] = "0";

// System Monitor stats
extern uint64_t boot_ticks;

// Draw background (sleek gradient)
static void draw_desktop(void) {{
    // Draw background gradient (dark blue to purple)
    for (int y = 0; y < screen_height; y++) {{
        uint8_t r = 13 + (y * 20 / screen_height);
        uint8_t g = 27 + (y * 10 / screen_height);
        uint8_t b = 42 + (y * 30 / screen_height);
        uint32_t color = (r << 16) | (g << 8) | b;
        for (int x = 0; x < screen_width; x++) {{
            put_pixel_raw(x, y, color);
        }}
    }}
    
    // Draw grid effect (subtle overlay)
    for (int y = 0; y < screen_height; y += 32) {{
        for (int x = 0; x < screen_width; x++) {{
            uint32_t orig = get_pixel_raw(x, y);
            uint8_t r = ((orig >> 16) & 0xFF) + 3;
            uint8_t g = ((orig >> 8) & 0xFF) + 3;
            uint8_t b = (orig & 0xFF) + 3;
            put_pixel_raw(x, y, (r << 16) | (g << 8) | b);
        }}
    }}
    for (int x = 0; x < screen_width; x += 32) {{
        for (int y = 0; y < screen_height; y++) {{
            uint32_t orig = get_pixel_raw(x, y);
            uint8_t r = ((orig >> 16) & 0xFF) + 3;
            uint8_t g = ((orig >> 8) & 0xFF) + 3;
            uint8_t b = (orig & 0xFF) + 3;
            put_pixel_raw(x, y, (r << 16) | (g << 8) | b);
        }}
    }}
    
    // Draw taskbar at the bottom
    fb_draw_rect(0, screen_height - 40, screen_width, 40, 0x1b263b);
    fb_draw_rect(0, screen_height - 40, screen_width, 2, 0x415a77);
    
    // Draw Start button
    fb_draw_rect(10, screen_height - 35, 80, 30, 0x415a77);
    fb_draw_rect_outline(10, screen_height - 35, 80, 30, 0x778da9);
    fb_draw_string(25, screen_height - 24, "Start", 0xFFFFFFFF, 0, 0);
    
    // Draw clock/uptime on right
    char time_str[32];
    uint64_t uptime_seconds = boot_ticks / 100;
    printk_sprintf(time_str, "Uptime: %ds", (uint32_t)uptime_seconds);
    fb_draw_string(screen_width - 150, screen_height - 24, time_str, 0xFFFFFFFF, 0, 0);
    
    // Draw central OS branding
    fb_draw_string(screen_width / 2 - 60, screen_height - 24, "ShadowBox OS v0.2.0", 0x778da9, 0, 0);
}}

// Content drawers
static void draw_system_monitor(window_t *self) {{
    // Background of window content
    fb_draw_rect(self->x + 2, self->y + 24, self->w - 4, self->h - 26, 0x0d1b2a);
    
    // Draw memory stats
    uint64_t total_pages = 0, used_pages = 0;
    extern void pmm_get_info(uint64_t *total, uint64_t *used);
    pmm_get_info(&total_pages, &used_pages);
    
    char buf[128];
    printk_sprintf(buf, "Memory Total: %d MB", (uint32_t)((total_pages * 4096) / (1024 * 1024)));
    fb_draw_string(self->x + 10, self->y + 35, buf, 0xFFFFFFFF, 0, 0);
    
    printk_sprintf(buf, "Memory Used : %d MB (%d%%)", 
                   (uint32_t)((used_pages * 4096) / (1024 * 1024)),
                   (uint32_t)((used_pages * 100) / total_pages));
    fb_draw_string(self->x + 10, self->y + 55, buf, 0x3a86ff, 0, 0);
    
    // Memory progress bar
    fb_draw_rect(self->x + 10, self->y + 75, self->w - 20, 15, 0x1b263b);
    int bar_width = (int)((used_pages * (self->w - 20)) / total_pages);
    fb_draw_rect(self->x + 10, self->y + 75, bar_width, 15, 0x3a86ff);
    
    // General scheduler stats
    extern struct process *proc_list;
    int proc_count = 0;
    struct process *p = proc_list;
    while (p) {{
        proc_count++;
        p = p->next;
    }}
    
    printk_sprintf(buf, "Active Tasks: %d", proc_count);
    fb_draw_string(self->x + 10, self->y + 105, buf, 0xFFFFFFFF, 0, 0);
    
    // Dynamic instruction notice
    fb_draw_string(self->x + 10, self->y + 135, "CPU State: Running (x86_64, SSE enabled)", 0x06d6a0, 0, 0);
}}

static void draw_text_editor(window_t *self) {{
    fb_draw_rect(self->x + 2, self->y + 24, self->w - 4, self->h - 26, 0xFFFFFFFF); // White page
    
    // Draw text buffer line by line
    int cur_line_y = self->y + 35;
    int cur_line_x = self->x + 10;
    
    char line_buf[128];
    int line_char_idx = 0;
    
    for (int i = 0; i < text_editor_len; i++) {{
        char c = text_editor_buf[i];
        if (c == '\n') {{
            line_buf[line_char_idx] = '\0';
            fb_draw_string(cur_line_x, cur_line_y, line_buf, 0, 0, 0);
            cur_line_y += 16;
            line_char_idx = 0;
        }} else {{
            line_buf[line_char_idx++] = c;
            if (line_char_idx >= 50) {{ // Wrap line
                line_buf[line_char_idx] = '\0';
                fb_draw_string(cur_line_x, cur_line_y, line_buf, 0, 0, 0);
                cur_line_y += 16;
                line_char_idx = 0;
            }}
        }}
    }}
    
    // Draw cursor in editor
    if (line_char_idx > 0) {{
        line_buf[line_char_idx] = '\0';
        fb_draw_string(cur_line_x, cur_line_y, line_buf, 0, 0, 0);
    }}
    fb_draw_rect(cur_line_x + line_char_idx * 8, cur_line_y, 2, 12, 0xFF000000); // Black cursor
}}

static void draw_calculator(window_t *self) {{
    // Grey calculator body
    fb_draw_rect(self->x + 2, self->y + 24, self->w - 4, self->h - 26, 0xd8f3dc);
    
    // Calculator Screen display
    fb_draw_rect(self->x + 10, self->y + 35, self->w - 20, 30, 0x1b263b);
    fb_draw_string(self->x + 20, self->y + 43, calc_display, 0x06d6a0, 0, 0);
    
    // Draw grid of buttons
    int btn_w = 40;
    int btn_h = 30;
    int gap = 10;
    
    const char *buttons[4][4] = {{
        {{"7", "8", "9", "/"}},
        {{"4", "5", "6", "*"}},
        {{"1", "2", "3", "-"}},
        {{"C", "0", "=", "+"}}
    }};
    
    for (int row = 0; row < 4; row++) {{
        for (int col = 0; col < 4; col++) {{
            int bx = self->x + 15 + col * (btn_w + gap);
            int by = self->y + 80 + row * (btn_h + gap);
            
            fb_draw_rect(bx, by, btn_w, btn_h, 0x52b788);
            fb_draw_rect_outline(bx, by, btn_w, btn_h, 0x2d6a4f);
            fb_draw_char(bx + 16, by + 10, buttons[row][col][0], 0xFFFFFFFF, 0, 0);
        }}
    }}
}}

static void draw_color_palette(window_t *self) {{
    fb_draw_rect(self->x + 2, self->y + 24, self->w - 4, self->h - 26, 0x151515);
    
    // Render color bars
    int bar_w = (self->w - 20) / 3;
    fb_draw_rect(self->x + 10, self->y + 35, bar_w, 40, 0xFF0000FF); // Red
    fb_draw_rect(self->x + 10 + bar_w, self->y + 35, bar_w, 40, 0x00FF00FF); // Green
    fb_draw_rect(self->x + 10 + 2*bar_w, self->y + 35, bar_w, 40, 0x0000FFFF); // Blue
    
    // Text labels
    fb_draw_string(self->x + 15, self->y + 47, "RED", 0xFFFFFFFF, 0, 0);
    fb_draw_string(self->x + 15 + bar_w, self->y + 47, "GREEN", 0xFFFFFFFF, 0, 0);
    fb_draw_string(self->x + 15 + 2*bar_w, self->y + 47, "BLUE", 0xFFFFFFFF, 0, 0);
    
    // Render color grid
    for (int y = 0; y < 80; y++) {{
        for (int x = 0; x < self->w - 20; x++) {{
            uint8_t r = (x * 255) / (self->w - 20);
            uint8_t g = (y * 255) / 80;
            uint8_t b = 128;
            uint32_t color = (r << 16) | (g << 8) | b;
            put_pixel_raw(self->x + 10 + x, self->y + 90 + y, color);
        }}
    }}
    
    fb_draw_string(self->x + 10, self->y + 180, "Alpha Blending Demonstrations", 0xFFFFFFFF, 0, 0);
}}

// Draw single window frame and content
static void draw_window(window_t *win) {{
    // Window header / title bar
    uint32_t title_color = win->active ? 0x3a86ff : 0x415a77;
    fb_draw_rect(win->x, win->y, win->w, 24, title_color);
    fb_draw_rect_outline(win->x, win->y, win->w, win->h, title_color);
    
    // Window Title Text
    fb_draw_string(win->x + 8, win->y + 6, win->title, 0xFFFFFFFF, 0, 0);
    
    // Window close button [X]
    fb_draw_rect(win->x + win->w - 20, win->y + 4, 16, 16, 0xff006e);
    fb_draw_char(win->x + win->w - 15, win->y + 8, 'X', 0xFFFFFFFF, 0, 0);
    
    // Execute content drawer
    if (win->draw_content) {{
        win->draw_content(win);
    }}
}}

// Full Desktop GUI redraw
static void redraw_gui(void) {{
    draw_desktop();
    
    // Draw windows in order (bottom to top)
    for (int i = num_windows - 1; i >= 0; i--) {{
        draw_window(&windows[i]);
    }}
    
    // Save background under the cursor and draw the cursor
    save_cursor(mouse_x, mouse_y);
    draw_cursor(mouse_x, mouse_y);
}}

// Window Manager logic
void gui_update_mouse(int dx, int dy, uint8_t buttons) {{
    restore_cursor();
    
    mouse_x += dx;
    mouse_y += dy;
    
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= screen_width) mouse_x = screen_width - 1;
    if (mouse_y >= screen_height) mouse_y = screen_height - 1;
    
    uint8_t old_buttons = mouse_buttons;
    mouse_buttons = buttons;
    
    int left_click = (mouse_buttons & 1) && !(old_buttons & 1);
    int left_release = !(mouse_buttons & 1) && (old_buttons & 1);
    
    if (left_click) {{
        // Check start menu click
        if (mouse_x >= 10 && mouse_x <= 90 && mouse_y >= screen_height - 35 && mouse_y <= screen_height - 5) {{
            calc_value = 0;
            calc_display[0] = '0';
            calc_display[1] = '\0';
            redraw_gui();
            return;
        }}
        
        // Find which window was clicked
        int clicked_win_idx = -1;
        for (int i = 0; i < num_windows; i++) {{
            window_t *win = &windows[i];
            if (mouse_x >= win->x && mouse_x <= win->x + win->w &&
                mouse_y >= win->y && mouse_y <= win->y + win->h) {{
                clicked_win_idx = i;
                break;
            }}
        }}
        
        if (clicked_win_idx != -1) {{
            window_t clicked_win = windows[clicked_win_idx];
            
            // Reorder to bring active window to front (index 0 is front/focused)
            for (int i = clicked_win_idx; i > 0; i--) {{
                windows[i] = windows[i - 1];
            }}
            windows[0] = clicked_win;
            
            // Set active states
            windows[0].active = 1;
            for (int i = 1; i < num_windows; i++) {{
                windows[i].active = 0;
            }}
            
            window_t *win = &windows[0];
            
            // Check close button click
            if (mouse_x >= win->x + win->w - 20 && mouse_x <= win->x + win->w - 4 &&
                mouse_y >= win->y + 4 && mouse_y <= win->y + 20) {{
                // Remove window
                for (int i = 0; i < num_windows - 1; i++) {{
                    windows[i] = windows[i + 1];
                }}
                num_windows--;
                redraw_gui();
                return;
            }}
            
            // Check title bar click (dragging)
            if (mouse_y >= win->y && mouse_y <= win->y + 24) {{
                win->dragging = 1;
                win->drag_offset_x = mouse_x - win->x;
                win->drag_offset_y = mouse_y - win->y;
            }}
            
            // Check calculator button click
            if (win->draw_content == draw_calculator) {{
                int cx = mouse_x - win->x;
                int cy = mouse_y - win->y;
                if (cx >= 15 && cx <= 215 && cy >= 80 && cy <= 240) {{
                    int btn_col = (cx - 15) / 50;
                    int btn_row = (cy - 80) / 40;
                    const char *buttons[4][4] = {{
                        {{"7", "8", "9", "/"}},
                        {{"4", "5", "6", "*"}},
                        {{"1", "2", "3", "-"}},
                        {{"C", "0", "=", "+"}}
                    }};
                    char b = buttons[btn_row][btn_col][0];
                    if (b >= '0' && b <= '9') {{
                        if (calc_display[0] == '0' && calc_display[1] == '\0') {{
                            calc_display[0] = b;
                        }} else {{
                            int l = 0;
                            while (calc_display[l]) l++;
                            if (l < 30) {{
                                calc_display[l] = b;
                                calc_display[l + 1] = '\0';
                            }}
                        }}
                    }} else if (b == 'C') {{
                        calc_display[0] = '0';
                        calc_display[1] = '\0';
                    }} else if (b == '=') {{
                        // A very basic parser stub to evaluate single operator expressions
                        // Find operator
                        int op_idx = -1;
                        char op = '\0';
                        for (int i = 0; calc_display[i]; i++) {{
                            if (calc_display[i] == '+' || calc_display[i] == '-' ||
                                calc_display[i] == '*' || calc_display[i] == '/') {{
                                op_idx = i;
                                op = calc_display[i];
                                break;
                            }}
                        }}
                        if (op_idx != -1) {{
                            int val1 = 0;
                            for (int i = 0; i < op_idx; i++) {{
                                val1 = val1 * 10 + (calc_display[i] - '0');
                            }}
                            int val2 = 0;
                            for (int i = op_idx + 1; calc_display[i]; i++) {{
                                val2 = val2 * 10 + (calc_display[i] - '0');
                            }}
                            int res = 0;
                            if (op == '+') res = val1 + val2;
                            else if (op == '-') res = val1 - val2;
                            else if (op == '*') res = val1 * val2;
                            else if (op == '/') res = val2 ? val1 / val2 : 0;
                            
                            printk_sprintf(calc_display, "%d", res);
                        }}
                    }} else {{
                        // Operator
                        int l = 0;
                        while (calc_display[l]) l++;
                        if (l < 30) {{
                            calc_display[l] = b;
                            calc_display[l + 1] = '\0';
                        }}
                    }}
                }}
            }}
        }}
    }}
    
    if (left_release) {{
        for (int i = 0; i < num_windows; i++) {{
            windows[i].dragging = 0;
        }}
    }}
    
    if (mouse_buttons & 1) {{
        // Handle dragging window
        for (int i = 0; i < num_windows; i++) {{
            if (windows[i].dragging) {{
                windows[i].x = mouse_x - windows[i].drag_offset_x;
                windows[i].y = mouse_y - windows[i].drag_offset_y;
                break; // only drag one window
            }}
        }}
    }}
    
    redraw_gui();
}}

// PS/2 Mouse streaming protocol Wait/Read/Write
void mouse_wait(uint8_t type) {{
    uint32_t timeout = 100000;
    if (type == 0) {{
        while (timeout--) {{
            if ((inb(0x64) & 1) == 1) return;
        }
    }} else {{
        while (timeout--) {{
            if ((inb(0x64) & 2) == 0) return;
        }
    }}
}}

void mouse_write(uint8_t write) {{
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}}

uint8_t mouse_read(void) {{
    mouse_wait(0);
    return inb(0x60);
}}

void mouse_init(void) {{
    uint8_t status;

    // Enable auxiliary mouse device
    mouse_wait(1);
    outb(0x64, 0xA8);

    // Enable interrupt for mouse (IRQ 12)
    mouse_wait(1);
    outb(0x64, 0x20); // read controller command byte
    mouse_wait(0);
    status = (inb(0x60) | 2); // set enable interrupt bit
    
    mouse_wait(1);
    outb(0x64, 0x60); // write controller command byte
    mouse_wait(1);
    outb(0x60, status);

    // Default settings
    mouse_write(0xF6);
    mouse_read(); // ACK

    // Stream mode packet streaming
    mouse_write(0xF4);
    mouse_read(); // ACK

    // Unmask IRQ 12
    pic_clear_mask(12);
}}

// Interrupt Handler for mouse
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

void mouse_handler(void) {{
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) return; // aux buffer empty

    uint8_t data = inb(0x60);
    
    switch (mouse_cycle) {{
        case 0:
            mouse_packet[0] = data;
            if (data & 8) {{
                mouse_cycle = 1;
            }}
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle = 2;
            break;
        case 2:
            mouse_packet[2] = data;
            mouse_cycle = 0;

            int x_offset = (int)mouse_packet[1];
            int y_offset = (int)mouse_packet[2];

            if (mouse_packet[0] & 0x10) x_offset |= ~0xFF;
            if (mouse_packet[0] & 0x20) y_offset |= ~0xFF;

            input_push(2, mouse_packet[0] & 7, x_offset, -y_offset);
            break;
    }}
}}

// Add new window helper
static void add_window(const char *title, int x, int y, int w, int h, void (*draw_content)(window_t *self)) {{
    if (num_windows >= MAX_WINDOWS) return;
    window_t *win = &windows[num_windows++];
    win->id = num_windows;
    for (int i = 0; i < 63; i++) {{
        win->title[i] = title[i];
        if (!title[i]) break;
    }}
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->active = (num_windows == 1);
    win->draw_content = draw_content;
    win->dragging = 0;
}}

// GUI Desktop main thread
void gui_start(void *arg) {{
    (void)arg;
    
    printk("GUI: Thread started, resolution %dx%d\n", screen_width, screen_height);
    
    // Add default desktop windows
    add_window("System Monitor", 50, 80, 320, 240, draw_system_monitor);
    add_window("Color Canvas Demo", 420, 80, 280, 240, draw_color_palette);
    add_window("Interactive Notepad", 100, 380, 420, 280, draw_text_editor);
    add_window("OS Calculator", 580, 380, 240, 280, draw_calculator);
    
    // Initialize graphics drawing
    redraw_gui();
    
    // Initialize PS/2 Mouse
    mouse_init();
    
    // Modify keyboard map for Arrow Key events mapping
    // We map Arrow Keys to move the cursor via keyboard directly!
    // Scan codes: Up=72, Left=75, Right=77, Down=80
    // We modify keyboard_handler mapping.
    
    while (1) {{
        // Yield to let scheduler run
        // Periodically redraw System Monitor window dynamically (for memory, CPU, clock)
        // Check keyboard input
        if (keyboard_has_char()) {{
            char c = keyboard_getchar();
            
            // Keyboard control mouse cursor fallback
            if (c == 17) {{ // Up
                gui_update_mouse(0, -10, mouse_buttons);
            }} else if (c == 18) {{ // Down
                gui_update_mouse(0, 10, mouse_buttons);
            }} else if (c == 19) {{ // Left
                gui_update_mouse(-10, 0, mouse_buttons);
            }} else if (c == 20) {{ // Right
                gui_update_mouse(10, 0, mouse_buttons);
            }} else if (c == '\n' || c == ' ') {{
                // Trigger mouse click at current location
                gui_update_mouse(0, 0, 1); // click
                msleep(100);
                gui_update_mouse(0, 0, 0); // release
            }} else if (num_windows > 0 && windows[0].active && windows[0].draw_content == draw_text_editor) {{
                // Send char to Notepad
                if (c == '\b') {{
                    if (text_editor_len > 0) {{
                        text_editor_len--;
                    }}
                }} else if (text_editor_len < 1000) {{
                    text_editor_buf[text_editor_len++] = c;
                }}
                redraw_gui();
            }}
        }}
        
        // Dynamically redraw system stats periodically
        redraw_gui();
        msleep(100);
    }}
}}
