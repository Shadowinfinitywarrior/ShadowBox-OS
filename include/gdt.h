#ifndef SHADOWBOX_GDT_H
#define SHADOWBOX_GDT_H

#include "types.h"

/*
 * gdt_entry - Global Descriptor Table entry
 * @limit_low: Lower 16 bits of segment limit
 * @base_low:  Lower 16 bits of base address
 * @base_mid:  Bits 16-23 of base address
 * @access:    Access flags (present, ring, type)
 * @granularity: Flags and upper limit bits
 * @base_high: Bits 24-31 of base address
 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/*
 * gdt_ptr - 64-bit GDT pointer for lgdt instruction
 * @limit: Size of GDT minus 1
 * @base:  Linear address of GDT
 */
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/*
 * tss_entry - Task State Segment for ring switching
 * @reserved0: Reserved
 * @rsp0: Kernel stack pointer for ring 0
 * @rsp1-rsp2: Stack pointers for rings 1-2
 * @ist1-ist7: Interrupt stack table entries
 * @iopb_offset: I/O permission bitmap offset
 */
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));

/*
 * gdt_init - Initialize Global Descriptor Table
 */
void gdt_init(void);
void gdt_init_ap(int cpu_id);

/*
 * tss_set_stack - Set kernel stack pointer in TSS
 * @rsp0: Kernel stack pointer value
 */
void tss_set_stack(uint64_t rsp0);

#endif
