// Minimal stub for real‑time (RT) scheduler implementation.
// Provides placeholder definitions so the kernel builds without linking errors.
// No functional RT scheduling logic is included.

#include "sched.h"
#include "kernel.h"
#include "task.h"

// Stub initialization function – currently does nothing.
void rt_init(void) {
    // No operation.
}

// Stub enqueue function for RT tasks – currently a no‑op.
void rt_enqueue(struct process *p) {
    (void)p; // suppress unused parameter warning
    // No operation.
}

// Stub dequeue function for RT tasks – currently a no‑op.
void rt_dequeue(struct process *p) {
    (void)p;
    // No operation.
}
