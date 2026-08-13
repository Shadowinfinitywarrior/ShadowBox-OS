#ifndef SHADOWBOX_FUTEX_H
#define SHADOWBOX_FUTEX_H

#include "types.h"
#include "task.h"

// Fast Userspace Mutex (Futex)
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_REQUEUE 3

typedef struct futex_waiter {
    thread_t *thread;
    struct futex_waiter *next;
} futex_waiter_t;

typedef struct futex_queue {
    uint32_t *uaddr;       // Userspace address being waited on
    futex_waiter_t *head;
    futex_waiter_t *tail;
    struct futex_queue *next;
} futex_queue_t;

// Syscall interfaces
int sys_futex(uint32_t *uaddr, int futex_op, uint32_t val, uint64_t timeout_ns, uint32_t *uaddr2, uint32_t val3);

#endif
