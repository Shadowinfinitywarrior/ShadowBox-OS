#include "preemptive_sched.h"
#include "sched.h"
#include "smp.h"
#include "task.h"
#include "kernel.h"

/* Simple preemptive scheduler wrapper that uses per‑CPU runqueues.
 * This component demonstrates a minimal integration of the per‑CPU
 * run‑queue infrastructure defined in smp.c. It does not replace the
 * existing scheduler logic; instead it provides an entry point that can be
 * called during system initialization. The implementation is intentionally
 * lightweight and relies on the already‑implemented per‑CPU queue helpers.
 */

void preemptive_sched_init(void)
{
    /* Ensure that the BSP (CPU 0) has its per‑CPU runqueue initialized.
     * The smp_init() routine allocates the runqueue structures but only
     * APs call percpu_rq_init() in smp_ap_init(). We initialize the BSP here
     * to make the scheduler fully functional on a single‑core configuration.
     */
    if (cpu_count > 0) {
        percpu_rq_init(cpus[0].rq, 0);
        printk(KERN_INFO "Preemptive Scheduler: BSP runqueue initialized\n");
    }
}
