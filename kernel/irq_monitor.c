#include "irq_monitor.h"
#include "kernel.h"
#include "spinlock.h"

/*
 * Simple IRQ monitor implementation.
 * Tracks interrupt counts for IRQ numbers 0..255.
 * Uses a spinlock to protect the counters in SMP environments.
 */

static uint64_t irq_counts[256];
static spinlock_t irq_monitor_lock;

void irq_monitor_init(void) {
    spinlock_init(&irq_monitor_lock);
    for (int i = 0; i < 256; i++) {
        irq_counts[i] = 0;
    }
    printk(KERN_INFO "IRQ Monitor initialized\n");
}

void irq_monitor_record(uint8_t irq) {
    spin_lock(&irq_monitor_lock);
    irq_counts[irq]++;
    spin_unlock(&irq_monitor_lock);
}

void irq_monitor_print_stats(void) {
    spin_lock(&irq_monitor_lock);
    for (int i = 0; i < 256; i++) {
        if (irq_counts[i]) {
            printk(KERN_INFO "IRQ %d count: %llu\n", i, irq_counts[i]);
        }
    }
    spin_unlock(&irq_monitor_lock);
}
