#include "syscall.h"
#include "kernel.h"
#include "vfs.h"
#include "tty.h"
#include "malloc.h"
#include "task.h"
#include "vmm.h"
#include "pmm.h"
#include "fcntl.h"
#include "stat.h"
#include "signal.h"
#include "wait.h"
#include "time.h"
#include "uname.h"
#include "kstring.h"
#include "elf.h"
#include "errno.h"
#include "slab.h"
#include "memory.h"
#include "sched.h"

// mprotect protection flags
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
#include "tmpfs.h"
#include "desktop.h"
#include "wifi.h"
#include "power.h"

#define MAX_PIPES 64

struct pipe {
    uint8_t buffer[4096];
    uint32_t head;
    uint32_t tail;
    uint32_t readers;
    uint32_t writers;
};

uint64_t hz = 100;
uint64_t boot_ticks = 0;

void syscall_tick(void) {
    boot_ticks++;
}

#define MSR_EFER 0xC0000080
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_SFMASK 0xC0000084
#define MSR_KERNEL_GS_BASE 0xC0000102
#define MSR_ARCH_PRCTL_FS 0xC0000100
#define MSR_ARCH_PRCTL_GS 0xC0000101

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t low = val & 0xFFFFFFFF;
    uint32_t high = val >> 32;
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

struct cpu_local {
    uint64_t kernel_stack;
    uint64_t user_stack;
} __attribute__((packed));

static struct cpu_local cpu0;

void syscall_set_kernel_stack(uint64_t stack) {
    cpu0.kernel_stack = stack;
}

uint64_t syscall_get_user_stack(void) {
    return cpu0.user_stack;
}

void syscall_set_user_stack(uint64_t stack) {
    cpu0.user_stack = stack;
}

extern void syscall_entry(void);

void syscall_init(void) {
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1);

    uint64_t star = ((uint64_t)0x13 << 48) | ((uint64_t)0x08 << 32);
    wrmsr(MSR_STAR, star);
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);
    wrmsr(MSR_SFMASK, 0x200);
    wrmsr(MSR_KERNEL_GS_BASE, (uint64_t)&cpu0);

    printk("ShadowBox syscall interface initialized.\n");
}

static inline int is_user_range(uint64_t addr, uint64_t size) {
    user_ptr_t uptr = { (void*)addr, size };
    return validate_user_ptr(uptr, 0) == 0;
}

static uint32_t pipe_read_func(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    struct pipe *p = (struct pipe*)node->impl;
    uint32_t read = 0;
    while (read < size) {
        if (p->head != p->tail) {
            buffer[read++] = p->buffer[p->tail++];
            p->tail %= 4096;
        } else {
            break;
        }
    }
    return read;
}

static uint32_t pipe_write_func(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    struct pipe *p = (struct pipe*)node->impl;
    uint32_t written = 0;
    while (written < size) {
        uint32_t next = (p->head + 1) % 4096;
        if (next != p->tail) {
            p->buffer[p->head] = buffer[written++];
            p->head = next;
        } else {
            if (p->readers <= 0) { 
                // Broken pipe
                break;
            }
            yield();
        }
    }
    return written;
}

extern uint64_t sys_sb_ipc_call(uint64_t target_pid, uint64_t msg_ptr, uint64_t u3, uint64_t u4, uint64_t u5);
extern uint64_t sys_sb_ipc_reply_wait(uint64_t reply_pid, uint64_t reply_msg_ptr, uint64_t req_msg_ptr, uint64_t u4, uint64_t u5);

static uint64_t sb_pull(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    if (!is_user_range(buf, count)) return -EFAULT;
    struct file *file = process_fd_get(get_current_process(), fd);
    if (!file) return -EBADF;
    uint32_t read = vfs_read(file->node, file->offset, count, (uint8_t*)buf);
    file->offset += read;
    return read;
}

static uint64_t sb_push(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    if (!is_user_range(buf, count)) return -EFAULT;
    struct file *file = process_fd_get(get_current_process(), fd);
     if (!file) return -EBADF;
    uint32_t written = vfs_write(file->node, file->offset, count, (uint8_t*)buf);
    file->offset += written;
    return written;
}

static int check_permissions(vfs_node_t *node, uint64_t flags, struct process *proc) {
    if (proc->uid == 0) return 1;
    int req_read = ((flags & 3) == SB_MODE_PULL) || ((flags & 3) == SB_MODE_PULLPUSH);
    int req_write = ((flags & 3) == SB_MODE_PUSH) || ((flags & 3) == SB_MODE_PULLPUSH) || (flags & SB_MODE_TRUNC);
    int perm;
    if (node->uid == proc->uid) {
        perm = (node->mask >> 6) & 7;
    } else if (node->gid == proc->gid) {
        perm = (node->mask >> 3) & 7;
    } else {
        perm = node->mask & 7;
    }
    if (req_read && !(perm & 4)) return 0;
    if (req_write && !(perm & 2)) return 0;
    return 1;
}

static uint64_t sb_acquire(uint64_t pathname, uint64_t flags, uint64_t mode, uint64_t unused1, uint64_t unused2) {
    (void)mode; (void)unused1; (void)unused2;
    if (!is_user_range(pathname, 1)) return -EFAULT;

    struct process *proc = get_current_process();
    char name[128];
    uint64_t i;
    for (i = 0; i < 127; i++) {
        name[i] = ((char*)pathname)[i];
        if (!name[i]) break;
    }
    name[127] = 0;

    char last_comp[128];
    vfs_node_t *dir = vfs_resolve_path(name, proc->cwd, last_comp);
    if (!dir) return -ENOENT;

    // If the path was a mount point (no last component), use the mount node directly
    vfs_node_t *node;
    if (last_comp[0] == 0) {
        node = dir;
    } else {
        node = vfs_finddir(dir, last_comp);
    }
    
    if (!node) {
        if (flags & SB_MODE_CREATE) {
            node = vfs_create(dir, last_comp, mode);
        }
        if (!node) return -ENOENT;
    }

    struct file *file = kmalloc(sizeof(struct file));
    file->node = node;
    file->offset = 0;
    if (flags & SB_MODE_TRUNC) file->offset = 0;
    file->flags = flags;
    file->refcount = 0;

    if (!check_permissions(node, flags, proc)) {
        kfree(file);
        return -EACCES;
    }


    if (node->open) node->open(node);

    int fd = process_fd_install(proc, file);
    if (fd < 0) {
        kfree(file);
        return fd;
    }
    return fd;
}

static uint64_t sb_release(uint64_t fd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    process_fd_close(get_current_process(), fd);
    return 0;
}

