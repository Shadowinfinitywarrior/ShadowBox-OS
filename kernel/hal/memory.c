#include "hal/memory.h"
#include "kernel.h"
#include "pmm.h"
#include "vmm.h"
#include "multiboot2.h"

static bool memory_initialized = false;
static uint64_t total_phys = 0;
static uint64_t usable_phys = 0;

extern uint32_t multiboot_info_ptr;

hal_status_t memory_init(void) {
    if (memory_initialized) return HAL_SUCCESS;

    uint64_t total_pages = pmm_total_pages();
    uint64_t used_pages = 0;
    pmm_get_info(&total_pages, &used_pages);
    total_phys = total_pages * 4096;
    usable_phys = (total_pages - used_pages) * 4096;

    printk(KERN_INFO "MEM: %llu MB total, %llu MB usable\n",
           total_phys / (1024 * 1024), usable_phys / (1024 * 1024));

    memory_initialized = true;
    return HAL_SUCCESS;
}

int memory_get_map(memory_map_entry_t *map, int max_entries) {
    if (!map || !multiboot_info_ptr) return 0;

    int count = 0;
    uint32_t total_size = *(uint32_t *)(uint64_t)multiboot_info_ptr;
    uint32_t offs = multiboot_info_ptr + 8;

    while (offs < multiboot_info_ptr + total_size && count < max_entries) {
        struct multiboot_tag *tag = (struct multiboot_tag *)(uint64_t)offs;
        if (tag->type == 0) break;

        if (tag->type == 6) {
            struct {
                uint32_t type;
                uint32_t size;
                uint64_t entry_size;
                uint64_t entry_version;
            } *mmap_tag = (void *)tag;

            uint64_t addr = (uint64_t)(mmap_tag + 1);
            uint64_t end = (uint64_t)tag + tag->size;

            while (addr + sizeof(uint64_t) * 4 <= end && count < max_entries) {
                uint64_t base = *(uint64_t *)addr;
                uint64_t len = *(uint64_t *)(addr + 8);
                uint32_t type = *(uint32_t *)(addr + 16);
                uint32_t attrs = *(uint32_t *)(addr + 20);

                map[count].base_addr = base;
                map[count].length = len;
                map[count].type = type;
                map[count].extended_attrs = attrs;
                count++;

                addr += mmap_tag->entry_size;
            }
        }
        offs += (tag->size + 7) & ~7;
    }
    return count;
}

uint64_t memory_get_total_size(void) {
    return total_phys;
}

uint64_t memory_get_available_size(void) {
    return usable_phys;
}

hal_status_t memory_get_stats(memory_stats_t *stats) {
    if (!stats) return HAL_ERROR_IO;
    stats->total_physical = total_phys;
    stats->available_physical = usable_phys;
    return HAL_SUCCESS;
}

void* memory_alloc_physical(uint64_t size, uint64_t align) {
    (void)align;
    uint64_t pages = (size + 4095) / 4096;
    void *addr = NULL;
    for (uint64_t i = 0; i < pages; i++) {
        void *page = pmm_alloc_page();
        if (!page) return NULL;
        if (i == 0) addr = page;
    }
    return addr;
}

void memory_free_physical(void *addr, uint64_t size) {
    uint64_t pages = (size + 4095) / 4096;
    uint64_t base = (uint64_t)addr;
    for (uint64_t i = 0; i < pages; i++) {
        pmm_free_page((void *)(base + i * 4096));
    }
}

void* memory_map_physical(uint64_t phys, uint64_t size, uint32_t flags) {
    uint32_t page_flags = PAGE_PRESENT;
    if (flags & MEMORY_ATTR_WRITABLE) page_flags |= PAGE_WRITE;
    if (flags & MEMORY_ATTR_EXECUTABLE) page_flags |= PAGE_USER;

    uint64_t pages = (size + 4095) / 4096;
    uint64_t base = phys;
    for (uint64_t i = 0; i < pages; i++) {
        vmm_map_page(base + i * 4096, base + i * 4096 + 0xFFFFFFFF80000000, page_flags);
    }
    return (void *)(phys + 0xFFFFFFFF80000000);
}

void memory_unmap_physical(void *virt, uint64_t size) {
    uint64_t pages = (size + 4095) / 4096;
    uint64_t base = (uint64_t)virt;
    for (uint64_t i = 0; i < pages; i++) {
        vmm_unmap_page(base + i * 4096);
    }
}

hal_status_t memory_get_dram_info(dram_info_t *info) {
    if (!info) return HAL_ERROR_IO;
    info->total_size = total_phys;
    info->usable_size = usable_phys;
    info->type = DRAM_TYPE_DDR4;
    info->channels = 1;
    info->speed = 0;
    info->voltage = 0;
    info->ecc_support = 0;
    return HAL_SUCCESS;
}

void memory_copy(void *dest, const void *src, uint64_t size) {
    for (uint64_t i = 0; i < size; i++)
        ((uint8_t*)dest)[i] = ((const uint8_t*)src)[i];
}

void memory_set(void *dest, uint8_t value, uint64_t size) {
    for (uint64_t i = 0; i < size; i++)
        ((uint8_t*)dest)[i] = value;
}

void memory_zero(void *dest, uint64_t size) {
    memory_set(dest, 0, size);
}

int memory_compare(const void *a, const void *b, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        if (((const uint8_t*)a)[i] != ((const uint8_t*)b)[i])
            return ((const uint8_t*)a)[i] - ((const uint8_t*)b)[i];
    }
    return 0;
}

void memory_barrier(void) {
    __asm__ volatile("mfence" ::: "memory");
}

void memory_read_barrier(void) {
    __asm__ volatile("lfence" ::: "memory");
}

void memory_write_barrier(void) {
    __asm__ volatile("sfence" ::: "memory");
}

void memory_full_barrier(void) {
    __asm__ volatile("mfence" ::: "memory");
}
