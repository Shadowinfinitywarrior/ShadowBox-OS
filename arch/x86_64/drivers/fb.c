#include "fb.h"
#include "font.h"
#include "kernel.h"
#include "vmm.h"

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t common_addr;
    uint32_t common_pitch;
    uint32_t common_width;
    uint32_t common_height;
    uint8_t common_bpp;
    uint8_t common_type;
    uint8_t reserved;
};

extern uint32_t multiboot_info_ptr;
static struct multiboot_tag_framebuffer *fb_tag = NULL;
static uint8_t *fb_addr = NULL;

void fb_set_info(uint64_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp) {
    fb_addr = NULL; // Defer mapping to fb_console_init
    // We need a fake fb_tag for the accessors; they will use the stored values
    // if fb_tag is set. We reuse a static struct.
    static struct multiboot_tag_framebuffer local_tag;
    local_tag.common_addr     = addr;
    local_tag.common_pitch    = pitch;
    local_tag.common_width    = width;
    local_tag.common_height   = height;
    local_tag.common_bpp      = bpp;
    fb_tag = &local_tag;
}

void fb_init(void) {
    printk("FB: Initializing Framebuffer / Graphics stub...\n");
    if (!multiboot_info_ptr) { printk("FB: no mbi ptr\n"); return; }
    
    uint64_t info_virt = (uint64_t)multiboot_info_ptr + 0xFFFFFFFF80000000ULL;
    uint64_t ptr = info_virt + 8;
    printk("FB: info_virt=%p mbi=%x\n", (void*)info_virt, multiboot_info_ptr);
    uint32_t total_size = *(uint32_t*)info_virt;
    printk("FB: total_size=%u\n", total_size);
    
    while (ptr < info_virt + total_size) {
        struct multiboot_tag_framebuffer *tag = (struct multiboot_tag_framebuffer*)ptr;
        printk("FB: tag type=%x size=%u\n", tag->type, tag->size);
        if (tag->type == 0) break;
        if (tag->type == 8) { // Framebuffer
            fb_tag = tag;
            fb_addr = NULL; // Defer mapping to fb_console_init
            printk("FB: Found Framebuffer %dx%dx%d\n", fb_tag->common_width, fb_tag->common_height, fb_tag->common_bpp);
            break;
        }
        ptr += (tag->size + 7) & ~7;
    }
}

void fb_putpixel(int x, int y, uint32_t color) {
    if (!fb_tag || !fb_addr) return;
    if (x < 0 || x >= (int)fb_tag->common_width || y < 0 || y >= (int)fb_tag->common_height) return;
    uint32_t offset = y * fb_tag->common_pitch + x * (fb_tag->common_bpp / 8);
    *(uint32_t*)(fb_addr + offset) = color;
}

uint8_t *fb_get_addr(void) {
    return fb_addr;
}

uint64_t fb_get_phys(void) {
    return fb_tag ? fb_tag->common_addr : 0;
}

uint32_t fb_get_width(void) {
    return fb_tag ? fb_tag->common_width : 0;
}

uint32_t fb_get_height(void) {
    return fb_tag ? fb_tag->common_height : 0;
}

uint32_t fb_get_pitch(void) {
    return fb_tag ? fb_tag->common_pitch : 0;
}

uint8_t fb_get_bpp(void) {
    return fb_tag ? fb_tag->common_bpp : 0;
}

void fb_get_info(uint32_t *width, uint32_t *height, uint32_t *pitch, uint8_t *bpp) {
    if (width)  *width  = fb_tag ? fb_tag->common_width  : 0;
    if (height) *height = fb_tag ? fb_tag->common_height : 0;
    if (pitch)  *pitch  = fb_tag ? fb_tag->common_pitch  : 0;
    if (bpp)    *bpp    = fb_tag ? fb_tag->common_bpp    : 0;
}

#define FONT_W 8
#define FONT_H 16
#define FB_CONSOLE_FG 0x00C0C0C0
#define FB_CONSOLE_BG 0x00000000

static int fbcon_col = 0;
static int fbcon_row = 0;
static int fbcon_initialized = 0;

// ANSI escape sequence parser state
enum { ANSI_NONE, ANSI_ESC, ANSI_BRACKET, ANSI_PARAM };
static int fbcon_ansi_state = ANSI_NONE;
static int fbcon_ansi_params[8];
static int fbcon_ansi_pcount = 0;
static int fbcon_ansi_val = 0;

static void fbcon_clear_screen(void) {
    if (!fb_tag || !fb_addr) return;
    uint32_t w = fb_get_width(), h = fb_get_height(), pitch = fb_get_pitch();
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++)
            *(uint32_t *)(fb_addr + y * pitch + x * 4) = FB_CONSOLE_BG;
    fbcon_col = 0;
    fbcon_row = 0;
}

static void fbcon_scroll(void) {
    uint32_t h = fb_get_height(), pitch = fb_get_pitch();
    int rows = h / FONT_H;
    int line_bytes = FONT_H * pitch;
    int total = (rows - 1) * line_bytes;

    for (int i = 0; i < total; i++)
        fb_addr[i] = fb_addr[i + line_bytes];
    for (int i = 0; i < line_bytes; i++)
        fb_addr[total + i] = 0;

    fbcon_row = rows - 1;
    fbcon_col = 0;
}