static uint64_t sys_stat(uint64_t pathname, uint64_t statbuf, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(pathname, 1) || !is_user_range(statbuf, sizeof(struct stat))) return -EFAULT;
    
    char name[128];
    uint64_t i;
    for (i = 0; i < 127; i++) {
        name[i] = ((char*)pathname)[i];
        if (!name[i]) break;
    }
    name[127] = 0;

    struct process *proc = get_current_process();
    char last_comp[128];
    vfs_node_t *dir = vfs_resolve_path(name, proc->cwd, last_comp);
    if (!dir) return -ENOENT;

    vfs_node_t *node;
    if (last_comp[0] == 0) {
        node = dir;
    } else {
        node = vfs_finddir(dir, last_comp);
    }
    if (!node) return -ENOENT;

    struct stat *st = (struct stat*)statbuf;
    for (uint64_t j = 0; j < sizeof(struct stat); j++) ((char*)st)[j] = 0;

    st->st_size = node->length;
    st->st_mode = node->mask & 07777;
    if (node->flags & FS_DIRECTORY) st->st_mode |= S_IFDIR;
    else if (node->flags & FS_CHARDEVICE) st->st_mode |= S_IFCHR;
    else st->st_mode |= S_IFREG;
    st->st_uid = node->uid;
    st->st_gid = node->gid;
    
    return 0;
}

static uint64_t sys_fstat(uint64_t fd, uint64_t statbuf, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(statbuf, sizeof(struct stat))) return -EFAULT;
    struct file *file = process_fd_get(get_current_process(), fd);
    if (!file) return -EBADF;

    struct stat *st = (struct stat*)statbuf;
    for (uint64_t i = 0; i < sizeof(struct stat); i++) ((char*)st)[i] = 0;

    st->st_size = file->node->length;
    st->st_mode = file->node->mask & 07777;
    if (file->node->flags & FS_DIRECTORY) st->st_mode |= S_IFDIR;
    else if (file->node->flags & FS_CHARDEVICE) st->st_mode |= S_IFCHR;
    else st->st_mode |= S_IFREG;
    st->st_uid = file->node->uid;
    st->st_gid = file->node->gid;
    return 0;
}

static uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    struct file *file = process_fd_get(get_current_process(), fd);
    if (!file) return -EBADF;

    switch (whence) {
        case 0: file->offset = offset; break;
        case 1: file->offset += offset; break;
        case 2: file->offset = file->node->length + offset; break;
        default: return -EINVAL;
    }
    return file->offset;
}

static uint64_t sys_mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t flags, uint64_t fd) {
    (void)flags; (void)fd;
    if (length == 0) return -EINVAL;

    uint64_t page_start = addr & ~(PAGE_SIZE - 1);
    uint64_t size = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (addr == 0) {
        addr = 0x70000000;
        page_start = addr;
    }

    struct process *proc = get_current_process();
    uint64_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
    }

    for (uint64_t offset = 0; offset < size; offset += PAGE_SIZE) {
        uint32_t page_flags = PAGE_USER | PAGE_DEMAND;
        if (prot & 2) page_flags |= PAGE_WRITE;
        vmm_map_page(0, page_start + offset, page_flags);
    }

    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
    }
    return page_start;
}

static uint64_t sys_mprotect(uint64_t addr, uint64_t len, uint64_t prot, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    if (addr & (PAGE_SIZE - 1)) return -EINVAL;
    if (len == 0) return -EINVAL;

    uint64_t size = (len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t set = PAGE_PRESENT | PAGE_USER;
    uint32_t clear = 0;

    if (prot & PROT_WRITE) set |= PAGE_WRITE;
    else clear |= PAGE_WRITE;

    struct process *proc = get_current_process();
    uint64_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
    }

    for (uint64_t offset = 0; offset < size; offset += PAGE_SIZE) {
        if (vmm_set_page_flags(addr + offset, set, clear) < 0) {
            if (proc->cr3 != old_cr3) {
                __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
            }
            return -ENOMEM;
        }
    }

    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
    }
    return 0;
}

static uint64_t sys_munmap(uint64_t addr, uint64_t length, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (addr & (PAGE_SIZE - 1)) return -EINVAL;
    if (length == 0) return -EINVAL;

    uint64_t size = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    struct process *proc = get_current_process();

    uint64_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
    }

    for (uint64_t offset = 0; offset < size; offset += PAGE_SIZE) {
        vmm_unmap_page(addr + offset);
    }

    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
    }
    
    return 0;
}

