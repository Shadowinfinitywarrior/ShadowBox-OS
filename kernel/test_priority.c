#include "sched.h"
#include "task.h"
#include "kernel.h"

/* Simple test for the static priority scheduler. */
void test_priority_scheduler(void) {

    /* Create two dummy processes on the stack. */
    struct process p_high = {0};
    struct process p_low = {0};

    /* Initialize essential fields. */
    p_high.pid = 1001;
    p_high.static_prio = 10;   /* Higher priority */
    p_high.nice = 0;
    p_high.time_slice = 20;
    p_high.sched_policy = SCHED_NORMAL;

    p_low.pid = 1002;
    p_low.static_prio = 1;    /* Lower priority */
    p_low.nice = 0;
    p_low.time_slice = 20;
    p_low.sched_policy = SCHED_NORMAL;

    /* Enqueue both processes. */
    sched_enqueue(&p_high);
    sched_enqueue(&p_low);

    /* Retrieve the next scheduled process; should be the high‑priority one. */
    struct process *next = sched_pick_next();
    if (next && next->pid == p_high.pid) {
        printk(KERN_INFO "Priority scheduler test passed: picked pid %d\n", next->pid);
    } else {
        printk(KERN_ERR "Priority scheduler test FAILED: expected pid %d, got %d\n",
               p_high.pid, next ? next->pid : -1);
    }
}
