#include "task.h"
#include "malloc.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "elf.h"
#include "kstring.h"
#include "errno.h"
#include "fcntl.h"
#include "slab.h"
#include "sched.h"

extern void switch_to(struct context **old, struct context *new);

volatile int need_resched;

static struct process *current_proc = 0;
struct process *proc_list = 0;
static uint32_t next_pid = 1;

slab_cache_t *process_cache;

struct process *get_current_process(void) {
    return current_proc;
}

void task_init(void) {
    process_cache = slab_create_cache("process", sizeof(struct process), 8, 0);
    struct process *main_proc = slab_alloc(process_cache);
    if (!main_proc) panic("Failed to allocate main process");
    memset(main_proc, 0, sizeof(struct process));

    printk(KERN_INFO "main_proc=%p\n", (void*)main_proc);

    main_proc->pid = next_pid++;
    main_proc->ppid = 0;
    main_proc->state = TASK_RUNNING;
    main_proc->ctx = 0;

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    main_proc->cr3 = cr3;

    main_proc->kstack = 0;
    main_proc->vruntime = 0;
    main_proc->uid = 0;
    main_proc->gid = 0;
    main_proc->euid = 0;
    main_proc->egid = 0;

    main_proc->cwd = fs_root;

    main_proc->next = main_proc;

    current_proc = main_proc;
    proc_list = main_proc;
    /* main_proc is the kernel idle thread - not enqueued */
}

static void do_exit(int status) {
    current_proc->exit_status = status;
    current_proc->state = TASK_ZOMBIE;

    // Close all file descriptors
    for (int i = 0; i < MAX_FDS; i++) {
        if (current_proc->fds[i]) {
            process_fd_close(current_proc, i);
        }
    }

    // Reparent children to init (PID 1)
    struct process *p = proc_list;
    if (p) {
        do {
            if (p->ppid == current_proc->pid && p != current_proc) {
                p->ppid = 1;
                // Send SIGCHLD to new parent (init)
                p->sig_pending |= (1ULL << (SIGCHLD - 1));
            }
            p = p->next;
        } while (p != proc_list);
    }

    // Notify parent
    struct process *parent = proc_find(current_proc->ppid);
    if (parent) {
        parent->sig_pending |= (1ULL << (SIGCHLD - 1));
        if (parent->state == TASK_BLOCKED) {
            parent->state = TASK_READY;
            sched_enqueue(parent);
        }
    }

    while (1) yield();
}

void task_exit(int status) {
    do_exit(status);
}

extern void thread_start(void);

extern uint64_t syscall_get_user_stack(void);

struct process *task_create_proc(void (*entry)(void*), void *arg) {
    struct process *new_proc = slab_alloc(process_cache);
    if (!new_proc) return 0;
    memset(new_proc, 0, sizeof(struct process));

    new_proc->pid = next_pid++;
    new_proc->ppid = current_proc->pid;
    new_proc->state = TASK_READY;

    printk(KERN_INFO "task_create_proc: proc=%p\n", (void*)new_proc);

    new_proc->cr3 = vmm_create_address_space();
    if (pcid_supported) {
        new_proc->cr3 |= (new_proc->pid & 0xFFF);
    }
    new_proc->vruntime = 0;

    new_proc->kstack = (uint64_t)kmalloc(KERNEL_STACK_SIZE);
    printk(KERN_INFO "task_create_proc: kstack=%p for pid=%d\n", (void*)new_proc->kstack, new_proc->pid);

    uint64_t *stack = (uint64_t *)(new_proc->kstack + KERNEL_STACK_SIZE);

    *(--stack) = (uint64_t)arg;
    *(--stack) = (uint64_t)entry;
    *(--stack) = (uint64_t)thread_start;

    // IA32_GS_BASE and IA32_KERNEL_GS_BASE values (saved/restored by switch_to)
    // Push order matches switch_to save order: IA32_GS_BASE first, then IA32_KERNEL_GS_BASE
    uint64_t gs_base, kern_gs_base;
    __asm__ volatile("movl $0xC0000101, %%ecx; rdmsr; shl $32, %%rdx; or %%rax, %%rdx" : "=d"(gs_base) : : "eax", "ecx");
    __asm__ volatile("movl $0xC0000102, %%ecx; rdmsr; shl $32, %%rdx; or %%rax, %%rdx" : "=d"(kern_gs_base) : : "eax", "ecx");
    *(--stack) = gs_base;        // IA32_GS_BASE (first push, restored last)
    *(--stack) = kern_gs_base;   // IA32_KERNEL_GS_BASE (second push, restored first)

