#include "softirq.h"
#include "spinlock.h"
#include "malloc.h"

static softirq_action_t softirq_vec[MAX_SOFTIRQS];
static volatile uint32_t softirq_pending = 0;
static spinlock_t softirq_lock;

static tasklet_t *tasklet_list = NULL;
static spinlock_t tasklet_lock;

#define TASKLET_SOFTIRQ 0

static void tasklet_action(void *arg) {
    (void)arg;
    spin_lock_irqsave(&tasklet_lock);
    tasklet_t *list = tasklet_list;
    tasklet_list = NULL;
    spin_unlock_irqrestore(&tasklet_lock);

    while (list) {
        tasklet_t *t = list;
        list = list->next;
        t->state = 0; // mark as not scheduled so it can be re-scheduled
        if (t->func) {
            t->func(t->arg);
        }
    }
}

void softirq_init(void) {
    spinlock_init(&softirq_lock);
    spinlock_init(&tasklet_lock);
    
    for (int i = 0; i < MAX_SOFTIRQS; i++) {
        softirq_vec[i].handler = NULL;
    }
    
    // Register the tasklet softirq
    softirq_register(TASKLET_SOFTIRQ, tasklet_action, NULL);
}

void softirq_register(int nr, softirq_handler_t handler, void *arg) {
    if (nr < 0 || nr >= MAX_SOFTIRQS) return;
    softirq_vec[nr].handler = handler;
    softirq_vec[nr].arg = arg;
}

void softirq_raise(int nr) {
    if (nr < 0 || nr >= MAX_SOFTIRQS) return;
    spin_lock_irqsave(&softirq_lock);
    softirq_pending |= (1 << nr);
    spin_unlock_irqrestore(&softirq_lock);
}

void softirq_do_pending(void) {
    if (!softirq_pending) return;
    
    spin_lock_irqsave(&softirq_lock);
    uint32_t pending = softirq_pending;
    softirq_pending = 0;
    spin_unlock_irqrestore(&softirq_lock);
    
    for (int i = 0; i < MAX_SOFTIRQS; i++) {
        if (pending & (1 << i)) {
            if (softirq_vec[i].handler) {
                softirq_vec[i].handler(softirq_vec[i].arg);
            }
        }
    }
}

void tasklet_init(tasklet_t *t, void (*func)(void *), void *arg) {
    t->next = NULL;
    t->state = 0;
    t->func = func;
    t->arg = arg;
}

void tasklet_schedule(tasklet_t *t) {
    spin_lock_irqsave(&tasklet_lock);
    if (!t->state) {
        t->state = 1;
        t->next = tasklet_list;
        tasklet_list = t;
        softirq_raise(TASKLET_SOFTIRQ);
    }
    spin_unlock_irqrestore(&tasklet_lock);
}