static uint64_t sys_brk(uint64_t brk, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    struct process *proc = get_current_process();

    // Round requested brk to page boundary
    uint64_t brk_page = (brk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (brk == 0) return proc->brk_end;

    // Handle shrinking or setting to smaller value
    if (brk_page <= proc->brk_end) {
        proc->brk_end = brk_page;
        return brk;
    }

    // Need to expand - map pages from current brk_end to brk_page
    // NOTE: We start from proc->brk_end (not brk_start) so retries continue
    // where we left off instead of remapping from the beginning.
    uint64_t alloc_start = proc->brk_end;
    uint64_t alloc_end = brk_page;

    if (alloc_end > alloc_start) {
        // Map one extra page beyond brk_page to serve as guard page and catch
        // any out-of-bounds writes from user code that go one byte past the
        // end of the heap.
        uint64_t map_limit = brk_page + PAGE_SIZE;

        printk(KERN_INFO "sys_brk: expanding from %lx to %lx (guard to %lx)\n",
               alloc_start, brk_page, map_limit);

        uint64_t old_cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
        if (proc->cr3 != old_cr3) {
            __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
        }

        for (uint64_t a = alloc_start; a < map_limit; a += PAGE_SIZE) {
            uint64_t phys = (uint64_t)pmm_alloc_page();
            if (!phys) {
                // Partial allocation: update brk_end to how far we got so
                // that the next sys_sbrk call continues from here, not from
                // a stale old brk_end value. Return the new brk_end so the
                // caller can proceed with whatever was actually allocated.
                printk(KERN_WARN "sys_brk: partially allocated %lu pages, %lu requested\n",
                       (a - alloc_start) / PAGE_SIZE, (map_limit - alloc_start) / PAGE_SIZE);
                uint64_t actual_end = a; // One page before the failed one
                proc->brk_end = actual_end;
                if (proc->cr3 != old_cr3) {
                    __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
                }
                return actual_end;
            }
            vmm_map_page(phys, a, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            // Use kernel identity mapping to zero the page
            void *zero_addr = (void *)(0xFFFFFFFF80000000ULL + phys);
            memset(zero_addr, 0, PAGE_SIZE);
        }

        if (proc->cr3 != old_cr3) {
            __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
        }
        proc->brk_end = brk_page;
    }
    return brk;
}

static uint64_t sys_rt_sigaction(uint64_t signum, uint64_t act, uint64_t oldact, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    if (signum == 0 || signum >= 32 || signum == SIGKILL || signum == SIGSTOP) return -EINVAL;
    struct process *proc = get_current_process();

    if (oldact && is_user_range(oldact, sizeof(struct sigaction))) {
        memcpy((void*)oldact, &proc->sig_actions[signum - 1], sizeof(struct sigaction));
    }
    if (act && is_user_range(act, sizeof(struct sigaction))) {
        memcpy(&proc->sig_actions[signum - 1], (void*)act, sizeof(struct sigaction));
    }
    return 0;
}

static uint64_t sys_rt_sigprocmask(uint64_t how, uint64_t set, uint64_t oldset, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    struct process *proc = get_current_process();
    if (oldset && is_user_range(oldset, 8)) {
        *(uint64_t*)oldset = proc->sig_blocked;
    }
    if (set && is_user_range(set, 8)) {
        uint64_t mask = *(uint64_t*)set;
        if (how == 0) proc->sig_blocked = mask;
        else if (how == 1) proc->sig_blocked |= mask;
        else if (how == 2) proc->sig_blocked &= ~mask;
    }
    return 0;
}

static uint64_t sys_rt_sigreturn(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return 0;
}

static uint64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t arg, uint64_t unused1, uint64_t unused2) {
    (void)request; (void)arg; (void)unused1; (void)unused2;
    struct file *file = process_fd_get(get_current_process(), fd);
    if (!file) return -EBADF;
    return -ENOTTY;
}

static uint64_t sys_pipe(uint64_t pipefd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(pipefd, 2 * sizeof(int))) return -EFAULT;

    struct pipe *p = kmalloc(sizeof(struct pipe));
    for (int i = 0; i < sizeof(struct pipe); i++) ((char*)p)[i] = 0;
    p->readers = 1;
    p->writers = 1;

    vfs_node_t *pipe_node = kmalloc(sizeof(vfs_node_t));
    for (uint64_t i = 0; i < sizeof(vfs_node_t); i++) ((char*)pipe_node)[i] = 0;
    pipe_node->flags = FS_PIPE;
    pipe_node->impl = (uint64_t)p;
    pipe_node->read = pipe_read_func;
    pipe_node->write = pipe_write_func;

    struct file *rfile = kmalloc(sizeof(struct file));
    rfile->node = pipe_node;
    rfile->offset = 0;
    rfile->flags = SB_MODE_PULL;
    rfile->refcount = 0;

    struct file *wfile = kmalloc(sizeof(struct file));
    wfile->node = pipe_node;
    wfile->offset = 0;
    wfile->flags = SB_MODE_PUSH;
    wfile->refcount = 0;

    struct process *proc = get_current_process();
    int rfd = process_fd_install(proc, rfile);
    int wfd = process_fd_install(proc, wfile);

    int *fds = (int*)pipefd;
    fds[0] = rfd;
    fds[1] = wfd;
    return 0;
}

static uint64_t sys_sched_yield(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    yield();
    return 0;
}

static void pipe_count_inc(struct file *file) {
    if (file->node && file->node->flags == FS_PIPE) {
        struct pipe *p = (struct pipe*)file->node->impl;
        if (file->flags == SB_MODE_PULL) p->readers++;
        else if (file->flags == SB_MODE_PUSH) p->writers++;
    }
}

static uint64_t sys_dup(uint64_t oldfd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    struct process *proc = get_current_process();
    struct file *file = process_fd_get(proc, oldfd);
    if (!file) return -EBADF;
    pipe_count_inc(file);
    return process_fd_install(proc, file);
}

static uint64_t sys_dup2(uint64_t oldfd, uint64_t newfd, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    struct process *proc = get_current_process();
    struct file *file = process_fd_get(proc, oldfd);
    if (!file) return -EBADF;
    if (newfd < 0 || newfd >= MAX_FDS) return -EBADF;
    if (proc->fds[newfd]) process_fd_close(proc, newfd);
    proc->fds[newfd] = file;
    file->refcount++;
    pipe_count_inc(file);
    return newfd;
}

static uint64_t sys_nanosleep(uint64_t req, uint64_t rem, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)rem; (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(req, sizeof(struct timespec))) return -EFAULT;
    struct timespec *ts = (struct timespec*)req;
    uint64_t target = boot_ticks + (ts->tv_sec * hz) + (ts->tv_nsec * hz / 1000000000);
    while (boot_ticks < target) yield();
    return 0;
}

static uint64_t sys_getpid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return get_current_process()->pid;
}

static uint64_t sb_replicate(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    struct process *child = task_fork();
    if (!child) return -EAGAIN;

    uint64_t child_pid = child->pid;

    if (get_current_process() == child) return 0;
    return child_pid;
}

static uint64_t sb_morph(uint64_t filename, uint64_t argv, uint64_t envp, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    if (!is_user_range(filename, 1)) return -EFAULT;

    char kname[128];
    uint64_t i;
    for (i = 0; i < 127; i++) {
        kname[i] = ((char*)filename)[i];
        if (!kname[i]) break;
    }
    kname[127] = 0;

    extern vfs_node_t *fs_root;
    struct process *proc = get_current_process();
    vfs_node_t *node = vfs_finddir(proc->cwd, kname);
    if (!node) {
        node = vfs_finddir(fs_root, kname);
    }
    if (!node) {
        printk("exec: '%s' not found (cwd=%x, root=%x)\n", kname, (uint32_t)(uint64_t)proc->cwd, (uint32_t)(uint64_t)fs_root);
        return -ENOENT;
    }
    printk("exec: loaded '%s' size=%u\n", kname, node->length);

    uint8_t *data = kmalloc(node->length);
    vfs_read(node, 0, node->length, data);

    char **kargv = 0;
    if (argv && is_user_range(argv, sizeof(uint64_t))) {
        int argc = 0;
        uint64_t *uargv = (uint64_t*)argv;
        while (uargv[argc]) argc++;
        kargv = kmalloc((argc + 1) * sizeof(char*));
        for (int j = 0; j < argc; j++) {
            char *s = (char*)uargv[j];
            if (!is_user_range((uint64_t)s, 1)) { kfree(kargv); kfree(data); return -EFAULT; }
            int len = 0;
            while (s[len]) len++;
            kargv[j] = kmalloc(len + 1);
            memcpy(kargv[j], s, len + 1);
        }
        kargv[argc] = 0;
    }

    char **kenvp = 0;
    if (envp && is_user_range(envp, sizeof(uint64_t))) {
        int envc = 0;
        uint64_t *uenvp = (uint64_t*)envp;
        while (uenvp[envc]) envc++;
        kenvp = kmalloc((envc + 1) * sizeof(char*));
        for (int j = 0; j < envc; j++) {
            char *s = (char*)uenvp[j];
            if (!is_user_range((uint64_t)s, 1)) { kfree(kargv); kfree(kenvp); kfree(data); return -EFAULT; }
            int len = 0;
            while (s[len]) len++;
            kenvp[j] = kmalloc(len + 1);
            memcpy(kenvp[j], s, len + 1);
        }
        kenvp[envc] = 0;
    }

    /* Set process name from executable filename */
    const char *base = kname;
    for (uint64_t j = 0; j < 127; j++) {
        if (kname[j] == '/') { base = &kname[j + 1]; }
        if (kname[j] == 0) break;
    }
    int name_len = 0;
    while (base[name_len] && name_len < 31) name_len++;
    memcpy(proc->name, base, name_len);
    proc->name[name_len] = 0;

    int ret = task_exec(proc, data, node->length, kargv, kenvp);
    if (ret < 0) printk("execve(%s) failed: %d\n", kname, ret);
    if (kargv) {
        for (int j = 0; kargv[j]; j++) kfree(kargv[j]);
        kfree(kargv);
    }
    if (kenvp) {
        for (int j = 0; kenvp[j]; j++) kfree(kenvp[j]);
        kfree(kenvp);
    }
    kfree(data);

    if (ret < 0) return ret;
    return 0;
}

