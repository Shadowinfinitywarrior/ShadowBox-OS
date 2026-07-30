#include "kernel.h"
#include "task.h"
#include "sb_ipc.h"
#include "errno.h"

// Helper to find process
extern struct process *proc_list;

static struct process *find_process(uint32_t pid) {
    struct process *p = proc_list;
    while (p) {
        if (p->pid == pid && p->state != TASK_ZOMBIE) return p;
        p = p->next;
    }
    return 0;
}

uint64_t sys_sb_ipc_call(uint64_t target_pid, uint64_t msg_ptr, uint64_t u3, uint64_t u4, uint64_t u5) {
    (void)u3; (void)u4; (void)u5;
    struct process *current = get_current_process();
    if (!current) return -EINVAL;

    struct process *target = find_process((uint32_t)target_pid);
    if (!target) return -ESRCH;

    current->ipc_status = 2; // SENDING/WAITING FOR REPLY
    current->ipc_peer = target->pid;
    current->ipc_msg_ptr = msg_ptr;

    // Check if target is waiting to receive
    if (target->ipc_status == 1 && (target->ipc_peer == 0 || target->ipc_peer == current->pid)) {
        // Target is receiving! Copy msg directly.
        sb_msg_t *src_msg = (sb_msg_t *)msg_ptr;
        sb_msg_t *dst_msg = (sb_msg_t *)target->ipc_msg_ptr;
        *dst_msg = *src_msg;
        dst_msg->data4 = current->pid; // Embed sender PID for the receiver

        // Wake up target
        target->ipc_status = 0;
        target->state = TASK_READY;
    } else {
        // Target is busy, enqueue ourselves
        current->ipc_next_waiter = 0;
        if (!target->ipc_waiters) {
            target->ipc_waiters = current;
        } else {
            struct process *w = target->ipc_waiters;
            while (w->ipc_next_waiter) w = w->ipc_next_waiter;
            w->ipc_next_waiter = current;
        }
    }

    // Block ourselves until target replies
    current->state = TASK_BLOCKED;
    yield();

    return 0; // Woken up by reply
}

uint64_t sys_sb_ipc_reply_wait(uint64_t reply_pid, uint64_t reply_msg_ptr, uint64_t req_msg_ptr, uint64_t u4, uint64_t u5) {
    (void)u4; (void)u5;
    struct process *current = get_current_process();
    if (!current) return -EINVAL;

    // 1. Send Reply if applicable
    if (reply_pid != 0 && reply_msg_ptr != 0) {
        struct process *target = find_process((uint32_t)reply_pid);
        if (target && target->ipc_status == 2 && target->ipc_peer == current->pid) {
            sb_msg_t *src_msg = (sb_msg_t *)reply_msg_ptr;
            sb_msg_t *dst_msg = (sb_msg_t *)target->ipc_msg_ptr;
            *dst_msg = *src_msg; // Copy reply back to sender

            // Wake up sender
            target->ipc_status = 0;
            target->state = TASK_READY;
        }
    }

    // 2. Wait for next request
    if (current->ipc_waiters) {
        // We have pending senders in queue!
        struct process *sender = current->ipc_waiters;
        current->ipc_waiters = sender->ipc_next_waiter;
        
        // Copy their request to us
        sb_msg_t *src_msg = (sb_msg_t *)sender->ipc_msg_ptr;
        sb_msg_t *dst_msg = (sb_msg_t *)req_msg_ptr;
        *dst_msg = *src_msg;
        dst_msg->data4 = sender->pid; // Embed sender PID

        // NOTE: sender remains blocked (ipc_status == 2) until we reply to them later!
        return 0; // Return immediately to user-space to process the message
    }

    // No pending messages, go to sleep
    current->ipc_status = 1; // RECEIVING
    current->ipc_peer = 0;   // Any
    current->ipc_msg_ptr = req_msg_ptr;
    current->state = TASK_BLOCKED;
    yield();

    return 0; // Woken up by a new sender
}
