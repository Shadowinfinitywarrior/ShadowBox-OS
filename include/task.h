#ifndef SHADOWBOX_TASK_H
#define SHADOWBOX_TASK_H

#include "types.h"
#include "signal.h"
#include "vfs.h"
#include "errno.h"
#include "sb_ipc.h"

#define MAX_FDS 256
#define PAGE_SIZE 4096
#define KERNEL_STACK_SIZE 16384

#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_BLOCKED  2
#define TASK_ZOMBIE   3
#define TASK_STOPPED  4

#define SCHED_NORMAL  0
#define SCHED_FIFO    1
#define SCHED_RR      2
#define SCHED_BATCH   3
#define SCHED_ISO     4
#define SCHED_IDLE    5

#define SCHED_CLASS_CFS  0
#define SCHED_CLASS_RT   1
#define SCHED_CLASS_FIFO 2
#define SCHED_CLASS_IDLE 3

/*
 * context - Saved CPU context for context switching
 * @r15-r12, @rbx, @rbp: Callee-saved registers
 * @rip:                 Return instruction pointer
 */
struct context {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rip;
};

/*
 * file - Open file descriptor
 * @node:     VFS node
 * @offset:   Current file offset
 * @flags:    Open flags
 * @refcount: Reference count
 */
struct file {
    vfs_node_t *node;
    uint64_t offset;
    uint32_t flags;
    uint32_t refcount;
};

/*
 * process - Process control block
 * @pid, @ppid: Process and parent PID
 * @state:      Task state (TASK_*)
 * @uid, @gid, @euid, @egid: Credentials
 * @ctx:        Saved context for switching
 * @cr3:        Page table physical address
 * @kstack:     Kernel stack pointer
 * @fds:        File descriptor table
 * @brk_start, @brk_end: Heap region
 * @cwd:        Current working directory
 * @sig_pending, @sig_blocked: Signal state
 * @sig_actions: Signal handler table
 * @exit_status: Exit code
 * @next:       Next process in global list
 * @vruntime:   Virtual runtime for CFS
 * @sched_policy, @nice, @static_prio, @rt_priority: Scheduling params
 * @time_slice: Remaining time slice
 * @tid:        Thread ID
 * @thread_group: Thread group leader
 * @next_thread: Next thread in group
 * @thread_count: Number of threads
 * @rlim_cur, @rlim_max: Resource limits
 * @start_time, @user_time, @sys_time: Timing stats
 * @mmap_base, @stack_base: Memory layout bases
 * @name: Process name (basename of executable)
 */
struct process {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint32_t uid;
    uint32_t gid;
    uint32_t euid;
    uint32_t egid;

    struct context *ctx;
    uint64_t cr3;
    uint64_t kstack;

    struct file *fds[MAX_FDS];

    // --- Microkernel IPC Endpoint State ---
    int ipc_status;          // 0 = NONE, 1 = RECEIVING, 2 = SENDING
    uint32_t ipc_peer;       // PID of the partner process
    uint64_t ipc_msg_ptr;    // Pointer to user-space sb_msg_t (either receiving or sending)
    struct process *ipc_waiters; // Queue of processes waiting to send to us
    struct process *ipc_next_waiter; // Linked list for the queue

    uint64_t brk_start;
    uint64_t brk_end;

    vfs_node_t *cwd;

    uint64_t sig_pending;
    uint64_t sig_blocked;
    struct sigaction sig_actions[32];

    int exit_status;
    struct process *next;
    uint64_t vruntime;

    int sched_policy;
    int nice;
    int static_prio;
    int rt_priority;
    uint64_t time_slice;

    uint32_t tid;
    struct process *thread_group;
    struct process *next_thread;
    uint32_t thread_count;

    uint64_t rlim_cur[RLIMIT_NLIMITS];
    uint64_t rlim_max[RLIMIT_NLIMITS];

    int sched_class;
    int cpu_affinity;
    int preempt_count;

    uint64_t start_time;
    uint64_t user_time;
    uint64_t sys_time;
    uint64_t user_rsp;

    uint64_t mmap_base;
    uint64_t stack_base;

    char name[32];
};

/*
 * get_current_process - Get the currently running process
 * Returns: Current process pointer
 */
struct process *get_current_process(void);

/*
 * task_init - Initialize task subsystem
 */
void task_init(void);

/*
 * task_exit - Exit the current process
 * @status: Exit status code
 */
void task_exit(int status);

/*
 * task_create_proc - Create a new kernel process
 * @entry: Entry function
 * @arg:   Argument to entry function
 * Returns: New process, or NULL
 */
struct process *task_create_proc(void (*entry)(void*), void *arg);

/*
 * task_fork - Fork the current process
 * Returns: Child PID, or -1 on error
 */
struct process *task_fork(void);

/*
 * task_exec - Execute a new program in the current process
 * @proc:    Process to exec in
 * @elf_data: ELF binary data
 * @size:    Binary size
 * @argv:    Argument vector
 * @envp:    Environment vector
 * Returns: 0 on success, -1 on error
 */
int task_exec(struct process *proc, uint8_t *elf_data, uint64_t size, char **argv, char **envp);

/*
 * schedule - Invoke the scheduler
 */
void schedule(void);

/*
 * yield - Voluntarily yield the CPU
 */
void yield(void);

/*
 * process_fd_install - Install a file descriptor
 * @proc: Process
 * @file: File to install
 * Returns: File descriptor number, or -1
 */
int process_fd_install(struct process *proc, struct file *file);

/*
 * process_fd_get - Get file by descriptor
 * @proc: Process
 * @fd:   File descriptor
 * Returns: File pointer, or NULL
 */
struct file *process_fd_get(struct process *proc, int fd);

/*
 * process_fd_close - Close a file descriptor
 * @proc: Process
 * @fd:   File descriptor
 */
void process_fd_close(struct process *proc, int fd);

/*
 * process_fd_fork - Duplicate file descriptors for fork
 * @parent: Parent process
 * @child:  Child process
 */
void process_fd_fork(struct process *parent, struct process *child);

/*
 * proc_find - Find a process by PID
 * @pid: PID to find
 * Returns: Process pointer, or NULL
 */
struct process *proc_find(uint32_t pid);

extern struct process *proc_list;

/*
 * proc_info - Process info structure for system calls
 */
struct proc_info {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint64_t kstack;
    uint64_t cr3;
    char name[32];
};

/*
 * task_proc_info - Fill process info buffer
 * @buf: Buffer to fill
 * @max: Maximum entries
 * Returns: Number of entries written
 */
int task_proc_info(struct proc_info *buf, int max);

#endif
