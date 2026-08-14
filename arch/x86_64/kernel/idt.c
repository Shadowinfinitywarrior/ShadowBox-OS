#include "idt.h"
#include "kernel.h"
#include "vmm.h"
#include "task.h"
#include "pic.h"
#include "apic.h"
#include "signal.h"

#define IDT_ENTRIES 256
#define MAX_IRQ_HANDLERS 16

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

static void (*irq_handlers[MAX_IRQ_HANDLERS])(void);

void irq_register_handler(uint8_t irq, void (*handler)(void)) {
    if (irq < MAX_IRQ_HANDLERS)
        irq_handlers[irq] = handler;
}

extern void* isr_stub_table[];

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].zero = 0;
}

void idt_init(void) {
    idtp.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idtp.base = (uint64_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }

    __asm__ volatile("lidt %0" : : "m"(idtp));
}

void idt_init_ap(void) {
    struct idt_ptr ap_idtp;
    ap_idtp.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    ap_idtp.base = (uint64_t)&idt;
    __asm__ volatile("lidt %0" : : "m"(ap_idtp));
}

void page_fault_handler(struct registers *regs) {
    uint64_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

    uint64_t error_code = regs->err_code;

    if (!(error_code & 1)) {
        if (vmm_handle_demand_page(fault_addr))
            return;
    }

    if (error_code & 2) {
        if (vmm_handle_cow_fault(fault_addr))
            return;
    }

    int present = error_code & 1;
    int write = error_code & 2;

    printk(KERN_ERR "PAGE FAULT at %llx, RIP=%llx, error=%llx, present=%d, write=%d\n",
           fault_addr, regs->rip, regs->err_code, present, write);
    printk(KERN_ERR "CS=%llx RFLAGS=%llx RSP=%llx SS=%llx\n",
           regs->cs, regs->rflags, regs->rsp, regs->ss);

    struct process *proc = get_current_process();
    printk(KERN_ERR "PF: cur_task=%p pid=%d state=%d kstack=%lx cr3=%lx\n", (void*)proc, proc ? proc->pid : -1, proc ? proc->state : -1, proc ? proc->kstack : 0, proc ? proc->cr3 : 0);
    {
        uint64_t *sp = (uint64_t*)regs->rsp;
        printk(KERN_ERR "PF: stack:");
        for (int i = 0; i < 16; i++) {
            if (!((uint64_t)sp & 0xFFFF000000000000ULL) && i == 0) break;
            if (i % 4 == 0) printk(KERN_ERR "\nPF:   ");
            printk(KERN_ERR " %lx", sp[i]);
        }
        printk(KERN_ERR "\n");
    }
    if (proc) {
        printk(KERN_ERR "PF: brk_start=%lx brk_end=%lx\n", proc->brk_start, proc->brk_end);
        printk(KERN_WARN "Killing process PID=%d (proc=%p)\n", proc->pid, (void*)proc);
        send_signal(proc->pid, 11); // SIGSEGV
        task_exit(-11);
    }
    panic("Unhandled page fault (no process)");
}

