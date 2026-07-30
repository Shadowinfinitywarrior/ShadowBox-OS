#include "kthread.h"
#include "kernel.h"
#include "task.h"
#include "malloc.h"
#include "kstring.h"

void kthread_init(void) {
    printk("KTHREAD: Initializing Kernel-Level Threads subsystem...\n");
}

struct process* kthread_create(void (*entry)(void*), void *arg, const char *name) {
    (void)name;
    return task_create_proc(entry, arg);
}
