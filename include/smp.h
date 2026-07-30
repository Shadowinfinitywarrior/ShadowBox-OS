#ifndef SHADOWBOX_SMP_H
#define SHADOWBOX_SMP_H

#include "types.h"
#include "kconfig.h"
#include "spinlock.h"
#include "sched.h"

#define MAX_CPUS CONFIG_MAX_CPUS

#define IPI_VEC_RESCHEDULE   0xF1
#define IPI_VEC_TLB_SHOOTDOWN 0xF2
#define IPI_VEC_PANIC        0xF3
#define IPI_VEC_STOP         0xF4

struct cpu_info {
    uint32_t cpu_id;
    uint32_t apic_id;
    uint32_t is_bsp;
    uint64_t kernel_stack;
    uint64_t user_stack;
    struct process *idle_task;
    struct per_cpu_runqueue *rq;
    uint64_t tlb_flush_ack;
    volatile uint32_t tlb_flush_needed;
    volatile uint32_t ipi_pending;
};

extern struct cpu_info cpus[MAX_CPUS];
extern int cpu_count;
extern int current_cpu_id(void);

static inline struct cpu_info *current_cpu(void) {
    return &cpus[current_cpu_id()];
}

void smp_init(void);
void smp_send_ipi(uint32_t apic_id, uint32_t icr_low);
void smp_send_ipi_all(uint8_t vector);
void smp_send_ipi_others(uint8_t vector);
void smp_tlb_shootdown(uint64_t addr, uint64_t size);
void smp_tlb_shootdown_all(void);
void smp_ap_boot(void);
void smp_ap_init(void);
void smp_cpu_halt(void);
void smp_cpu_stop_others(void);

/* Per-CPU runqueue for SMP scheduling */
struct per_cpu_runqueue {
    spinlock_t lock;
    struct process **heap;
    size_t size;
    size_t capacity;
    uint64_t min_vruntime;
    int cpu_id;
    uint64_t nr_running;
    uint64_t nr_switches;
};

void percpu_rq_init(struct per_cpu_runqueue *rq, int cpu_id);
void percpu_rq_enqueue(struct per_cpu_runqueue *rq, struct process *p);
struct process *percpu_rq_dequeue(struct per_cpu_runqueue *rq);
void percpu_rq_remove(struct per_cpu_runqueue *rq, struct process *p);
struct process *percpu_rq_steal(struct per_cpu_runqueue *rq);

#endif