    *(--stack) = 0; // rbp
    *(--stack) = 0; // rbx
    *(--stack) = 0; // r12
    *(--stack) = 0; // r13
    *(--stack) = 0; // r14
    *(--stack) = 0; // r15

    new_proc->ctx = (struct context *)stack;

    new_proc->uid = current_proc->uid;
    new_proc->gid = current_proc->gid;
    new_proc->euid = current_proc->euid;
    new_proc->egid = current_proc->egid;
    new_proc->cwd = current_proc->cwd;

    new_proc->next = proc_list->next;
    proc_list->next = new_proc;

    sched_enqueue(new_proc);

    return new_proc;
}

struct process *task_fork(void) {
    struct process *child = slab_alloc(process_cache);
    memcpy(child, current_proc, sizeof(struct process));

    child->pid = next_pid++;
    child->ppid = current_proc->pid;
    child->state = TASK_READY;
    child->ctx = 0;
    child->exit_status = 0;
    child->sig_pending = 0;

    for (int i = 0; i < MAX_FDS; i++) {
        if (child->fds[i]) {
            child->fds[i]->refcount++;
        }
    }

    child->cwd = current_proc->cwd;

    child->cr3 = vmm_fork_address_space(current_proc->cr3 & ~0xFFF);
    if (pcid_supported) {
        child->cr3 |= (child->pid & 0xFFF);
    }

    // Build child's kernel stack from scratch so it returns from fork with value 0
    child->kstack = (uint64_t)kmalloc(KERNEL_STACK_SIZE);
    uint64_t *stack = (uint64_t *)(child->kstack + KERNEL_STACK_SIZE);

    // Read ALL user registers from the parent's syscall entry save area
    // New layout after saving callee-saved regs in syscall_entry:
    //   top-1: rbp, top-2: rbx, top-3: r12, top-4: r13, top-5: r14, top-6: r15
    //   top-7: rcx (user RIP), top-8: r11 (user RFLAGS)
    //   top-9: r10, top-10: r9, top-11: r8, top-12: rdi, top-13: rsi, top-14: rdx
    //   top-15: rax (syscall number)
    uint64_t *parent_stack_top = (uint64_t *)(current_proc->kstack + KERNEL_STACK_SIZE);
    uint64_t user_rbp  = *(parent_stack_top - 1);
    uint64_t user_rbx  = *(parent_stack_top - 2);
    uint64_t user_r12  = *(parent_stack_top - 3);
    uint64_t user_r13  = *(parent_stack_top - 4);
    uint64_t user_r14  = *(parent_stack_top - 5);
    uint64_t user_r15  = *(parent_stack_top - 6);
    uint64_t user_rip  = *(parent_stack_top - 7);   // rcx
    uint64_t user_rfl  = *(parent_stack_top - 8);   // r11
    uint64_t user_r10  = *(parent_stack_top - 9);
    uint64_t user_r9   = *(parent_stack_top - 10);
    uint64_t user_r8   = *(parent_stack_top - 11);
    uint64_t user_rdi  = *(parent_stack_top - 12);
    uint64_t user_rsi  = *(parent_stack_top - 13);
    uint64_t user_rdx  = *(parent_stack_top - 14);
    uint64_t user_rsp  = syscall_get_user_stack();
    child->user_rsp = user_rsp;

    // Push child stack from highest address downward so fork_child_entry
    // pops values from lowest to highest address (pop order = reverse push order).
    // user_rsp is pushed FIRST (highest addr) so it's popped LAST into RSP.
    // user_rbp is pushed LAST (lowest addr) so it's popped FIRST into RBP.

    // Pushed first (highest addr) — popped last:
    *(--stack) = user_rsp;      // restored by fork_child_entry's pop %rsp

    // Syscall regs (restored by fork_child_entry, in this order)
    *(--stack) = user_rdx;
    *(--stack) = user_rsi;
    *(--stack) = user_rdi;
    *(--stack) = user_r8;
    *(--stack) = user_r9;
    *(--stack) = user_r10;
    *(--stack) = user_rfl;      // r11 = RFLAGS
    *(--stack) = user_rip;      // rcx = RIP

