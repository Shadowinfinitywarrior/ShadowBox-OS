#include "display.h"
#include "kernel.h"
#include "kstring.h"
#include "malloc.h"

static display_output_t display_outputs[16];
static int display_count = 0;
static int display_initialized = 0;

void display_subsystem_init(void) {
    if (display_initialized) return;
    display_initialized = 1;
    display_count = 0;
    printk(KERN_INFO "DISPLAY: Initialized display subsystem\n");
}

int display_register_output(display_output_t *disp) {
    if (!disp || display_count >= 16) return -1;

    display_outputs[display_count] = *disp;
    display_count++;

    display_outputs[display_count - 1].base_dev = NULL;
    display_outputs[display_count - 1].set_mode = NULL;
    display_outputs[display_count - 1].dpms_set = NULL;

    printk(KERN_INFO "DISPLAY: Registered output (type=%d, %ux%u)\n",
           disp->type, disp->current_width, disp->current_height);
    return 0;
}

void display_set_layout(display_output_t *disp, int32_t x, int32_t y) {
    if (!disp) return;
    disp->virtual_x = x;
    disp->virtual_y = y;
    printk(KERN_INFO "DISPLAY: Layout set to (%d, %d)\n", x, y);
}
