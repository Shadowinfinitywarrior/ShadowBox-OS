#ifndef SHADOWBOX_PMM_H
#define SHADOWBOX_PMM_H

#include "types.h"

#define PAGE_SIZE 4096

/*
 * pmm_init - Initialize physical memory manager
 * @mb2_info_ptr: Multiboot2 info structure address
 */
void pmm_init(uint32_t mb2_info_ptr);

/*
 * pmm_alloc_page - Allocate a single physical page
 * Returns: Physical address of allocated page, or NULL
 */
void* pmm_alloc_page(void);

/*
 * pmm_free_page - Free a physical page
 * @ptr: Physical address of page to free
 */
void pmm_free_page(void* ptr);

/*
 * pmm_get_info - Get physical memory usage statistics
 * @total: Output for total pages
 * @used:  Output for used pages
 */
void pmm_get_info(uint64_t *total, uint64_t *used);

/*
 * pmm_total_pages - Get total number of physical pages
 * Returns: Total page count
 */
uint64_t pmm_total_pages(void);

#endif
