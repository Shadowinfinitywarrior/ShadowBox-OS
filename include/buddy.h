#ifndef SHADOWBOX_BUDDY_H
#define SHADOWBOX_BUDDY_H

#include "types.h"

#define MAX_ORDER 16

struct buddy_page {
    struct buddy_page *next;
    unsigned int order;
    unsigned int free:1;
};

void buddy_init(uint64_t base_addr, uint64_t page_count);
void *buddy_alloc(unsigned int order);
void buddy_free(void *addr, unsigned int order);
uint64_t buddy_free_pages(void);
uint64_t buddy_total_pages(void);
uint64_t buddy_used_pages(void);

#endif
