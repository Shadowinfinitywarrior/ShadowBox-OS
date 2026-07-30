#include "pmm.h"
#include "multiboot2.h"
#include "kernel.h"
#include "spinlock.h"

extern uint8_t _kernel_phys_end;

static uint8_t *bitmap;
static uint64_t max_blocks = 0;
static uint64_t used_blocks = 0;
static spinlock_t pmm_lock;

static inline void bitmap_set(uint64_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint64_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int bitmap_test(uint64_t bit) {
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(uint32_t mb2_info_ptr) {
    spinlock_init(&pmm_lock);
    
    // Info ptr is a physical address, but since we mapped the first 2MB, 
    // it's accessible. For proper higher half, we should access it via
    // a physical offset if it's > 2MB, but Multiboot typically places it low.
    uint32_t total_size = *(uint32_t*)(uint64_t)mb2_info_ptr;
    uint32_t ptr = mb2_info_ptr + 8; // skip size and reserved
    
    struct multiboot_tag_mmap *mmap_tag = 0;

    while (ptr < mb2_info_ptr + total_size) {
        struct multiboot_tag *tag = (struct multiboot_tag*)(uint64_t)ptr;
        printk(KERN_DEBUG "Tag type: %x\n", tag->type);
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap_tag = (struct multiboot_tag_mmap *)tag;
        }
        // Round up to 8 byte alignment
        ptr += (tag->size + 7) & ~7;
    }

    if (!mmap_tag) {
        panic("No memory map from bootloader!");
    }

    // Find the max address to determine bitmap size
    uint64_t highest_addr = 0;
    uint32_t entries = (mmap_tag->size - sizeof(struct multiboot_tag_mmap)) / mmap_tag->entry_size;
    
    for (uint32_t i = 0; i < entries; i++) {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)((uint64_t)mmap_tag->entries + i * mmap_tag->entry_size);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (entry->addr + entry->len > highest_addr) {
                highest_addr = entry->addr + entry->len;
            }
        }
    }

    max_blocks = highest_addr / PAGE_SIZE;
    used_blocks = max_blocks; // Initially all used/reserved

    // Place the bitmap directly after the kernel
    uint64_t kernel_end = (uint64_t)&_kernel_phys_end;
    // We access it through the higher half mapping since we will unmap the lower half later
    bitmap = (uint8_t *)(kernel_end + 0xFFFFFFFF80000000); 

    // Calculate bitmap size
    uint64_t bitmap_size = max_blocks / 8;
    if (bitmap_size * 8 < max_blocks) bitmap_size++;

    // Since we write to the bitmap using its virtual address, we need to make sure
    // the virtual address is mapped. The kernel page table only maps up to 2MB right now.
    // If bitmap goes beyond 2MB, it will page fault!
    // For now, let's assume the kernel + bitmap fits in 2MB.
    
    // Set all blocks as used initially
    for (uint64_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    // Free the regions marked as AVAILABLE
    for (uint32_t i = 0; i < entries; i++) {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)((uint64_t)mmap_tag->entries + i * mmap_tag->entry_size);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint64_t addr = entry->addr;
            uint64_t len = entry->len;
            // Align start address to page boundary
            uint64_t start_block = (addr + PAGE_SIZE - 1) / PAGE_SIZE;
            uint64_t end_block = (addr + len) / PAGE_SIZE;
            
            for (uint64_t b = start_block; b < end_block; b++) {
                bitmap_clear(b);
                used_blocks--;
            }
        }
    }

    // Reserve block 0 (NULL) and everything up to kernel_end + bitmap_size
    uint64_t end_of_reserved = kernel_end + bitmap_size;
    uint64_t reserved_blocks = (end_of_reserved + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = 0; i < reserved_blocks; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_blocks++;
        }
    }

    // Also reserve Multiboot modules
    ptr = mb2_info_ptr + 8;
    while (ptr < mb2_info_ptr + total_size) {
        struct multiboot_tag *tag = (struct multiboot_tag*)(uint64_t)ptr;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;
        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            struct multiboot_tag_module *mod = (struct multiboot_tag_module *)tag;
            uint64_t start_block = mod->mod_start / PAGE_SIZE;
            uint64_t end_block = (mod->mod_end + PAGE_SIZE - 1) / PAGE_SIZE;
            for (uint64_t b = start_block; b < end_block; b++) {
                if (!bitmap_test(b)) {
                    bitmap_set(b);
                    used_blocks++;
                }
            }
        }
        ptr += (tag->size + 7) & ~7;
    }

    printk(KERN_INFO "PMM initialized: %x blocks total, %x blocks used\n", max_blocks, used_blocks);
}

void* pmm_alloc_page(void) {
    spin_lock_irqsave(&pmm_lock);
    for (uint64_t i = 0; i < max_blocks; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_blocks++;
            spin_unlock_irqrestore(&pmm_lock);
            return (void*)(i * PAGE_SIZE);
        }
    }
    spin_unlock_irqrestore(&pmm_lock);
    return 0; // Out of memory
}

void pmm_free_page(void* ptr) {
    uint64_t addr = (uint64_t)ptr;
    uint64_t block = addr / PAGE_SIZE;
    
    spin_lock_irqsave(&pmm_lock);
    if (bitmap_test(block)) {
        bitmap_clear(block);
        used_blocks--;
    }
    spin_unlock_irqrestore(&pmm_lock);
}

void pmm_get_info(uint64_t *total, uint64_t *used) {
    spin_lock_irqsave(&pmm_lock);
    if (total) *total = max_blocks;
    if (used) *used = used_blocks;
    spin_unlock_irqrestore(&pmm_lock);
}

uint64_t pmm_total_pages(void) {
    return max_blocks;
}

