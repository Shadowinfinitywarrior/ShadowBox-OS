#ifndef SHADOWBOX_KTHREAD_H
#define SHADOWBOX_KTHREAD_H

#include "types.h"
#include "task.h"

/*
 * kthread_init - Initialize kernel thread subsystem
 */
void kthread_init(void);

/*
 * kthread_create - Create a new kernel thread
 * @entry: Thread entry function
 * @arg:   Argument passed to entry function
 * @name:  Thread name
 * Returns: Pointer to new process structure
 */
struct process* kthread_create(void (*entry)(void*), void *arg, const char *name);

#endif
