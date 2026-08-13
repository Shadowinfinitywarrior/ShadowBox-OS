#ifndef SHADOWBOX_APIC_H
#define SHADOWBOX_APIC_H

#include "types.h"

#define LAPIC_DEFAULT_BASE  0xFEE00000
#define IOAPIC_DEFAULT_BASE 0xFEC00000

#define LAPIC_ID            0x0020
#define LAPIC_VERSION       0x0030
#define LAPIC_TPR           0x0080
#define LAPIC_EOI           0x00B0
#define LAPIC_SVR           0x00F0
#define LAPIC_ESR           0x0280
#define LAPIC_ICR_LOW       0x0300
#define LAPIC_ICR_HIGH      0x0310
#define LAPIC_LVT_TIMER     0x0320
#define LAPIC_LVT_LINT0     0x0350
#define LAPIC_LVT_LINT1     0x0360
#define LAPIC_LVT_ERROR     0x0370
#define LAPIC_TIMER_INITCNT 0x0380
#define LAPIC_TIMER_CURCNT  0x0390
#define LAPIC_TIMER_DIV     0x03E0

/*
 * apic_init - Initialize APIC subsystem
 * Enables local APIC and configures I/O APIC
 */
void apic_init(void);

/*
 * lapic_write - Write to local APIC register
 * @reg: Register offset
 * @val: Value to write
 */
void lapic_write(uint32_t reg, uint32_t val);

/*
 * lapic_read - Read from local APIC register
 * @reg: Register offset
 * Returns: Register value
 */
uint32_t lapic_read(uint32_t reg);

/*
 * lapic_eoi - Signal end of interrupt to local APIC
 */
void lapic_eoi(void);

/*
 * lapic_enable - Enable the local APIC
 */
void lapic_enable(void);

/*
 * ioapic_read - Read from I/O APIC register
 * @reg: Register offset
 * Returns: Register value
 */
uint32_t ioapic_read(uint32_t reg);

/*
 * ioapic_write - Write to I/O APIC register
 * @reg: Register offset
 * @val: Value to write
 */
void ioapic_write(uint32_t reg, uint32_t val);

/*
 * ioapic_set_entry - Set I/O APIC redirection entry
 * @index: Entry index
 * @data: 64-bit entry data
 */
void ioapic_set_entry(uint8_t index, uint64_t data);
void ioapic_route_irq(uint8_t irq, uint8_t vector, uint32_t lapic_id);

void apic_count_cpus(uint32_t *apic_ids, int max, int *count);

#endif