static uint64_t sb_terminate(uint64_t status, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    task_exit((int)status);
    return 0;
}

static uint64_t sys_wait4(uint64_t pid, uint64_t wstatus, uint64_t options, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    struct process *parent = get_current_process();

    while (1) {
        struct process *p = proc_list;
        if (!p) return -ECHILD;
        int found_child = 0;
        int found_live_child = 0;
        do {
            if (p->ppid == parent->pid) {
                if (pid == 0 || pid == (uint64_t)(-1) || p->pid == (uint32_t)pid || pid == (uint64_t)(-2)) {
                    found_child = 1;
                    if (p->state == TASK_ZOMBIE) {
                        if (wstatus && is_user_range(wstatus, sizeof(int))) {
                            int ws = (p->exit_status & 0xFF);
                            *(int*)wstatus = ws;
                        }
                        uint32_t child_pid = p->pid;

                        struct process *prev = p;
                        while (prev->next != p) prev = prev->next;
                        prev->next = p->next;
                        if (p == proc_list) proc_list = p->next;
                        if (proc_list == p) proc_list = 0;

                        sched_remove(p);
                        if (p->cr3) vmm_destroy_address_space(p->cr3 & ~0xFFF);
                        if (p->kstack) kfree((void*)p->kstack);
                        extern slab_cache_t *process_cache;
                        slab_free(process_cache, p);
                        return child_pid;
                    }
                    if (p->state == TASK_RUNNING || p->state == TASK_READY || p->state == TASK_BLOCKED) {
                        found_live_child = 1;
                    }
                }
            }
            p = p->next;
        } while (p != proc_list);

        if (!found_child) return -ECHILD;
        if (options & WNOHANG) return 0;
        if (!found_live_child) return -ECHILD;
        parent->state = TASK_BLOCKED;
        yield();
    }
}

static uint64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (sig >= 32) return -EINVAL;

    struct process *target = proc_find(pid);
    if (!target) return -ESRCH;

    // sig 0 = just check if process exists
    if (sig == 0) return 0;

    if (sig == SIGKILL) {
        target->exit_status = -1;
        target->state = TASK_ZOMBIE;
        // Wake up parent if waiting
        struct process *parent = proc_find(target->ppid);
        if (parent && parent->state == TASK_BLOCKED) {
            parent->state = TASK_READY;
            sched_enqueue(parent);
        }
        return 0;
    }

    send_signal(pid, sig);
    return 0;
}

static uint64_t sys_uname(uint64_t buf, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(buf, sizeof(struct utsname))) return -EFAULT;

    struct utsname *u = (struct utsname*)buf;
    memcpy(u->sysname, "ShadowBox", 10);
    memcpy(u->nodename, "shadowbox", 10);
    memcpy(u->release, "0.1.0", 6);
    memcpy(u->version, "ShadowBox 0.1.0", 16);
    memcpy(u->machine, "x86_64", 7);
    u->domainname[0] = 0;
    return 0;
}

static uint64_t sys_getdents(uint64_t fd, uint64_t dirent, uint64_t count, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    if (!is_user_range(dirent, count)) return -EFAULT;
    struct file *file = process_fd_get(get_current_process(), fd);
    if (!file) return -EBADF;
    
    struct dirent *d = vfs_readdir(file->node, file->offset);
    if (!d) return 0; // End of directory
    
    if (count < sizeof(struct dirent)) return -EINVAL;
    memcpy((void*)dirent, d, sizeof(struct dirent));
    file->offset++;
    
    return sizeof(struct dirent);
}

static uint64_t sys_getcwd(uint64_t buf, uint64_t size, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(buf, size)) return -EFAULT;
    char *s = (char*)buf;
    if (size > 0) s[0] = '/';
    if (size > 1) s[1] = 0;
    return 1;
}

static uint64_t sys_chdir(uint64_t path, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(path, 1)) return -EFAULT;
    
    char name[128];
    uint64_t i;
    for (i = 0; i < 127; i++) {
        name[i] = ((char*)path)[i];
        if (!name[i]) break;
    }
    name[127] = 0;

    struct process *proc = get_current_process();
    char last_comp[128];
    vfs_node_t *dir = vfs_resolve_path(name, proc->cwd, last_comp);
    if (!dir) return -ENOENT;

    vfs_node_t *node;
    if (last_comp[0] == 0) {
        node = dir;
    } else {
        node = vfs_finddir(dir, last_comp);
    }
    if (!node) return -ENOENT;
    // FS_DIRECTORY is 0x02
    if ((node->flags & FS_DIRECTORY) == 0) return -ENOTDIR;
    
    proc->cwd = node;
    return 0;
}

static uint64_t sys_getuid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return get_current_process()->uid;
}

static uint64_t sys_getgid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return get_current_process()->gid;
}

static uint64_t sys_geteuid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return get_current_process()->euid;
}

static uint64_t sys_getegid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return get_current_process()->egid;
}

static uint64_t sys_getppid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return get_current_process()->ppid;
}

static uint64_t sys_gettid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return get_current_process()->pid;
}

static uint64_t sys_arch_prctl(uint64_t code, uint64_t addr, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (code == 0x1001) {
        wrmsr(MSR_ARCH_PRCTL_FS, addr);
        return 0;
    }
    if (code == 0x1002) {
        wrmsr(MSR_ARCH_PRCTL_GS, addr);
        return 0;
    }
    return -EINVAL;
}

