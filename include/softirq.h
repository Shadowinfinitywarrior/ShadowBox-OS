#ifndef SHADOWBOX_SOFTIRQ_H
#define SHADOWBOX_SOFTIRQ_H

#include "types.h"

#define MAX_SOFTIRQS 32

typedef void (*softirq_handler_t)(void *arg);

typedef struct softirq_action {
    softirq_handler_t handler;
    void *arg;
} softirq_action_t;

/* Initialize softirq subsystem */
void softirq_init(void);

/* Register a softirq handler */
void softirq_register(int nr, softirq_handler_t handler, void *arg);

/* Raise a softirq to be executed in the bottom half */
void softirq_raise(int nr);

/* Execute all pending softirqs (typically called on exit from ISR) */
void softirq_do_pending(void);

// Tasklet API (Bottom Halves built on top of softirq)
typedef struct tasklet {
    struct tasklet *next;
    uint32_t state;
    void (*func)(void *arg);
    void *arg;
} tasklet_t;

/* Initialize a tasklet structure */
void tasklet_init(tasklet_t *t, void (*func)(void *), void *arg);

/* Schedule a tasklet for execution in the bottom half */
void tasklet_schedule(tasklet_t *t);

#endif