void fb_console_putchar(char c) {
    if (!fb_tag || !fb_addr || !fbcon_initialized) return;
    uint32_t w = fb_get_width();
    uint32_t pitch = fb_get_pitch();
    uint8_t bpp = fb_get_bpp();
    if (bpp != 32) return;

    // ANSI escape sequence state machine
    if (fbcon_ansi_state != ANSI_NONE) {
        if (fbcon_ansi_state == ANSI_ESC) {
            if (c == '[') {
                fbcon_ansi_state = ANSI_BRACKET;
                fbcon_ansi_pcount = 0;
                fbcon_ansi_val = 0;
            } else {
                fbcon_ansi_state = ANSI_NONE;
            }
            return;
        }
        if (fbcon_ansi_state == ANSI_BRACKET) {
            if (c >= '0' && c <= '9') {
                fbcon_ansi_val = fbcon_ansi_val * 10 + (c - '0');
                return;
            }
            fbcon_ansi_params[fbcon_ansi_pcount++] = fbcon_ansi_val;
            if (c == ';') {
                fbcon_ansi_val = 0;
                return;
            }
            // Command letter
            if (c == 'J') {
                if (fbcon_ansi_params[0] == 2) fbcon_clear_screen();
            } else if (c == 'H') {
                int r = fbcon_ansi_pcount > 0 ? fbcon_ansi_params[0] - 1 : 0;
                int cl = fbcon_ansi_pcount > 1 ? fbcon_ansi_params[1] - 1 : 0;
                if (r >= 0) fbcon_row = r;
                if (cl >= 0) fbcon_col = cl;
            } else if (c == 'm') {
                // Color codes — ignore for now
            } else if (c == 'A') {
                fbcon_row -= fbcon_ansi_params[0] > 0 ? fbcon_ansi_params[0] : 1;
                if (fbcon_row < 0) fbcon_row = 0;
            } else if (c == 'B') {
                fbcon_row += fbcon_ansi_params[0] > 0 ? fbcon_ansi_params[0] : 1;
            } else if (c == 'C') {
                fbcon_col += fbcon_ansi_params[0] > 0 ? fbcon_ansi_params[0] : 1;
            } else if (c == 'D') {
                fbcon_col -= fbcon_ansi_params[0] > 0 ? fbcon_ansi_params[0] : 1;
                if (fbcon_col < 0) fbcon_col = 0;
            }
            fbcon_ansi_state = ANSI_NONE;
            return;
        }
        fbcon_ansi_state = ANSI_NONE;
        return;
    }

    if (c == '\033') {
        fbcon_ansi_state = ANSI_ESC;
        return;
    }

    if (c == '\n') { fbcon_col = 0; fbcon_row++; }
    else if (c == '\r') { fbcon_col = 0; }
    else if (c == '\t') { fbcon_col = (fbcon_col + 4) & ~3; }
    else if (c == '\b') { if (fbcon_col > 0) fbcon_col--; }
    else if (c >= 0x20) {
        int x0 = fbcon_col * FONT_W;
        int y0 = fbcon_row * FONT_H;
        if (x0 + FONT_W > (int)w) { fbcon_col = 0; fbcon_row++; x0 = 0; y0 = fbcon_row * FONT_H; }

        for (int row = 0; row < FONT_H; row++) {
            uint8_t bits = font8x16[(unsigned char)c * 16 + row];
            for (int col = 0; col < FONT_W; col++) {
                uint32_t color = (bits & (0x80 >> col)) ? FB_CONSOLE_FG : FB_CONSOLE_BG;
                uint32_t offset = (y0 + row) * pitch + (x0 + col) * (bpp / 8);
                *(uint32_t *)(fb_addr + offset) = color;
            }
        }
        fbcon_col++;
        if (fbcon_col * FONT_W + FONT_W > (int)w) { fbcon_col = 0; fbcon_row++; }
    }
    int max_row = fb_get_height() / FONT_H;
    if (fbcon_row >= max_row) fbcon_scroll();
}

void fb_console_init(void) {
    if (!fb_tag || !fb_addr) return;
    uint32_t w = fb_get_width(), h = fb_get_height(), pitch = fb_get_pitch();
    uint8_t bpp = fb_get_bpp();
    uint64_t fb_phys = fb_tag->common_addr;
    uint32_t fb_size = h * pitch;

    uint64_t fb_kva = (uint64_t)vmap_phys(fb_phys, fb_size);

    fb_addr = (uint8_t *)fb_kva;

    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++)
            *(uint32_t *)(fb_addr + y * pitch + x * 4) = 0;

    fbcon_col = 0;
    fbcon_row = 0;
    fbcon_initialized = 1;
    printk("FBCON: %dx%dx%d, virt=%p, phys=%p\n", w, h, bpp, (void*)fb_addr, (void*)fb_phys);
}
