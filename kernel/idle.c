// Per-CPU idle task implementation
// This file provides a simple idle loop for each CPU. The idle task runs
// when the scheduler has no runnable work and issues a halt instruction to
// reduce power consumption. It is created for each CPU during system boot.

#include "kernel.h"
#include "task.h"
#include "smp.h"
#include "sched.h"
#include "hal/cpu.h"

// Forward declaration for the halt function (implementation is in kernel/hal/cpu.c)
extern void cpu_halt(void);

static void idle_loop(void *arg)
{
    (void)arg;
    extern volatile int need_resched;
    // Infinite loop that halts the processor until the next interrupt.
    while (1) {
        cpu_halt();
        // Wake up: if the tick marked the scheduler, pick another task.
        // Without this the idle task would halt forever and no ready task
        // (e.g. the init process) would ever be scheduled again.
        if (need_resched) {
            extern void schedule(void);
            schedule();
        }
    }
}

// Initialise an idle task for every online CPU and enqueue it on the
// per‑CPU runqueue. The idle task is marked with the idle scheduling class
// and pinned to its CPU via the cpu_affinity field.
void idle_tasks_init(void)
{
    for (int i = 0; i < cpu_count; ++i) {
        struct process *idle = task_create_proc(idle_loop, NULL);
        if (!idle) {
            printk(KERN_ERR "Failed to create idle task for CPU %d\n", i);
            continue;
        }
        // Mark as idle scheduling class so the scheduler can treat it specially.
        idle->sched_class = SCHED_CLASS_IDLE;
        // Pin the idle thread to this CPU.
        idle->cpu_affinity = i;
        // Record in the cpu_info structure.
        cpus[i].idle_task = idle;
        // Enqueue on the per‑CPU runqueue.
        percpu_rq_enqueue(cpus[i].rq, idle);
    }
}
