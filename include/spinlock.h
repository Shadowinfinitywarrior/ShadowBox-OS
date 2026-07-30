#ifndef SHADOWBOX_SPINLOCK_H
#define SHADOWBOX_SPINLOCK_H

#include "types.h"
#include "compiler.h"

typedef struct spinlock {
    volatile uint32_t locked;
    uint64_t flags;
} spinlock_t;

static inline void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
    lock->flags = 0;
}

static inline void spin_lock(spinlock_t *lock) {
    uint32_t tmp = 1;
    __asm__ volatile(
        "1: xorl %%eax, %%eax\n"
        "   lock cmpxchgl %1, %0\n"
        "   je 3f\n"
        "2: pause\n"
        "   cmpl $0, %0\n"
        "   jne 2b\n"
        "   jmp 1b\n"
        "3:"
        : "+m"(lock->locked), "+r"(tmp) : : "eax", "memory", "cc"
    );
}

static inline void spin_unlock(spinlock_t *lock) {
    __asm__ volatile(
        "movl $0, %0"
        : "=m"(lock->locked) : : "memory"
    );
}

static inline void spin_lock_irqsave(spinlock_t *lock) {
    uint64_t rflags;
    __asm__ volatile ("pushfq; pop %0; cli" : "=rm" (rflags) : : "memory");
    spin_lock(lock);
    lock->flags = rflags;
}

static inline void spin_unlock_irqrestore(spinlock_t *lock) {
    uint64_t rflags = lock->flags;
    spin_unlock(lock);
    __asm__ volatile ("push %0; popfq" : : "rm" (rflags) : "memory", "cc");
}

#endif
