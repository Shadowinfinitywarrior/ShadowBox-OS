#ifndef SHADOWBOX_IRQ_MONITOR_H
#define SHADOWBOX_IRQ_MONITOR_H

#include "types.h"
#include "spinlock.h"

/*
 * IRQ Monitor component – tracks the number of times each IRQ line fires.
 * Provides initialization, record, and a simple stats printing function.
 */

void irq_monitor_init(void);
void irq_monitor_record(uint8_t irq);
void irq_monitor_print_stats(void);

#endif // SHADOWBOX_IRQ_MONITOR_H
