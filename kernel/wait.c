#include "task.h"
#include "memory.h"
#include "errno.h"
#include "malloc.h"
#include "wait.h"

/*
 * Simple wait4 implementation for the ShadowBox OS.
 *
 * Supports waiting for any child (pid == (uint64_t)-1) and blocks
 * until a child becomes a zombie. Options are currently ignored.
 * The exit status of the child is written to the user-provided
 * pointer if it is non‑zero.
 */
uint64_t sys_wait4(uint64_t pid, uint64_t wstatus, uint64_t options, uint64_t unused1, uint64_t unused2) {
    (void)pid;     // Only -1 (any child) is supported for now
    (void)options; (void)unused1; (void)unused2; // Options are ignored (no WNOHANG support)

    struct process *cur = get_current_process();
    if (!cur) return -ECHILD;

    while (1) {
        struct process *p = proc_list;
        int any_child = 0;
        if (!p) return -ECHILD; // No processes at all
        do {
            if (p->ppid == cur->pid) {
                any_child = 1;
                if (p->state == TASK_ZOMBIE) {
                    uint64_t child_pid = p->pid;
                    int status = p->exit_status;
                    if (wstatus) {
                        if (copy_to_user((void *)wstatus, &status, sizeof(int)) != 0) {
                            return -EFAULT;
                        }
                    }
                    // Unlink the child from the global process list
                    struct process *prev = proc_list;
                    while (prev->next != p) {
                        prev = prev->next;
                    }
                    prev->next = p->next;
                    if (proc_list == p) {
                        proc_list = p->next;
                    }
                    // Free kernel stack if allocated
                    if (p->kstack) {
                        kfree((void *)p->kstack);
                    }
                    // Finally free the process structure itself
                    kfree(p);
                    return child_pid;
                }
            }
            p = p->next;
        } while (p != proc_list);

        if (!any_child) {
            // No child processes exist for this parent
            return -ECHILD;
        }
        // No zombie child yet – yield the CPU and try again
        yield();
    }
    // Unreachable
    return -ECHILD;
}
