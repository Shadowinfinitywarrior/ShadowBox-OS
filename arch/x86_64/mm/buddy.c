#include "buddy.h"
#include "kernel.h"
#include "spinlock.h"
#include "kstring.h"

static struct buddy_page *free_areas[MAX_ORDER + 1];
static uint64_t total_page_count = 0;
static uint64_t free_page_count = 0;
static uint64_t used_page_count = 0;
static uint64_t buddy_base = 0;
static spinlock_t buddy_lock;

static inline uint64_t buddy_max_order_size(void) {
    return 1ULL << MAX_ORDER;
}

void buddy_init(uint64_t base_addr, uint64_t page_count) {
    spinlock_init(&buddy_lock);
    memset(free_areas, 0, sizeof(free_areas));
    buddy_base = base_addr;
    total_page_count = page_count;
    free_page_count = 0;
    used_page_count = 0;

    uint64_t remaining = page_count;
    uint64_t addr = base_addr;

    for (int order = MAX_ORDER; order >= 0; order--) {
        uint64_t block_size = 1ULL << order;
        while (remaining >= block_size) {
            struct buddy_page *page = (struct buddy_page *)(uint64_t)(addr * 4096 + 0xFFFFFFFF80000000);
            page->order = order;
            page->free = 1;
            page->next = free_areas[order];
            free_areas[order] = page;
            remaining -= block_size;
            addr += block_size;
            free_page_count += block_size;
        }
    }
    printk(KERN_INFO "BUDDY: initialized with %llu pages free\n", free_page_count);
}

static struct buddy_page *buddy_remove_from_list(int order) {
    struct buddy_page *page = free_areas[order];
    if (page) {
        free_areas[order] = page->next;
        page->next = 0;
        page->free = 0;
    }
    return page;
}

static void buddy_add_to_list(struct buddy_page *page, int order) {
    page->order = order;
    page->free = 1;
    page->next = free_areas[order];
    free_areas[order] = page;
}

void *buddy_alloc(unsigned int order) {
    if (order > MAX_ORDER) return 0;
    spin_lock_irqsave(&buddy_lock);
    int current_order = order;
    while (current_order <= MAX_ORDER && !free_areas[current_order]) {
        current_order++;
    }
    if (current_order > MAX_ORDER) {
        spin_unlock_irqrestore(&buddy_lock);
        return 0;
    }

    struct buddy_page *page = buddy_remove_from_list(current_order);
    while (current_order > order) {
        current_order--;
        uint64_t block_size = 1ULL << current_order;
        struct buddy_page *buddy = (struct buddy_page *)((uint8_t *)page + block_size * 4096);
        buddy_add_to_list(buddy, current_order);
    }
    free_page_count -= (1ULL << order);
    used_page_count += (1ULL << order);
    spin_unlock_irqrestore(&buddy_lock);
    return (void *)((uint64_t)page - 0xFFFFFFFF80000000);
}

void buddy_free(void *addr, unsigned int order) {
    if (!addr || order > MAX_ORDER) return;
    spin_lock_irqsave(&buddy_lock);
    uint64_t page_idx = ((uint64_t)addr) / 4096;
    struct buddy_page *page = (struct buddy_page *)((uint64_t)(page_idx * 4096) + 0xFFFFFFFF80000000);
    page->free = 0;

    uint64_t block_size = 1ULL << order;
    uint64_t base = page_idx;
    int current_order = order;

    while (current_order < MAX_ORDER) {
        uint64_t buddy_idx = base ^ (1ULL << current_order);
        struct buddy_page *buddy = (struct buddy_page *)(buddy_idx * 4096 + 0xFFFFFFFF80000000);
        if (!buddy->free || buddy->order != current_order) break;

        struct buddy_page **prev = &free_areas[current_order];
        while (*prev) {
            if (*prev == buddy) {
                *prev = buddy->next;
                break;
            }
            prev = &(*prev)->next;
        }
        buddy->free = 0;
        base = base & ~((1ULL << (current_order + 1)) - 1);
        current_order++;
    }

    struct buddy_page *merged = (struct buddy_page *)(base * 4096 + 0xFFFFFFFF80000000);
    buddy_add_to_list(merged, current_order);
    free_page_count += block_size;
    used_page_count -= block_size;
    spin_unlock_irqrestore(&buddy_lock);
}

uint64_t buddy_free_pages(void) { return free_page_count; }
uint64_t buddy_total_pages(void) { return total_page_count; }
uint64_t buddy_used_pages(void) { return used_page_count; }
