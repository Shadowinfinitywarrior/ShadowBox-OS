#include "swap.h"
#include "kernel.h"

void swap_init(void) {
    printk(KERN_DEBUG "SWAP: Initializing page swapping/reclaiming subsystem...\n");
}

int swap_page_out(UNUSED uint64_t phys_addr) {
    return -1;
}

uint64_t swap_page_in(UNUSED uint64_t swap_offset) {
    return 0;
}
