#include "malloc.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "spinlock.h"

#define HEAP_START 0xFFFFFFFF81000000

struct block_header {
    uint64_t size;
    struct block_header *next;
    uint32_t free;
    uint32_t magic;
};

#define MAGIC 0xDEADBEEF
#define ALIGN(x) (((x) + 7) & ~7)

static struct block_header *free_list = 0;
static uint64_t heap_end = HEAP_START;
static spinlock_t malloc_lock;

void malloc_init(void) {
    spinlock_init(&malloc_lock);

    // Initial 4KB page
    uint64_t phys = (uint64_t)pmm_alloc_page();
    if (!phys) panic("malloc_init: Out of memory");
    vmm_map_page(phys, heap_end, PAGE_PRESENT | PAGE_WRITE);

    free_list = (struct block_header *)heap_end;
    free_list->size = PAGE_SIZE - sizeof(struct block_header);
    free_list->free = 1;
    free_list->next = 0;
    free_list->magic = MAGIC;

    heap_end += PAGE_SIZE;
    printk(KERN_INFO "Malloc initialized.\n");
}

static void split_block(struct block_header *block, uint64_t size) {
    if (block->size > size + sizeof(struct block_header) + 8) {
        struct block_header *new_block = (struct block_header *)((uint8_t *)block + sizeof(struct block_header) + size);
        new_block->size = block->size - size - sizeof(struct block_header);
        new_block->free = 1;
        new_block->magic = MAGIC;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}

static void expand_heap(uint64_t size) {
    uint64_t pages = (size + sizeof(struct block_header) + PAGE_SIZE - 1) / PAGE_SIZE;
    struct block_header *last = free_list;
    while (last && last->next) {
        last = last->next;
    }

    uint64_t old_heap_end = heap_end;
    for (uint64_t i = 0; i < pages; i++) {
        uint64_t phys = (uint64_t)pmm_alloc_page();
        if (!phys) panic("kmalloc: Out of memory during expand");
        vmm_map_page(phys, heap_end, PAGE_PRESENT | PAGE_WRITE);
        heap_end += PAGE_SIZE;
    }

    struct block_header *new_block = (struct block_header *)old_heap_end;
    new_block->size = (pages * PAGE_SIZE) - sizeof(struct block_header);
    new_block->free = 1;
    new_block->magic = MAGIC;
    new_block->next = 0;

    if (last) {
        last->next = new_block;
    } else {
        free_list = new_block;
    }
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;
    
    size = ALIGN(size);
    
    spin_lock_irqsave(&malloc_lock);
    while (1) {
        struct block_header *curr = free_list;
        while (curr) {
            if (curr->free && curr->size >= size) {
                split_block(curr, size);
                curr->free = 0;
                spin_unlock_irqrestore(&malloc_lock);
                return (void *)((uint8_t *)curr + sizeof(struct block_header));
            }
            curr = curr->next;
        }

        expand_heap(size);
    }
}

void kfree(void *ptr) {
    if (!ptr) return;
    spin_lock_irqsave(&malloc_lock);
    struct block_header *block = (struct block_header *)((uint8_t *)ptr - sizeof(struct block_header));
    if (block->magic == MAGIC) {
        block->free = 1;
        // Merge contiguous free blocks
        struct block_header *curr = free_list;
        while (curr) {
            if (curr->free && curr->next && curr->next->free) {
                if ((uint8_t *)curr + sizeof(struct block_header) + curr->size == (uint8_t *)curr->next) {
                    curr->size += sizeof(struct block_header) + curr->next->size;
                    curr->next = curr->next->next;
                }
            }
            curr = curr->next;
        }
    }
    spin_unlock_irqrestore(&malloc_lock);
}