    // Callee-saved regs (restored by fork_child_entry, in this order)
    *(--stack) = user_r15;
    *(--stack) = user_r14;
    *(--stack) = user_r13;
    *(--stack) = user_r12;
    *(--stack) = user_rbx;
    // Popped first (lowest addr):
    *(--stack) = user_rbp;

    // Return address for switch_to -> fork_child_entry
    extern void fork_child_entry(void);
    *(--stack) = (uint64_t)fork_child_entry;

    // IA32_GS_BASE and IA32_KERNEL_GS_BASE values (saved/restored by switch_to)
    uint64_t gs_base, kern_gs_base;
    __asm__ volatile("movl $0xC0000101, %%ecx; rdmsr; shl $32, %%rdx; or %%rax, %%rdx" : "=d"(gs_base) : : "eax", "ecx");
    __asm__ volatile("movl $0xC0000102, %%ecx; rdmsr; shl $32, %%rdx; or %%rax, %%rdx" : "=d"(kern_gs_base) : : "eax", "ecx");
    *(--stack) = gs_base;        // IA32_GS_BASE (first push, restored last)
    *(--stack) = kern_gs_base;   // IA32_KERNEL_GS_BASE (second push, restored first)

    // ctx for switch_to (push order matches switch_to: rbp,rbx,r12,r13,r14,r15)
    *(--stack) = user_rbp;
    *(--stack) = user_rbx;
    *(--stack) = user_r12;
    *(--stack) = user_r13;
    *(--stack) = user_r14;
    *(--stack) = user_r15;
    child->ctx = (struct context *)stack;

    child->next = proc_list->next;
    proc_list->next = child;
    sched_enqueue(child);

    return child;
}


int task_exec(struct process *proc, uint8_t *elf_data, uint64_t size, char **argv, char **envp) {
    (void)size;
    uint64_t entry = 0;

    int ret = elf_load_segments(proc, elf_data, &entry);
    if (ret < 0) return ret;

    uint64_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
    }

    uint64_t user_stack_virt = 0x8000000000;
    for (int i = 4; i >= 1; i--) {
        uint64_t phys = (uint64_t)pmm_alloc_page();
        vmm_map_page(phys, user_stack_virt - i * PAGE_SIZE, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    uint64_t *stack = (uint64_t *)user_stack_virt;

    int argc = 0;
    if (argv) while (argv[argc]) argc++;

    int envc = 0;
    if (envp) while (envp[envc]) envc++;

    uint64_t argstr_len = 0;
    for (int i = 0; i < argc; i++) argstr_len += strlen(argv[i]) + 1;
    for (int i = 0; i < envc; i++) argstr_len += strlen(envp[i]) + 1;

    char *argstr = (char*)stack - argstr_len;
    char *p = argstr;

    uint64_t *argv_ptrs = kmalloc((argc + 1) * sizeof(uint64_t));
    for (int i = 0; i < argc; i++) {
        argv_ptrs[i] = (uint64_t)p;
        for (const char *s = argv[i]; *s; s++) *p++ = *s;
        *p++ = 0;
    }
    argv_ptrs[argc] = 0;

    uint64_t *envp_ptrs = kmalloc((envc + 1) * sizeof(uint64_t));
    for (int i = 0; i < envc; i++) {
        envp_ptrs[i] = (uint64_t)p;
        for (const char *s = envp[i]; *s; s++) *p++ = *s;
        *p++ = 0;
    }
    envp_ptrs[envc] = 0;

    uint64_t auxv[] = { 0 };

    stack = (uint64_t*)((uint64_t)argstr & ~7);
    *(--stack) = (uint64_t)auxv + sizeof(auxv);
    *(--stack) = (uint64_t)envp_ptrs;
    *(--stack) = (uint64_t)argv_ptrs;
    *(--stack) = (uint64_t)argc;

    proc->brk_start = 0x60000000;
    proc->brk_end = 0x60000000;

    for (int i = 0; i < MAX_FDS; i++) {
        if (proc->fds[i] && (proc->fds[i]->flags & FD_CLOEXEC)) {
            process_fd_close(proc, i);
        }
    }

    uint64_t rsp = (uint64_t)stack;
    extern void switch_to_user_mode(uint64_t rip, uint64_t rsp);
    switch_to_user_mode(entry, rsp);
    return 0;
}

void schedule(void) {
    uint64_t flags;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags));

    if (!current_proc) {
        __asm__ volatile("pushq %0; popfq" :: "r"(flags));
        return;
    }

    struct process *next = sched_pick_next();
    
    // Ensure the chosen process is valid
    while (next && (next->state != TASK_READY && next->state != TASK_RUNNING)) {
        if (next->state == TASK_ZOMBIE) sched_remove(next);
        next = sched_pick_next();
    }
    
    if (!next || next == current_proc) {
        __asm__ volatile("pushq %0; popfq" :: "r"(flags));
        return;
    }

    struct process *old = current_proc;
    if (old && old->state == TASK_RUNNING && old->kstack != 0) {
        old->state = TASK_READY;
        sched_enqueue(old);
    }
    if (old) {
        old->user_rsp = syscall_get_user_stack();
    }
    next->state = TASK_RUNNING;
    current_proc = next;

    if (old && old->cr3 != next->cr3) {
        // We write next->cr3. PCID in lower 12 bits avoids TLB flush across PCIDs natively if CR4.PCIDE=1
        uint64_t cr3_val = next->cr3;
        // Do NOT set bit 63 (NO_FLUSH) on first switch as it can cause #GP. Just use standard switch.
        __asm__ volatile("mov %0, %%cr3" :: "r"(cr3_val) : "memory");
    }

    extern void tss_set_stack(uint64_t rsp0);
    tss_set_stack(next->kstack + KERNEL_STACK_SIZE);
    extern void syscall_set_kernel_stack(uint64_t stack);
    syscall_set_kernel_stack(next->kstack + KERNEL_STACK_SIZE);
    extern void syscall_set_user_stack(uint64_t stack);
    syscall_set_user_stack(next->user_rsp);

    switch_to(&old->ctx, next->ctx);
    __asm__ volatile("pushq %0; popfq" :: "r"(flags));
}

