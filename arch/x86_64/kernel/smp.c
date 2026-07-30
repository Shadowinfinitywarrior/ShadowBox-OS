#include "smp.h"
#include "kernel.h"
#include "apic.h"
#include "acpi.h"
#include "pmm.h"
#include "vmm.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "kstring.h"
#include "malloc.h"
#include "sched.h"
#include "task.h"

struct cpu_info cpus[MAX_CPUS];
int cpu_count = 1;

extern void ap_trampoline_start(void);
extern void ap_trampoline_end(void);
extern uint64_t ap_trampoline_pml4;
extern uint64_t ap_trampoline_stack;
extern uint64_t ap_trampoline_entry;
extern uint64_t ap_trampoline_gdt;
extern uint64_t ap_trampoline_gdt_desc;

static spinlock_t smp_lock;
static volatile int ap_ready_count = 0;

static uint32_t apic_ids[MAX_CPUS];
static int apic_id_count = 1;

static inline uint32_t read_lapic_id(void) {
    return lapic_read(LAPIC_ID) >> 24;
}

int current_cpu_id(void) {
    uint32_t apic_id = read_lapic_id();
    for (int i = 0; i < cpu_count; i++) {
        if (cpus[i].apic_id == apic_id) return i;
    }
    return 0;
}

void smp_send_ipi(uint32_t apic_id, uint32_t icr_low) {
    while (lapic_read(LAPIC_ICR_LOW) & (1 << 12));

    lapic_write(LAPIC_ICR_HIGH, apic_id << 24);
    barrier();
    lapic_write(LAPIC_ICR_LOW, icr_low);

    while (lapic_read(LAPIC_ICR_LOW) & (1 << 12));
}

void smp_send_ipi_all(uint8_t vector) {
    for (int i = 1; i < cpu_count; i++) {
        smp_send_ipi(cpus[i].apic_id, vector);
    }
}

void smp_send_ipi_others(uint8_t vector) {
    int cur = current_cpu_id();
    for (int i = 0; i < cpu_count; i++) {
        if (i != cur) smp_send_ipi(cpus[i].apic_id, vector);
    }
}

void smp_tlb_shootdown(uint64_t addr, uint64_t size) {
    int cur = current_cpu_id();
    for (int i = 0; i < cpu_count; i++) {
        if (i == cur) continue;
        cpus[i].tlb_flush_needed = 1;
        cpus[i].tlb_flush_ack = 0;
        smp_send_ipi(cpus[i].apic_id, IPI_VEC_TLB_SHOOTDOWN);
    }

    for (int i = 0; i < cpu_count; i++) {
        if (i == cur) continue;
        volatile uint64_t timeout = 1000000;
        while (!cpus[i].tlb_flush_ack && timeout--) {
            __asm__ volatile("pause");
        }
        if (addr) {
            __asm__ volatile("invlpg (%0)" :: "r"(addr) : "memory");
        } else {
            uint64_t cr3;
            __asm__ volatile("mov %%cr3, %0; mov %0, %%cr3" : "=r"(cr3) :: "memory");
        }
    }
}

void smp_tlb_shootdown_all(void) {
    smp_tlb_shootdown(0, 0);
}

static void ap_trampoline_copy(void) {
    extern uint8_t _kernel_phys_end;
    uint64_t tramp_dest = 0x1000;

    uint8_t *src_start = (uint8_t *)&ap_trampoline_start;
    uint8_t *src_end = (uint8_t *)&ap_trampoline_end;
    uint32_t tramp_size = (uint32_t)(src_end - src_start);

    uint8_t *dest = (uint8_t *)(uint64_t)tramp_dest;
    for (uint32_t i = 0; i < tramp_size; i++) {
        dest[i] = src_start[i];
    }

    __asm__ volatile("wbinvd" ::: "memory");
}