static uint64_t sys_time(uint64_t tloc, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    uint64_t now = boot_ticks / hz;
    if (tloc && is_user_range(tloc, 8)) *(uint64_t*)tloc = now;
    return now;
}

static uint64_t sys_clock_gettime(uint64_t clk_id, uint64_t tp, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    (void)clk_id;
    if (!is_user_range(tp, sizeof(struct timespec))) return -EFAULT;
    struct timespec *ts = (struct timespec*)tp;
    uint64_t total_ms = (boot_ticks * 1000) / hz;
    ts->tv_sec = total_ms / 1000;
    ts->tv_nsec = (total_ms % 1000) * 1000000;
    return 0;
}

static uint64_t sb_terminate_group(uint64_t status, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    task_exit((int)status);
    return 0;
}

static uint64_t sys_times(uint64_t buf, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (buf && is_user_range(buf, 16)) {
        uint64_t *t = (uint64_t*)buf;
        t[0] = boot_ticks;
        t[1] = hz;
    }
    return boot_ticks;
}

static uint64_t sys_proc_info(uint64_t buf, uint64_t max, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(buf, max * sizeof(struct proc_info))) return -EFAULT;
    return task_proc_info((struct proc_info*)buf, (int)max);
}

static uint64_t sys_mem_info(uint64_t buf, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(buf, 16)) return -EFAULT;
    uint64_t k_total, k_used;
    pmm_get_info(&k_total, &k_used);
    uint64_t *m = (uint64_t*)buf;
    m[0] = k_total;
    m[1] = k_used;
    return 0;
}

/*
 * sys_sys_status - Snapshot of system state for the taskbar system tray.
 *   Fills a sys_status_t at buf: wifi state, bluetooth devices, uptime, memory.
 */
static uint64_t sys_sys_status(uint64_t buf, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(buf, sizeof(sys_status_t))) return -EFAULT;
    sys_status_t *st = (sys_status_t *)buf;
    memset(st, 0, sizeof(sys_status_t));

    st->uptime_ticks = boot_ticks;
    pmm_get_info(&st->mem_total, &st->mem_used);

    extern uint32_t bluetooth_device_count(void);
    st->bt_available = 1;
    st->bt_devices = (uint8_t)bluetooth_device_count();

    wifi_device_t *wifi = wifi_get_device();
    if (wifi) {
        st->wifi_state = (uint8_t)wifi->state;
        if (wifi->current_bss) {
            for (int i = 0; i < 32 && wifi->current_bss->ssid[i]; i++)
                st->wifi_ssid[i] = wifi->current_bss->ssid[i];
            st->wifi_ssid[32] = 0;
            st->wifi_signal = wifi->current_bss->signal_dbm;
        }
    }
    return 0;
}

/* sys_notify_peek - Copy up to `max` pending notifications into user buffer. */
static uint64_t sys_notify_peek(uint64_t buf, uint64_t max, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (max == 0) return 0;
    if (max > 32) max = 32;
    if (!is_user_range(buf, max * sizeof(sys_notify_t))) return -EFAULT;
    return notification_peek((sys_notify_t *)buf, (uint32_t)max);
}

/* sys_notify_dismiss - Remove a notification by id. */
static uint64_t sys_notify_dismiss(uint64_t id, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    notification_dismiss((uint32_t)id);
    return 0;
}

/* sys_power - Power management from userland.
 * action: 0 = reboot, 1 = shutdown, 2 = suspend. */
static uint64_t sys_power(uint64_t action, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (action == 0) power_reboot();
    else if (action == 1) power_shutdown();
    else if (action == 2) power_suspend();
    return 0;
}

static uint64_t sys_mkdir(uint64_t path, uint64_t mode, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(path, 1)) return -EFAULT;
    char name[128];
    int i = 0;
    while (((char*)path)[i] && i < 127) { name[i] = ((char*)path)[i]; i++; }
    name[i] = 0;
    struct process *proc = get_current_process();
    return vfs_mkdir(name, proc->cwd, mode);
}

static uint64_t sys_rmdir(uint64_t path, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(path, 1)) return -EFAULT;
    char name[128];
    int i = 0;
    while (((char*)path)[i] && i < 127) { name[i] = ((char*)path)[i]; i++; }
    name[i] = 0;
    struct process *proc = get_current_process();
    return vfs_rmdir(name, proc->cwd);
}

static uint64_t sys_unlink(uint64_t path, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(path, 1)) return -EFAULT;
    char name[128];
    int i = 0;
    while (((char*)path)[i] && i < 127) { name[i] = ((char*)path)[i]; i++; }
    name[i] = 0;
    struct process *proc = get_current_process();
    return vfs_unlink(name, proc->cwd);
}

static uint64_t sys_rename(uint64_t oldpath, uint64_t newpath, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(oldpath, 1) || !is_user_range(newpath, 1)) return -EFAULT;
    char oldname[128], newname[128];
    int i = 0;
    while (((char*)oldpath)[i] && i < 127) { oldname[i] = ((char*)oldpath)[i]; i++; }
    oldname[i] = 0;
    i = 0;
    while (((char*)newpath)[i] && i < 127) { newname[i] = ((char*)newpath)[i]; i++; }
    newname[i] = 0;
    if (tmpfs_rename(oldname, newname) == 0) return 0;
    return -ENOENT;
}

static uint64_t sys_access(uint64_t path, uint64_t mode, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)mode; (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(path, 1)) return -EFAULT;
    char name[128];
    int i = 0;
    while (((char*)path)[i] && i < 127) { name[i] = ((char*)path)[i]; i++; }
    name[i] = 0;
    // Check tmpfs
    if (tmpfs_access(name) == 0) return 0;
    // Resolve path relative to cwd (handles absolute and relative paths)
    struct process *proc = get_current_process();
    char last_comp[128];
    vfs_node_t *dir = vfs_resolve_path(name, proc->cwd, last_comp);
    if (!dir) return -ENOENT;
    vfs_node_t *node;
    if (last_comp[0] == 0) {
        node = dir;
    } else {
        node = vfs_finddir(dir, last_comp);
    }
    if (node) return 0;
    return -ENOENT;
}

static uint64_t sys_setuid(uint64_t uid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    get_current_process()->uid = (uint32_t)uid;
    return 0;
}

static uint64_t sys_setgid(uint64_t gid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    get_current_process()->gid = (uint32_t)gid;
    return 0;
}

static int days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
static inline uint64_t rtc_to_unix(uint32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    uint64_t days = 0;
    for (int y = 1970; y < year; y++) {
        days += 365 + ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    }
    for (int m = 1; m < month; m++) {
        days += days_in_month[m];
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) days++;
    }
    days += day - 1;
    return ((days * 24 + hour) * 60 + min) * 60 + sec;
}

