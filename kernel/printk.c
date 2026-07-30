#include "types.h"
#include "kernel.h"
#include "serial.h"
#include "fb.h"
#include "spinlock.h"

static spinlock_t printk_lock = {0, 0};
static int current_loglevel = 6;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
static uint16_t *vga_buffer = (uint16_t *)0xFFFFFFFF800B8000;
static int vga_col = 0;
static int vga_row = 0;

static void vga_scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
    for (int x = 0; x < VGA_WIDTH; x++)
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = 0x0F00 | ' ';
    vga_row = VGA_HEIGHT - 1;
    vga_col = 0;
}

static void vga_putchar(char c) {
    if (c == '\n') { vga_col = 0; vga_row++; }
    else if (c == '\r') { vga_col = 0; }
    else if (c == '\t') { vga_col = (vga_col + 8) & ~7; }
    else if (c == '\b') { if (vga_col > 0) vga_col--; }
    else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = 0x0F00 | (uint16_t)(unsigned char)c;
        vga_col++;
        if (vga_col >= VGA_WIDTH) { vga_col = 0; vga_row++; }
    }
    if (vga_row >= VGA_HEIGHT) vga_scroll();
}

static void put_char(char c) {
    serial_write_char(c);
    vga_putchar(c);
    fb_console_putchar(c);
}

void printk(const char *fmt, ...) {
    while (printk_lock.locked) { __builtin_ia32_pause(); }
    printk_lock.locked = 1;

    __builtin_va_list vl;
    __builtin_va_start(vl, fmt);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') { put_char(fmt[i]); continue; }
        i++;

        switch (fmt[i]) {
            case 's': {
                const char *s = __builtin_va_arg(vl, const char *);
                if (!s) s = "(null)";
                while (*s) { put_char(*s); s++; }
                break;
            }
            case 'd': case 'i': {
                int val = __builtin_va_arg(vl, int);
                char buf[12]; int idx = 0;
                if (val < 0) { put_char('-'); val = -val; }
                if (val == 0) { put_char('0'); break; }
                while (val > 0) { buf[idx++] = '0' + (val % 10); val /= 10; }
                while (idx > 0) { put_char(buf[--idx]); }
                break;
            }
            case 'x': {
                unsigned int val = __builtin_va_arg(vl, unsigned int);
                const char *hex = "0123456789abcdef";
                char buf[8]; int idx = 0;
                if (val == 0) { put_char('0'); break; }
                while (val > 0) { buf[idx++] = hex[val & 0xf]; val >>= 4; }
                while (idx > 0) { put_char(buf[--idx]); }
                break;
            }
            case 'p': {
                unsigned long long val = (unsigned long long)__builtin_va_arg(vl, void *);
                const char *hex = "0123456789abcdef";
                char buf[16]; int idx = 0;
                put_char('0'); put_char('x');
                if (val == 0) { put_char('0'); break; }
                while (val > 0) { buf[idx++] = hex[val & 0xf]; val >>= 4; }
                while (idx > 0) { put_char(buf[--idx]); }
                break;
            }
            case 'l': {
                int long_flag = 1;
                if (fmt[i+1] == 'l') { long_flag = 2; i++; }
                i++;
                if (fmt[i] == 'x' || fmt[i] == 'X') {
                    unsigned long long val = (long_flag == 2) ? __builtin_va_arg(vl, unsigned long long) : __builtin_va_arg(vl, unsigned long);
                    const char *hex = "0123456789abcdef";
                    char buf[16]; int idx = 0;
                    if (val == 0) { put_char('0'); break; }
                    while (val > 0) { buf[idx++] = hex[val & 0xf]; val >>= 4; }
                    while (idx > 0) { put_char(buf[--idx]); }
                } else if (fmt[i] == 'u') {
                    unsigned long long val = (long_flag == 2) ? __builtin_va_arg(vl, unsigned long long) : __builtin_va_arg(vl, unsigned long);
                    char buf[20]; int idx = 0;
                    if (val == 0) { put_char('0'); break; }
                    while (val > 0) { buf[idx++] = '0' + (val % 10); val /= 10; }
                    while (idx > 0) { put_char(buf[--idx]); }
                } else if (fmt[i] == 'd') {
                    long long val = (long_flag == 2) ? __builtin_va_arg(vl, long long) : __builtin_va_arg(vl, long);
                    char buf[20]; int idx = 0;
                    if (val < 0) { put_char('-'); val = -val; }
                    if (val == 0) { put_char('0'); break; }
                    while (val > 0) { buf[idx++] = '0' + (val % 10); val /= 10; }
                    while (idx > 0) { put_char(buf[--idx]); }
                }
                break;
            }
            case 'u': {
                unsigned int val = __builtin_va_arg(vl, unsigned int);
                char buf[12]; int idx = 0;
                if (val == 0) { put_char('0'); break; }
                while (val > 0) { buf[idx++] = '0' + (val % 10); val /= 10; }
                while (idx > 0) { put_char(buf[--idx]); }
                break;
            }
            case 'c': {
                char c = (char)__builtin_va_arg(vl, int);
                put_char(c);
                break;
            }
            case '%': {
                put_char('%');
                break;
            }
            default: {
                put_char('%'); put_char(fmt[i]);
                break;
            }
        }
    }
    __builtin_va_end(vl);

    printk_lock.locked = 0;
}

int printk_set_loglevel(int level) {
    int old = current_loglevel;
    if (level >= 0 && level <= 7) {
        current_loglevel = level;
    }
    return old;
}

void panic(const char *msg) {
    printk("\n\033[31m*** PANIC: %s ***\033[0m\n", msg);
    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}