void smp_init(void) {
    spinlock_init(&smp_lock);
    memset(cpus, 0, sizeof(cpus));

    uint32_t bsp_apic_id = read_lapic_id();
    cpus[0].cpu_id = 0;
    cpus[0].apic_id = bsp_apic_id;
    cpus[0].is_bsp = 1;
    cpu_count = 1;

    apic_ids[0] = bsp_apic_id;
    apic_id_count = 1;

    printk(KERN_INFO "SMP: BSP APIC ID = %u\n", bsp_apic_id);

    apic_count_cpus(apic_ids, MAX_CPUS, &apic_id_count);
    printk(KERN_INFO "SMP: Found %d APIC IDs total\n", apic_id_count);

    if (apic_id_count <= 1) {
        printk(KERN_INFO "SMP: Single-core system, skipping AP bringup\n");
        return;
    }

    ap_trampoline_copy();

    uint64_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));

    ap_trampoline_pml4 = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r"(ap_trampoline_pml4));

    struct gdt_ptr {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed));
    struct gdt_ptr gdt_desc;
    __asm__ volatile("sgdt %0" : "=m"(gdt_desc));
    ap_trampoline_gdt = gdt_desc.base;
    ap_trampoline_gdt_desc = *(uint64_t *)&gdt_desc;

    ap_trampoline_entry = (uint64_t)smp_ap_init;

    for (int i = 1; i < apic_id_count && cpu_count < MAX_CPUS; i++) {
        uint32_t apic_id = apic_ids[i];
        uint64_t ap_stack = (uint64_t)kmalloc(16384) + 16384;

        ap_trampoline_stack = ap_stack;

        cpus[cpu_count].cpu_id = cpu_count;
        cpus[cpu_count].apic_id = apic_id;
        cpus[cpu_count].is_bsp = 0;
        cpus[cpu_count].kernel_stack = ap_stack;

        printk(KERN_INFO "SMP: Starting AP %d (APIC ID %u)\n", cpu_count, apic_id);

        smp_send_ipi(apic_id, 0x28);

        uint64_t timeout = 100000;
        while (timeout--);

        smp_send_ipi(apic_id, 0x28 | 0x4600);

        timeout = 200;
        while (timeout--);

        for (int retry = 0; retry < 3; retry++) {
            smp_send_ipi(apic_id, 0x28 | 0x4600);
            timeout = 100000;
            int got_ack = 0;
            while (timeout--) {
                if (cpus[cpu_count].kernel_stack == (uint64_t)-1) {
                    got_ack = 1;
                    break;
                }
                __asm__ volatile("pause");
            }
            if (got_ack) break;
        }

        cpu_count++;
    }

    printk(KERN_INFO "SMP: %d CPU(s) online\n", cpu_count);
}

void smp_ap_init(void) {
    int cpu_id = cpu_count - 1;
    int cur_apic = read_lapic_id();

    for (int i = 0; i < cpu_count; i++) {
        if (cpus[i].apic_id == cur_apic) {
            cpu_id = i;
            break;
        }
    }

    cpus[cpu_id].kernel_stack = (uint64_t)-1;

    gdt_init_ap(cpu_id);
    idt_init_ap();

    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 7);
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));

    __asm__ volatile("sti");

    uint64_t *stack = (uint64_t *)(cpus[cpu_id].kernel_stack);
    stack -= 64;
    memset(stack, 0, 512);
    __asm__ volatile("mov %0, %%rsp" :: "r"(stack));

    percpu_rq_init(cpus[cpu_id].rq, cpu_id);

    printk(KERN_INFO "SMP: AP %d (APIC %u) online\n", cpu_id, cur_apic);

    while (1) {
        struct process *next = percpu_rq_dequeue(cpus[cpu_id].rq);
        if (next) {
            schedule();
        } else {
            __asm__ volatile("sti; hlt");
        }
    }
}

void smp_cpu_halt(void) {
    __asm__ volatile("cli; hlt");
}

void smp_cpu_stop_others(void) {
    smp_send_ipi_all(IPI_VEC_STOP);
}