void isr_handler(struct registers *regs) {
    if (regs->int_no == 14) {
        page_fault_handler(regs);
        return;
    }

    if (regs->int_no < 32) {
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        uint64_t real_cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(real_cr3));
        printk(KERN_ERR "EXCEPTION: %llx, ERR_CODE: %llx\n", regs->int_no, regs->err_code);
        printk(KERN_ERR "RIP: %llx, CR2: %llx, RSP=%llx CS=%llx SS=%llx FLAGS=%llx\n",
               regs->rip, cr2, regs->rsp, regs->cs, regs->ss, regs->rflags);
        printk(KERN_ERR "RAX=%llx RBX=%llx RCX=%llx RDX=%llx\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
        printk(KERN_ERR "RSI=%llx RDI=%llx RBP=%llx R8=%llx R9=%llx R10=%llx R11=%llx\n",
               regs->rsi, regs->rdi, regs->rbp, regs->r8, regs->r9, regs->r10, regs->r11);
        printk(KERN_ERR "R12=%llx R13=%llx R14=%llx R15=%llx\n", regs->r12, regs->r13, regs->r14, regs->r15);
        printk(KERN_ERR "REAL_CR3=%llx\n", real_cr3);

        if ((regs->cs & 3) == 3) {
            printk(KERN_ERR "USR_STACK:");
            for (int i = 0; i < 16; i++) {
                if (i % 4 == 0) printk(KERN_ERR "\nUSR_STACK:   ");
                uint64_t v = regs->rsp + i * 8;
                uint64_t *pml4 = (uint64_t*)(real_cr3 + 0xFFFFFFFF80000000ULL);
                uint64_t *pdp, *pd, *pt;
                uint64_t pml4e = pml4[(v >> 39) & 0x1FF];
                if (!(pml4e & 1)) { printk(KERN_ERR " ..."); continue; }
                if (pml4e & (1 << 7)) { printk(KERN_ERR " !2M"); continue; }
                pdp = (uint64_t*)((pml4e & 0xFFFFFFFFFF000) + 0xFFFFFFFF80000000ULL);
                uint64_t pdpe = pdp[(v >> 30) & 0x1FF];
                if (!(pdpe & 1)) { printk(KERN_ERR " ..."); continue; }
                if (pdpe & (1 << 7)) { printk(KERN_ERR " !1G"); continue; }
                pd = (uint64_t*)((pdpe & 0xFFFFFFFFFF000) + 0xFFFFFFFF80000000ULL);
                uint64_t pde = pd[(v >> 21) & 0x1FF];
                if (!(pde & 1)) { printk(KERN_ERR " ..."); continue; }
                uint64_t phys;
                if (pde & (1 << 7))
                    phys = (pde & 0xFFFFFE00000) + (v & 0x1FFFFF);
                else {
                    pt = (uint64_t*)((pde & 0xFFFFFFFFFF000) + 0xFFFFFFFF80000000ULL);
                    uint64_t pte = pt[(v >> 12) & 0x1FF];
                    if (!(pte & 1)) { printk(KERN_ERR " ..."); continue; }
                    phys = (pte & 0xFFFFFFFFFF000) + (v & 0xFFF);
                }
                if (phys >= 0x8000000ULL) { printk(KERN_ERR " >128M"); continue; }
                printk(KERN_ERR " %lx", *(uint64_t*)(phys + 0xFFFFFFFF80000000ULL));
            }
            printk(KERN_ERR "\n");
        }

        struct process *proc = get_current_process();
        if (proc) {
            printk(KERN_ERR "current_proc=%p pid=%d state=%d cr3=%llx kstack=%llx\n",
                   (void*)proc, proc->pid, proc->state, (uint64_t)proc->cr3, (uint64_t)proc->kstack);
        }
        extern struct process *proc_list;
        struct process *pl = proc_list;
        if (pl) {
            int n = 0;
            do {
                printk(KERN_ERR "proc_list[%d]=%p pid=%d state=%d cr3=%llx\n",
                       n, (void*)pl, pl->pid, pl->state, (uint64_t)pl->cr3);
                pl = pl->next;
                n++;
            } while (pl != proc_list && n < 8);
        }
        extern uint64_t g_sched_runqueue_page;
        extern struct process **runqueue;
        extern size_t rq_size;
        printk(KERN_ERR "rq_size=%lu rq_page=%llx\n", rq_size, g_sched_runqueue_page);
        if (runqueue) {
            for (size_t i = 0; i < rq_size && i < 8; i++)
                printk(KERN_ERR "runqueue[%d]=%p\n", (int)i, (void*)runqueue[i]);
        }

        if (proc && proc->pid > 1 && proc->pid != 0x40309) {
            printk(KERN_WARN "Killing process PID=%d\n", proc->pid);
            proc->exit_status = -6;
            proc->state = TASK_ZOMBIE;
            while (1) yield();
        }
        panic("CPU Exception");
    } else if (regs->int_no >= 32 && regs->int_no < 48) {
        uint8_t irq = regs->int_no - 32;
        
        if (irq >= 8) {
            outb(0xA0, 0x20);
        }
        outb(0x20, 0x20);
        lapic_eoi();

        if (irq_handlers[irq]) {
            irq_handlers[irq]();
        }
        if (irq == 0) {
            extern void pit_handler(void);
            pit_handler();
        } else if (irq == 1) {
            extern void keyboard_handler(void);
            keyboard_handler();
        } else if (irq == 12) {
            extern void mouse_handler(void);
            mouse_handler();
        }
    }
}