static uint64_t sys_gettimeofday(uint64_t tv, uint64_t tz, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)tz; (void)unused1; (void)unused2; (void)unused3;
    if (tv && is_user_range(tv, 16)) {
        extern void rtc_get_time(uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint32_t*);
        uint8_t sec, min, hour, day, month;
        uint32_t year;
        rtc_get_time(&sec, &min, &hour, &day, &month, &year);
        int64_t unix_time = (int64_t)rtc_to_unix(year, month, day, hour, min, sec);
        unix_time += time_get_adjust();                  /* NTP correction */
        unix_time += (int64_t)time_get_timezone_offset() * 60; /* time zone */
        uint64_t *ptr = (uint64_t*)tv;
        ptr[0] = (uint64_t)unix_time;
        ptr[1] = 0; // usec 
        return 0;
    }
    return -EFAULT;
}

/*
 * sys_timezone - Get/set the kernel time zone offset.
 *   action 0 = get (returns offset in minutes)
 *   action 1 = set offset in minutes (arg2)
 */
static uint64_t sys_timezone(uint64_t action, uint64_t offset, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (action == 0) {
        return (uint64_t)(int64_t)time_get_timezone_offset();
    } else if (action == 1) {
        if (offset > 14 * 60 || offset < -(12 * 60)) return -EINVAL;
        time_set_timezone_offset((int)offset);
        return 0;
    }
    return -EINVAL;
}

/*
 * sys_dns_resolve - Resolve a hostname to an IPv4 address.
 *   arg1 = user pointer to NUL-terminated hostname
 *   arg2 = user pointer to uint32_t output (big-endian on the wire)
 */
static uint64_t sys_dns_resolve(uint64_t name_ptr, uint64_t out_ptr, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    char name[256];
    int i;
    if (!is_user_range(name_ptr, 1) || !is_user_range(out_ptr, 4)) return -EFAULT;
    for (i = 0; i < 255; i++) {
        name[i] = ((char *)name_ptr)[i];
        if (!name[i]) break;
    }
    name[255] = 0;
    if (i == 255) return -EINVAL;
    uint32_t ip = 0;
    int rc = dns_resolve(name, &ip);
    if (rc == 0) {
        uint32_t *out = (uint32_t *)out_ptr;
        out[0] = ((ip >> 24) & 0xFF) | ((ip >> 8) & 0xFF00) |
                 ((ip << 8) & 0xFF0000) | ((ip << 24) & 0xFF000000);
    }
    return rc;
}

/*
 * sys_ntp_sync - Sync the kernel clock against an NTP server.
 *   arg1 = server IPv4 address (network byte order)
 *   arg2 = user pointer to int64_t offset output (may be 0)
 */
static uint64_t sys_ntp_sync(uint64_t server_ip, uint64_t off_ptr, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (off_ptr && !is_user_range(off_ptr, 8)) return -EFAULT;
    int64_t offset = 0;
    int rc = ntp_sync((uint32_t)server_ip, &offset);
    if (rc == 0 && off_ptr) {
        ((int64_t *)off_ptr)[0] = offset;
    }
    return rc;
}

/*
 * sys_ping - ICMP echo request; returns RTT in ms or -1.
 *   arg1 = destination IPv4 (network byte order)
 *   arg2 = timeout in milliseconds
 */
static uint64_t sys_ping(uint64_t ip, uint64_t timeout_ms, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    return (uint64_t)(int64_t)net_ping((uint32_t)ip, (uint32_t)timeout_ms);
}

/*
 * sys_netinfo - Copy network device info to a user struct.
 *   arg1 = user pointer to struct sb_netinfo
 */
static uint64_t sys_netinfo(uint64_t ptr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    extern net_device_t *net_devices;
    if (!ptr) return -EFAULT;
    if (!net_devices) return -ENODEV;
    if (!is_user_range(ptr, 4 + 4 + 4 + 4 + 6 + 1 + 16)) return -EFAULT;
    net_device_t *dev = net_devices;
    uint8_t *u = (uint8_t *)ptr;
    uint32_t ip = ((dev->ip >> 24) & 0xFF) | ((dev->ip >> 8) & 0xFF00) |
                  ((dev->ip << 8) & 0xFF0000) | ((dev->ip << 24) & 0xFF000000);
    uint32_t nm = ((dev->netmask >> 24) & 0xFF) | ((dev->netmask >> 8) & 0xFF00) |
                  ((dev->netmask << 8) & 0xFF0000) | ((dev->netmask << 24) & 0xFF000000);
    uint32_t gw = ((dev->gateway >> 24) & 0xFF) | ((dev->gateway >> 8) & 0xFF00) |
                  ((dev->gateway << 8) & 0xFF0000) | ((dev->gateway << 24) & 0xFF000000);
    uint32_t dn = ((dns_get_server() >> 24) & 0xFF) | ((dns_get_server() >> 8) & 0xFF00) |
                  ((dns_get_server() << 8) & 0xFF0000) | ((dns_get_server() << 24) & 0xFF000000);
    ((uint32_t *)u)[0] = ip;
    ((uint32_t *)u)[1] = nm;
    ((uint32_t *)u)[2] = gw;
    ((uint32_t *)u)[3] = dn;
    for (int i = 0; i < 6; i++) u[16 + i] = dev->mac[i];
    u[22] = dev->ip != 0 ? 1 : 0; /* link */
    int nl = 0;
    while (dev->name[nl] && nl < 15) { u[23 + nl] = dev->name[nl]; nl++; }
    u[23 + nl] = 0;
    return 0;
}

static uint64_t sb_socket_create_wrapper(uint64_t domain, uint64_t type, uint64_t protocol, uint64_t u1, uint64_t u2) {
    (void)u1; (void)u2; return sb_socket_create((int)domain, (int)type, (int)protocol);
}
static uint64_t sb_socket_connect_wrapper(uint64_t sockfd, uint64_t addr, uint64_t addrlen, uint64_t u1, uint64_t u2) {
    (void)u1; (void)u2; return sb_socket_connect((int)sockfd, (const void*)addr, (size_t)addrlen);
}
static uint64_t sb_socket_bind_wrapper(uint64_t sockfd, uint64_t addr, uint64_t addrlen, uint64_t u1, uint64_t u2) {
    (void)u1; (void)u2; return sb_socket_bind((int)sockfd, (const void*)addr, (size_t)addrlen);
}
static uint64_t sb_socket_listen_wrapper(uint64_t sockfd, uint64_t backlog, uint64_t u1, uint64_t u2, uint64_t u3) {
    (void)u1; (void)u2; (void)u3; return sb_socket_listen((int)sockfd, (int)backlog);
}
static uint64_t sb_socket_accept_wrapper(uint64_t sockfd, uint64_t addr, uint64_t addrlen, uint64_t u1, uint64_t u2) {
    (void)u1; (void)u2; return sb_socket_accept((int)sockfd, (void*)addr, (size_t*)addrlen);
}
static uint64_t sb_socket_sendto_wrapper(uint64_t sockfd, uint64_t buf, uint64_t len, uint64_t flags, uint64_t u1) {
    (void)u1; (void)flags; return sb_push(sockfd, buf, len, 0, 0); // Simplified
}
static uint64_t sb_socket_recvfrom_wrapper(uint64_t sockfd, uint64_t buf, uint64_t len, uint64_t flags, uint64_t u1) {
    (void)u1; (void)flags; return sb_pull(sockfd, buf, len, 0, 0); // Simplified
}

