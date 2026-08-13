#include "tickless.h"

/* Weak fallback for hid_kbd_repeat_tick if not defined */
void hid_kbd_repeat_tick(void) __attribute__((weak));
void hid_kbd_repeat_tick(void) { /* no-op */ }
#include "apic.h"
#include "idt.h"
#include "kernel.h"

/*
 * tickless.c - Minimal tickless timer implementation.
 *
 * The traditional PIT generates periodic interrupts at a configurable
 * frequency (normally 100 Hz).  Tickless mode eliminates that periodic
 * timer and instead programs the local APIC timer in one‑shot mode.
 *
 * This implementation is deliberately simple: it registers a handler for
 * IRQ 0 (the same slot used by the PIT) and programs the APIC timer to
 * fire a one‑shot interrupt roughly every 10 ms.  Each interrupt increments
 * the kernel tick counter via `syscall_tick()` and marks the scheduler as
 * needing a reschedule (the same behaviour as `pit_handler`).  The handler
 * then re‑programs the timer for the next interval.
 *
 * The code does not attempt to dynamically calculate the next wake‑up
 * based on pending timers – it merely provides a functional placeholder
 * that compiles and integrates with the existing build system.
 */

/* Forward declaration of the kernel tick function defined in syscall.c */
extern void syscall_tick(void);
/* The global tick frequency (ticks per second). Defined in syscall.c */
extern uint64_t hz;
/* Scheduler flag indicating a reschedule is required. Defined in task.c */
extern volatile int need_resched;
extern void hid_kbd_repeat_tick(void);

/* Vector used for the APIC timer interrupt.  0x31 (49) is free in the IDT. */
#define TICKLESS_VECTOR 0x31

/* Desired timer interval in microseconds (10 ms). */
#define TICKLESS_INTERVAL_US 10000ULL

static void tickless_handler(void)
{
    /* Increment boot tick count. */
    syscall_tick();
    /* Mark that the scheduler should run. */
    need_resched = 1;
    hid_kbd_repeat_tick();

    /* Re‑program the APIC one‑shot timer for the next interval. */
    /* Calculate the initial‑count value based on the desired interval.
     * The APIC timer runs at the CPU bus frequency divided by the divisor.
     * For a placeholder we simply use the existing hz value (ticks per
     * second) to derive a count that would produce roughly the same period.
     */
    uint64_t count = (hz * TICKLESS_INTERVAL_US) / 1000000ULL;
    if (count == 0) {
        count = 1; /* Ensure the timer is armed. */
    }
    lapic_write(LAPIC_TIMER_INITCNT, (uint32_t)count);
}

void tickless_init(void)
{
    /* Register the handler in the IRQ table, re‑using slot 0. */
    irq_register_handler(0, tickless_handler);

    /* Configure the Local APIC timer:
     *   - Vector = TICKLESS_VECTOR
     *   - Mode = one‑shot (bits 17:16 = 0)
     *   - Unmasked (bit 16 = 0) – the handler will be invoked.
     */
    lapic_write(LAPIC_LVT_TIMER, TICKLESS_VECTOR);

    /* Use a divisor of 16 (value 0x3) – same as the legacy PIT setup.
     * This divisor is a reasonable default for the placeholder.
     */
    lapic_write(LAPIC_TIMER_DIV, 0x3);

    /* Arm the timer for the first tick. */
    uint64_t count = (hz * TICKLESS_INTERVAL_US) / 1000000ULL;
    if (count == 0) {
        count = 1;
    }
    lapic_write(LAPIC_TIMER_INITCNT, (uint32_t)count);
}
