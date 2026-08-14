#include "sched.h"
#include "kernel.h"
#include "malloc.h"
#include "spinlock.h"
#include "kstring.h"
#include "task.h"
#include "smp.h"

struct process **runqueue = 0;
size_t rq_size = 0;
static size_t rq_capacity = 0;
static uint64_t min_vruntime = 0;
static spinlock_t sched_lock;
static uint64_t sched_granularity = 10000000;

uint64_t g_sched_runqueue_page = 0;

static const int weight_table[40] = {
    88761, 71713, 56483, 46273, 36291,
    29154, 23254, 18705, 14949, 11916,
    9548, 7620, 6100, 4904, 3906,
    3121, 2501, 1991, 1586, 1277,
    1024, 820, 655, 526, 423,
    335, 272, 215, 172, 137,
    110, 87, 70, 56, 45,
    36, 29, 23, 18, 15
};

static int nice_to_weight(int nice) {
    if (nice < -20) nice = -20;
    if (nice > 19) nice = 19;
    return weight_table[nice + 20];
}

void sched_init(void) {
    spinlock_init(&sched_lock);
    rq_capacity = 64;
    runqueue = kmalloc(rq_capacity * sizeof(struct process*));
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t v = (uint64_t)runqueue;
    uint64_t pml4e = pml4[(v >> 39) & 0x1FF];
    uint64_t *pdp = (uint64_t *)((pml4e & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t pdpe = pdp[(v >> 30) & 0x1FF];
    uint64_t *pd = (uint64_t *)((pdpe & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t pde = pd[(v >> 21) & 0x1FF];
    uint64_t *pt = (uint64_t *)((pde & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t pte = pt[(v >> 12) & 0x1FF];
    g_sched_runqueue_page = (pte & ~0xFFFULL);
    printk("SCHED: runqueue=%p phys_page=%p\n", (void*)runqueue, (void*)g_sched_runqueue_page);
    printk("SCHED: Enhanced SMP CFS scheduler initialized\n");
}

static void swap_nodes(size_t i, size_t j) {
    struct process *tmp = runqueue[i];
    runqueue[i] = runqueue[j];
    runqueue[j] = tmp;
}

static void heapify_up(size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (runqueue[idx]->vruntime >= runqueue[parent]->vruntime) break;
        swap_nodes(idx, parent);
        idx = parent;
    }
}

static void heapify_down(size_t idx) {
    while (1) {
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        size_t smallest = idx;
        if (left < rq_size && runqueue[left]->vruntime < runqueue[smallest]->vruntime)
            smallest = left;
        if (right < rq_size && runqueue[right]->vruntime < runqueue[smallest]->vruntime)
            smallest = right;
        if (smallest == idx) break;
        swap_nodes(idx, smallest);
        idx = smallest;
    }
}

static uint64_t vruntime_delta(struct process *p) {
    int weight = nice_to_weight(p->nice);
    return (sched_granularity * 1024) / weight;
}

void sched_enqueue(struct process *p) {
    if (!p) return;
    spin_lock_irqsave(&sched_lock);
    if (rq_size >= rq_capacity) {
        size_t new_cap = rq_capacity * 2;
        struct process **new_rq = kmalloc(new_cap * sizeof(struct process*));
        for (size_t i = 0; i < rq_size; i++) new_rq[i] = runqueue[i];
        kfree(runqueue);
        runqueue = new_rq;
        rq_capacity = new_cap;
    }

    for (size_t i = 0; i < rq_size; i++) {
        if (runqueue[i] == p) { spin_unlock_irqrestore(&sched_lock); return; }
    }

    if (p->vruntime == 0) p->vruntime = min_vruntime;
    if (p->vruntime < min_vruntime) p->vruntime = min_vruntime;

    p->sched_class = SCHED_CLASS_CFS;

    runqueue[rq_size] = p;
    heapify_up(rq_size);
    rq_size++;
    spin_unlock_irqrestore(&sched_lock);
}

struct process* sched_pick_next(void) {
    spin_lock_irqsave(&sched_lock);
    if (rq_size == 0) {
        spin_unlock_irqrestore(&sched_lock);
        return 0;
    }

    struct process *p = runqueue[0];
    int weight = nice_to_weight(p->nice);
    uint64_t vruntime_inc = (sched_granularity * 1024) / weight;
    p->vruntime += vruntime_inc;
    if (p->vruntime > min_vruntime) min_vruntime = p->vruntime;

    p->time_slice = (p->time_slice > 5) ? p->time_slice - 5 : 0;

    if (p->sched_policy == SCHED_FIFO) {
        if (p->time_slice > 0) return p;
    }

    runqueue[0] = runqueue[rq_size - 1];
    rq_size--;
    if (rq_size > 0) heapify_down(0);

    spin_unlock_irqrestore(&sched_lock);
    return p;
}

void sched_remove(struct process *p) {
    if (!p) return;
    spin_lock_irqsave(&sched_lock);
    for (size_t i = 0; i < rq_size; i++) {
        if (runqueue[i] == p) {
            runqueue[i] = runqueue[rq_size - 1];
            rq_size--;
            if (i < rq_size) {
                heapify_down(i);
                heapify_up(i);
            }
            break;
        }
    }
    spin_unlock_irqrestore(&sched_lock);
}

void sched_enqueue_on(struct process *p, int cpu_id) {
    (void)cpu_id;
    sched_enqueue(p);
}

int sched_find_idlest_cpu(void) {
    int idlest = 0;
    uint64_t min_load = 0;
    for (int i = 0; i < cpu_count; i++) {
        uint64_t load = 0;
        if (cpus[i].rq) load = cpus[i].rq->nr_running;
        if (i == 0 || load < min_load) {
            min_load = load;
            idlest = i;
        }
    }
    return idlest;
}

void sched_balance_runqueues(void) {
    int cur = current_cpu_id();
    for (int i = 0; i < cpu_count; i++) {
        if (i == cur || !cpus[i].rq) continue;
        if (cpus[i].rq->nr_running > cpus[cur].rq->nr_running + 2) {
            struct process *p = percpu_rq_steal(cpus[i].rq);
            if (p) {
                p->cpu_affinity = cur;
                sched_enqueue_on(p, cur);
            }
        }
    }
}

struct process *sched_steal_task(int dst_cpu) {
    for (int i = 0; i < cpu_count; i++) {
        if (i == dst_cpu || !cpus[i].rq) continue;
        if (cpus[i].rq->nr_running > 1) {
            return percpu_rq_steal(cpus[i].rq);
        }
    }
    return 0;
}