static uint64_t sys_getpriority(uint64_t which, uint64_t who, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)which; (void)who; (void)unused1; (void)unused2; (void)unused3;
    // Simplified: return default priority
    return 0;
}

static uint64_t sys_setpriority(uint64_t which, uint64_t who, uint64_t prio, uint64_t unused1, uint64_t unused2) {
    (void)which; (void)who; (void)prio; (void)unused1; (void)unused2;
    // Simplified: always succeed
    return 0;
}

static uint64_t sys_getrlimit(uint64_t resource, uint64_t rlim, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)resource; (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(rlim, 16)) return -EFAULT;
    // Return default limits (unlimited)
    uint64_t *limits = (uint64_t*)rlim;
    limits[0] = 0xFFFFFFFFFFFFFFFF; // Soft limit (unlimited)
    limits[1] = 0xFFFFFFFFFFFFFFFF; // Hard limit (unlimited)
    return 0;
}

static uint64_t sys_setrlimit(uint64_t resource, uint64_t rlim, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)resource; (void)rlim; (void)unused1; (void)unused2; (void)unused3;
    // Simplified: always succeed
    return 0;
}

static uint64_t sys_getrusage(uint64_t who, uint64_t usage, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)who; (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(usage, 72)) return -EFAULT;
    // Zero out the usage structure
    uint8_t *ptr = (uint8_t*)usage;
    for (int i = 0; i < 72; i++) ptr[i] = 0;
    return 0;
}

static uint64_t sys_sync(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    // Sync filesystem buffers (no-op for now)
    return 0;
}

static uint64_t sys_fsync(uint64_t fd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)fd; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    // Sync file buffers (no-op for now)
    return 0;
}

static uint64_t sys_getgroups(uint64_t size, uint64_t list, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    struct process *proc = get_current_process();
    if (size == 0) {
        return 1; // Only one group (primary group)
    }
    if (!is_user_range(list, size * 4)) return -EFAULT;
    uint32_t *groups = (uint32_t*)list;
    if (size >= 1) groups[0] = proc->gid;
    return 1;
}

static uint64_t sys_setgroups(uint64_t size, uint64_t list, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)size; (void)list; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

static uint64_t sys_mount(uint64_t source, uint64_t target, uint64_t fstype, uint64_t flags, uint64_t data) {
    (void)source; (void)flags; (void)data;
    if (!is_user_range(target, 1)) return -EFAULT;
    if (!is_user_range(fstype, 1)) return -EFAULT;
    char target_path[128]; int i = 0;
    while (((char*)target)[i] && i < 127) { target_path[i] = ((char*)target)[i]; i++; }
    target_path[i] = 0;
    char fsname[128]; i = 0;
    while (((char*)fstype)[i] && i < 127) { fsname[i] = ((char*)fstype)[i]; i++; }
    fsname[i] = 0;
    if (strcmp(fsname, "tmpfs") == 0) {
        extern vfs_node_t *tmpfs_root;
        if (tmpfs_root) { vfs_mount(target_path, tmpfs_root, flags); return 0; }
    }
    if (strcmp(fsname, "devfs") == 0) {
        extern vfs_node_t *devfs_root;
        if (devfs_root) { vfs_mount(target_path, devfs_root, flags); return 0; }
    }
    if (strcmp(fsname, "proc") == 0) {
        extern vfs_node_t *procfs_root;
        if (procfs_root) { vfs_mount(target_path, procfs_root, flags); return 0; }
    }
    return -EINVAL;
}

static uint64_t sys_umount2(uint64_t target, uint64_t flags, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(target, 1)) return -EFAULT;
    char target_path[128]; int i = 0;
    while (((char*)target)[i] && i < 127) { target_path[i] = ((char*)target)[i]; i++; }
    target_path[i] = 0;
    return vfs_unmount(target_path);
}

static uint64_t sys_fb_mmap(uint64_t u1, uint64_t u2, uint64_t u3, uint64_t u4, uint64_t u5) {
    (void)u1; (void)u2; (void)u3; (void)u4; (void)u5;
    extern uint64_t fb_get_phys(void);
    uint64_t fb_phys = fb_get_phys();
    if (!fb_phys) return -ENODEV;
    extern uint32_t fb_get_width(void);
    extern uint32_t fb_get_height(void);
    extern uint32_t fb_get_pitch(void);
    uint32_t width = fb_get_width();
    uint32_t height = fb_get_height();
    uint32_t pitch = fb_get_pitch();
    uint64_t fb_size = (uint64_t)height * pitch;

    uint64_t vaddr = 0x78000000ULL;
    struct process *proc = get_current_process();
    uint64_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
    }

    uint64_t page_start = vaddr & ~(0x1000 - 1);
    uint64_t page_end = (vaddr + fb_size + 0x1000 - 1) & ~(0x1000 - 1);
    for (uint64_t offset = 0; offset < page_end - page_start; offset += 0x1000) {
        vmm_map_page(fb_phys + offset, page_start + offset, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
    }
    return vaddr;
}

static uint64_t sys_fb_info(uint64_t buf, uint64_t u1, uint64_t u2, uint64_t u3, uint64_t u4) {
    (void)u1; (void)u2; (void)u3; (void)u4;
    if (!is_user_range(buf, 16)) return -EFAULT;
    uint32_t *m = (uint32_t*)buf;
    extern void fb_get_info(uint32_t*, uint32_t*, uint32_t*, uint8_t*);
    fb_get_info(&m[0], &m[1], &m[2], (uint8_t*)&m[3]);
    return 0;
}

static uint64_t sys_chmod(uint64_t pathname, uint64_t mode, uint64_t unused1, uint64_t unused2, uint64_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!is_user_range(pathname, 1)) return -EFAULT;
    char name[128];
    for (int i = 0; i < 127; i++) {
        name[i] = ((char*)pathname)[i];
        if (!name[i]) break;
    }
    name[127] = 0;
    
    struct process *proc = get_current_process();
    extern vfs_node_t *fs_root;
    vfs_node_t *node = vfs_finddir(proc->cwd, name);
    if (!node) node = vfs_finddir(fs_root, name);
    if (!node) return -ENOENT;
    
    if (proc->uid != 0 && proc->uid != node->uid) return -EPERM;
    
    node->mask = (mode & 07777); // Update permissions
    return 0;
}

