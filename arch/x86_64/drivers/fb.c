#include "fb.h"
#include "kernel.h"

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
            fb_addr = (uint8_t*)(fb_tag->common_addr + 0xFFFFFFFF80000000ULL);
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
