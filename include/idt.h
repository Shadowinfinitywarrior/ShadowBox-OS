#ifndef SHADOWBOX_IDT_H
#define SHADOWBOX_IDT_H

#include "types.h"

/*
 * idt_entry - Interrupt Descriptor Table entry (64-bit)
 * @offset_low:  Lower 16 bits of handler address
 * @selector:    Code segment selector
 * @ist:         Interrupt stack table index
 * @type_attr:   Gate type and attributes
 * @offset_mid:  Bits 16-31 of handler address
 * @offset_high: Bits 32-63 of handler address
 * @zero:        Reserved
 */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

/*
 * idt_ptr - 64-bit IDT pointer for lidt instruction
 * @limit: Size of IDT minus 1
 * @base:  Linear address of IDT
 */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/*
 * registers - Saved register state during interrupts
 * @r15-r8: General purpose registers
 * @rbp, rdi, rsi, rdx, rcx, rbx, rax: General purpose registers
 * @int_no:  Interrupt vector number
 * @err_code: Error code (if applicable)
 * @rip, cs, rflags, rsp, ss: CPU state at interrupt time
 */
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

/*
 * idt_init - Initialize Interrupt Descriptor Table
 */
void idt_init(void);
void idt_init_ap(void);
void irq_register_handler(uint8_t irq, void (*handler)(void));

#endif
