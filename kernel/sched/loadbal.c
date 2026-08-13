#include "sched.h"
#include "kernel.h"
#include "spinlock.h"

/*
 * Minimal stub implementation for load balancing.
 * The real load balancer would distribute tasks across CPUs.
 * This file provides placeholder functions so the kernel builds
 * without linking errors. No functional load‑balancing logic is
 * included.
 */

/* Initialize any load‑balancer state. Currently a no‑op. */
void loadbal_init(void) {
    /* No initialization required for the stub. */
}

/* Perform load‑balancing across runqueues. Currently a no‑op. */
void loadbal_balance(void) {
    /* Stub implementation – does nothing. */
}
