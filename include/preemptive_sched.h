#ifndef SHADOWBOX_PREEMPTIVE_SCHED_H
#define SHADOWBOX_PREEMPTIVE_SCHED_H

// Minimal preemptive scheduler header. Provides initialization entry point.
// Actual implementation lives in kernel/preemptive_sched.c.

void preemptive_sched_init(void);

#endif // SHADOWBOX_PREEMPTIVE_SCHED_H