void yield(void) {
    schedule();
}

int process_fd_install(struct process *proc, struct file *file) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (!proc->fds[i]) {
            proc->fds[i] = file;
            if (file) file->refcount++;
            return i;
        }
    }
    return -EMFILE;
}

struct file *process_fd_get(struct process *proc, int fd) {
    if (fd < 0 || fd >= MAX_FDS) return 0;
    return proc->fds[fd];
}

void process_fd_close(struct process *proc, int fd) {
    if (fd < 0 || fd >= MAX_FDS) return;
    struct file *file = proc->fds[fd];
    if (!file) return;
    proc->fds[fd] = 0;
    file->refcount--;
    if (file->refcount == 0) {
        if (file->node) {
            if (file->node->flags == 5) { // FS_PIPE
                struct pipe { uint8_t buffer[4096]; uint32_t head; uint32_t tail; int readers; int writers; };
                struct pipe *p = (struct pipe*)file->node->impl;
                if (file->flags == 0) p->readers--; // SB_MODE_PULL
                if (file->flags == 1) p->writers--; // SB_MODE_PUSH
                if (p->readers <= 0 && p->writers <= 0) {
                    kfree(p);
                    kfree(file->node);
                }
            } else if (file->node->close) {
                file->node->close(file->node);
            }
        }
        kfree(file);
    }
}

void process_fd_fork(struct process *parent, struct process *child) {
    for (int i = 0; i < MAX_FDS; i++) {
        child->fds[i] = parent->fds[i];
        if (child->fds[i]) child->fds[i]->refcount++;
    }
}

struct process *proc_find(uint32_t pid) {
    struct process *p = proc_list;
    if (!p) return 0;
    do {
        if (p->pid == pid) return p;
        p = p->next;
    } while (p != proc_list);
    return 0;
}

int task_proc_info(struct proc_info *buf, int max) {
    struct process *p = proc_list;
    if (!p) return 0;
    int count = 0;
    do {
        if (count >= max) break;
        buf[count].pid = p->pid;
        buf[count].ppid = p->ppid;
        buf[count].state = p->state;
        buf[count].kstack = p->kstack;
        buf[count].cr3 = p->cr3;
        count++;
        p = p->next;
    } while (p != proc_list);
    return count;
}
