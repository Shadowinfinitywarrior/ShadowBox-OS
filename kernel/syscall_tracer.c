#include "kernel.h"
#include "syscall_tracer.h"

static int tracer_enabled = 1;

void syscall_tracer_log(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    if (!tracer_enabled) return;
    printk(KERN_DEBUG "TRACE: syscall %llu: args %llu %llu %llu %llu %llu\n",
           (unsigned long long)num,
           (unsigned long long)a1,
           (unsigned long long)a2,
           (unsigned long long)a3,
           (unsigned long long)a4,
           (unsigned long long)a5);
}

void syscall_tracer_init(void) {
    // Enable tracer (could be configurable later)
    tracer_enabled = 1;
    printk(KERN_INFO "Syscall tracer initialized.\n");
}
