#include "aslr.h"
#include "kernel.h"
#include "kconfig.h"
#include "vmm.h"

static uint64_t kernel_offset = 0;
static uint64_t rng_state = 0;

static inline uint64_t rdtsc_val(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    __asm__ volatile("cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(leaf), "c"(subleaf));
}

static int cpu_has_smep(void) {
    if (!CONFIG_SMEP) return 0;
    uint32_t a, b, c, d;
    cpuid(7, 0, &a, &b, &c, &d);
    return (b >> 7) & 1;
}

static int cpu_has_smap(void) {
    if (!CONFIG_SMAP) return 0;
    uint32_t a, b, c, d;
    cpuid(7, 0, &a, &b, &c, &d);
    return (b >> 20) & 1;
}

void aslr_init(void) {
    rng_state = rdtsc_val() ^ (uint64_t)(uintptr_t)aslr_init;
    rng_state ^= (rng_state << 21) ^ (rng_state >> 35) ^ (rng_state << 4);
    kernel_offset = (xorshift64(&rng_state) % 64) << 21;
    printk(KERN_INFO "ASLR: initialized (kernel offset = %llx)\n", kernel_offset);
}

uint64_t aslr_random_addr(uint64_t base, uint64_t range, uint64_t align) {
    if (!CONFIG_ASLR) return base;
    return base + (xorshift64(&rng_state) % (range / align)) * align;
}

uint64_t aslr_get_mmap_base(void) {
    return aslr_random_addr(0x60000000, 0x20000000, 0x100000);
}

uint64_t aslr_get_stack_base(void) {
    return aslr_random_addr(0x70000000, 0x10000000, 0x1000);
}

uint64_t aslr_get_heap_base(void) {
    return aslr_random_addr(0x40000000, 0x20000000, 0x100000);
}

uint64_t aslr_get_kernel_base(void) {
    return 0xFFFFFFFF80000000 + kernel_offset;
}

uint64_t aslr_get_kernel_offset(void) {
    return kernel_offset;
}

void aslr_enable_smep(void) {
    if (cpu_has_smep()) {
        uint64_t cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= (1 << 20);
        __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
        printk(KERN_INFO "ASLR: SMEP enabled\n");
    } else {
        printk(KERN_INFO "ASLR: SMEP not supported by CPU\n");
    }
}

void aslr_enable_smap(void) {
    if (cpu_has_smap()) {
        uint64_t cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= (1 << 21);
        __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
        printk(KERN_INFO "ASLR: SMAP enabled\n");
    } else {
        printk(KERN_INFO "ASLR: SMAP not supported by CPU\n");
    }
}