static uint64_t sys_chown(uint64_t pathname, uint64_t owner, uint64_t group, uint64_t unused1, uint64_t unused2) {
    (void)unused1; (void)unused2;
    if (!is_user_range(pathname, 1)) return -EFAULT;
    char name[128];
    for (int i = 0; i < 127; i++) {
        name[i] = ((char*)pathname)[i];
        if (!name[i]) break;
    }
    name[127] = 0;
    
    struct process *proc = get_current_process();
    extern vfs_node_t *fs_root;
    vfs_node_t *node = vfs_finddir(proc->cwd, name);
    if (!node) node = vfs_finddir(fs_root, name);
    if (!node) return -ENOENT;
    
    if (proc->uid != 0) return -EPERM; // Only root can chown
    
    if ((int32_t)owner != -1) node->uid = owner;
    if ((int32_t)group != -1) node->gid = group;
    return 0;
}

static uint64_t (*syscall_table[])(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) = {
    [SB_PULL_DATA]         = sb_pull,
    [SB_PUSH_DATA]        = sb_push,
    [SB_ACQUIRE]         = sb_acquire,
    [SB_RELEASE]        = sb_release,
    [SYS_STAT]         = sys_stat,
    [SYS_FSTAT]        = sys_fstat,
    [SYS_LSEEK]        = sys_lseek,
    [SYS_MMAP]         = sys_mmap,
    [SYS_MPROTECT]     = sys_mprotect,
    [SYS_MUNMAP]       = sys_munmap,
    [SYS_BRK]          = sys_brk,
    [SB_SOCKET_CREATE]       = sb_socket_create_wrapper,
    [SB_SOCKET_CONNECT]      = sb_socket_connect_wrapper,
    [SB_SOCKET_ACCEPT]       = sb_socket_accept_wrapper,
    [SB_SOCKET_SENDTO]       = sb_socket_sendto_wrapper,
    [SB_SOCKET_RECVFROM]     = sb_socket_recvfrom_wrapper,
    [SB_SOCKET_BIND]         = sb_socket_bind_wrapper,
    [SB_SOCKET_LISTEN]       = sb_socket_listen_wrapper,
    [SYS_RT_SIGACTION]   = sys_rt_sigaction,
    [SYS_RT_SIGPROCMASK] = sys_rt_sigprocmask,
    [SYS_RT_SIGRETURN]   = sys_rt_sigreturn,
    [SYS_IOCTL]        = sys_ioctl,
    [SYS_PIPE]         = sys_pipe,
    [SYS_SCHED_YIELD]  = sys_sched_yield,
    [SYS_DUP]          = sys_dup,
    [SYS_DUP2]         = sys_dup2,
    [SYS_NANOSLEEP]    = sys_nanosleep,
    [SYS_GETPID]       = sys_getpid,
    [SB_REPLICATE]         = sb_replicate,
    [SB_MORPH]       = sb_morph,
    [SB_TERMINATE]         = sb_terminate,
    [SYS_WAIT4]        = sys_wait4,
    [SYS_KILL]         = sys_kill,
    [SYS_UNAME]        = sys_uname,
    [SYS_GETDENTS]     = sys_getdents,
    [SYS_GETCWD]       = sys_getcwd,
    [SYS_CHDIR]        = sys_chdir,
    [SYS_GETUID]       = sys_getuid,
    [SYS_GETGID]       = sys_getgid,
    [SYS_GETEUID]      = sys_geteuid,
    [SYS_GETEGID]      = sys_getegid,
    [SYS_GETPPID]      = sys_getppid,
    [SYS_ARCH_PRCTL]   = sys_arch_prctl,
    [SYS_GETTID]       = sys_gettid,
    [SYS_TIME]         = sys_time,
    [SYS_CLOCK_GETTIME]  = sys_clock_gettime,
    [SB_TERMINATE_GROUP]   = sb_terminate_group,
    [SYS_TIMES]        = sys_times,
    [SYS_PROC_INFO]    = sys_proc_info,
    [SYS_MEM_INFO]     = sys_mem_info,
    [SYS_ACCESS]       = sys_access,
    [SYS_MKDIR]        = sys_mkdir,
    [SYS_RMDIR]        = sys_rmdir,
    [SYS_UNLINK]       = sys_unlink,
    [90]               = sys_chmod,
    [92]               = sys_chown,
    [100]              = sys_times,
    [SYS_RENAME]       = sys_rename,
    [SYS_GETTIMEOFDAY] = sys_gettimeofday,
    [SYS_SETUID]       = sys_setuid,
    [SYS_SETGID]       = sys_setgid,
    [SYS_GETPRIORITY]  = sys_getpriority,
    [SYS_SETPRIORITY]  = sys_setpriority,
    [SYS_GETRLIMIT]    = sys_getrlimit,
    [SYS_SETRLIMIT]    = sys_setrlimit,
    [SYS_GETRUSAGE]    = sys_getrusage,
    [SYS_SYNC]         = sys_sync,
    [SYS_FSYNC]        = sys_fsync,
    [SYS_GETGROUPS]    = sys_getgroups,
    [SYS_SETGROUPS]    = sys_setgroups,
    [SYS_MOUNT]        = sys_mount,
    [SYS_UMOUNT2]      = sys_umount2,
    [SYS_FB_MMAP]      = sys_fb_mmap,
    [SYS_FB_INFO]      = sys_fb_info,
    [SYS_SYS_STATUS]   = sys_sys_status,
    [SYS_NOTIFY_PEEK]  = sys_notify_peek,
    [SYS_NOTIFY_DISMISS] = sys_notify_dismiss,
    [SYS_POWER]          = sys_power,
    [SYS_TIMEZONE]       = sys_timezone,
    [SYS_DNS_RESOLVE]    = sys_dns_resolve,
    [SYS_NTP_SYNC]       = sys_ntp_sync,
    [SYS_PING]           = sys_ping,
    [SYS_NETINFO]        = sys_netinfo,
    [SB_IPC_CALL]        = sys_sb_ipc_call,
    [SB_IPC_REPLY_WAIT]  = sys_sb_ipc_reply_wait,
};

#define SYSCALL_COUNT (sizeof(syscall_table) / sizeof(syscall_table[0]))

uint64_t syscall_handler(uint64_t rax, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    if (rax < SYSCALL_COUNT && syscall_table[rax]) {
        return syscall_table[rax](arg1, arg2, arg3, arg4, arg5);
    }
    return -ENOSYS;
}
