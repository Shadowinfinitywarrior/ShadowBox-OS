#ifndef SHADOWBOX_SWAP_H
#define SHADOWBOX_SWAP_H

#include "types.h"

/*
 * swap_init - Initialize swap subsystem
 */
void swap_init(void);

/*
 * swap_page_out - Write a physical page to swap
 * @phys_addr: Physical address of page to swap out
 * Returns: Swap offset on success, -1 on error
 */
int swap_page_out(uint64_t phys_addr);

/*
 * swap_page_in - Read a physical page from swap
 * @swap_offset: Swap offset to read from
 * Returns: Physical address of the page
 */
uint64_t swap_page_in(uint64_t swap_offset);

#endif
