#ifndef SHADOWBOX_SCHED_H
#define SHADOWBOX_SCHED_H

#include "types.h"
#include "task.h"

void sched_init(void);
void sched_enqueue(struct process *p);
struct process* sched_pick_next(void);
void sched_remove(struct process *p);

void sched_enqueue_on(struct process *p, int cpu_id);
int sched_find_idlest_cpu(void);
void sched_balance_runqueues(void);
struct process *sched_steal_task(int dst_cpu);

#endif