void percpu_rq_init(struct per_cpu_runqueue *rq, int cpu_id) {
    spinlock_init(&rq->lock);
    rq->capacity = 64;
    rq->size = 0;
    rq->cpu_id = cpu_id;
    rq->min_vruntime = 0;
    rq->nr_running = 0;
    rq->nr_switches = 0;
    rq->heap = kmalloc(rq->capacity * sizeof(struct process *));
}

static void swap_ptrs(struct process **a, struct process **b) {
    struct process *t = *a;
    *a = *b;
    *b = t;
}

static void heapify_up(struct process **heap, size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (heap[idx]->vruntime >= heap[parent]->vruntime) break;
        swap_ptrs(&heap[idx], &heap[parent]);
        idx = parent;
    }
}

static void heapify_down(struct process **heap, size_t size, size_t idx) {
    while (1) {
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        size_t smallest = idx;
        if (left < size && heap[left]->vruntime < heap[smallest]->vruntime)
            smallest = left;
        if (right < size && heap[right]->vruntime < heap[smallest]->vruntime)
            smallest = right;
        if (smallest == idx) break;
        swap_ptrs(&heap[idx], &heap[smallest]);
        idx = smallest;
    }
}

void percpu_rq_enqueue(struct per_cpu_runqueue *rq, struct process *p) {
    spin_lock_irqsave(&rq->lock);
    if (rq->size >= rq->capacity) {
        size_t new_cap = rq->capacity * 2;
        struct process **new_heap = kmalloc(new_cap * sizeof(struct process *));
        for (size_t i = 0; i < rq->size; i++) new_heap[i] = rq->heap[i];
        kfree(rq->heap);
        rq->heap = new_heap;
        rq->capacity = new_cap;
    }

    for (size_t i = 0; i < rq->size; i++) {
        if (rq->heap[i] == p) {
            spin_unlock_irqrestore(&rq->lock);
            return;
        }
    }

    if (p->vruntime == 0) p->vruntime = rq->min_vruntime;
    if (p->vruntime < rq->min_vruntime) p->vruntime = rq->min_vruntime;

    rq->heap[rq->size] = p;
    heapify_up(rq->heap, rq->size);
    rq->size++;
    rq->nr_running++;
    spin_unlock_irqrestore(&rq->lock);
}

struct process *percpu_rq_dequeue(struct per_cpu_runqueue *rq) {
    spin_lock_irqsave(&rq->lock);
    if (rq->size == 0) {
        spin_unlock_irqrestore(&rq->lock);
        return 0;
    }

    struct process *p = rq->heap[0];
    rq->heap[0] = rq->heap[rq->size - 1];
    rq->size--;
    if (rq->size > 0) heapify_down(rq->heap, rq->size, 0);
    rq->nr_running--;
    rq->nr_switches++;
    spin_unlock_irqrestore(&rq->lock);
    return p;
}

void percpu_rq_remove(struct per_cpu_runqueue *rq, struct process *p) {
    spin_lock_irqsave(&rq->lock);
    for (size_t i = 0; i < rq->size; i++) {
        if (rq->heap[i] == p) {
            rq->heap[i] = rq->heap[rq->size - 1];
            rq->size--;
            if (i < rq->size) {
                heapify_down(rq->heap, rq->size, i);
                heapify_up(rq->heap, i);
            }
            rq->nr_running--;
            break;
        }
    }
    spin_unlock_irqrestore(&rq->lock);
}

struct process *percpu_rq_steal(struct per_cpu_runqueue *rq) {
    spin_lock_irqsave(&rq->lock);
    if (rq->size <= 1) {
        spin_unlock_irqrestore(&rq->lock);
        return 0;
    }
    struct process *p = rq->heap[rq->size - 1];
    rq->size--;
    rq->nr_running--;
    spin_unlock_irqrestore(&rq->lock);
    return p;
}
