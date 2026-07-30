#include "signal.h"
#include "kernel.h"
#include "task.h"
#include "malloc.h"
#include "kstring.h"
#include "errno.h"
#include "sched.h"

void signal_init(void) {
    printk("SIGNAL: Initializing enhanced POSIX signal routing and handlers...\n");
}

// Default signal actions
static void sig_default_term(int sig) {
    (void)sig;
    // Terminate the process
    task_exit(128 + sig);
}

static void sig_default_ignore(int sig) {
    (void)sig;
    // Do nothing
}

static void sig_default_stop(int sig) {
    (void)sig;
    // Stop the process (not implemented yet)
}

static void sig_default_cont(int sig) {
    (void)sig;
    // Continue the process (not implemented yet)
}

// Get default action for a signal
static void (*get_default_action(int sig))(int) {
    switch (sig) {
        case SIGCHLD:
        case SIGCONT:
        case SIGWINCH:
        case SIGURG:
            return sig_default_ignore;
        case SIGSTOP:
        case SIGTSTP:
        case SIGTTIN:
        case SIGTTOU:
            return sig_default_stop;
        case SIGKILL:
        case SIGTERM:
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGILL:
        case SIGTRAP:
        case SIGABRT:
        case SIGBUS:
        case SIGFPE:
        case SIGSEGV:
        case SIGPIPE:
        case SIGALRM:
        case SIGUSR1:
        case SIGUSR2:
        case SIGSTKFLT:
        case SIGXCPU:
        case SIGXFSZ:
        case SIGVTALRM:
        case SIGPROF:
        case SIGPWR:
        case SIGSYS:
        default:
            return sig_default_term;
    }
}

int send_signal(int pid, int sig) {
    if (sig < 1 || sig > 31) return -EINVAL;
    
    struct process *proc = proc_find(pid);
    if (!proc) return -ESRCH;
    
    proc->sig_pending |= (1ULL << (sig - 1));
    
    if (proc->state == TASK_BLOCKED) {
        proc->state = TASK_READY;
        sched_enqueue(proc);
    }
    
    return 0;
}

int sys_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
    if (sig < 1 || sig > 31) return -EINVAL;
    if (sig == SIGKILL || sig == SIGSTOP) return -EINVAL;
    
    struct process *proc = get_current_process();
    if (!proc) return -ESRCH;
    
    // Save old action if requested
    if (oldact) {
        oldact->sa_handler = proc->sig_actions[sig - 1].sa_handler;
        oldact->sa_flags = proc->sig_actions[sig - 1].sa_flags;
        oldact->sa_mask = proc->sig_actions[sig - 1].sa_mask;
        oldact->sa_restorer = proc->sig_actions[sig - 1].sa_restorer;
    }
    
    // Set new action if provided
    if (act) {
        proc->sig_actions[sig - 1].sa_handler = act->sa_handler;
        proc->sig_actions[sig - 1].sa_flags = act->sa_flags;
        proc->sig_actions[sig - 1].sa_mask = act->sa_mask;
        proc->sig_actions[sig - 1].sa_restorer = act->sa_restorer;
    }
    
    return 0;
}

void (*sys_signal(int sig, void (*handler)(int)))(int) {
    struct sigaction act, oldact;
    act.sa_handler = handler;
    act.sa_flags = 0;
    act.sa_mask = 0;
    act.sa_restorer = 0;
    
    if (sys_sigaction(sig, &act, &oldact) < 0) {
        return SIG_ERR;
    }
    
    return oldact.sa_handler;
}

int sys_sigprocmask(int how, const uint64_t *set, uint64_t *oldset) {
    struct process *proc = get_current_process();
    if (!proc) return -ESRCH;
    
    // Save old mask if requested
    if (oldset) {
        *oldset = proc->sig_blocked;
    }
    
    // Modify mask if set provided
    if (set) {
        switch (how) {
            case 0: // SIG_BLOCK
                proc->sig_blocked |= *set;
                break;
            case 1: // SIG_UNBLOCK
                proc->sig_blocked &= ~(*set);
                break;
            case 2: // SIG_SETMASK
                proc->sig_blocked = *set;
                break;
            default:
                return -EINVAL;
        }
        
        // SIGKILL and SIGSTOP cannot be blocked
        proc->sig_blocked &= ~(1ULL << (SIGKILL - 1));
        proc->sig_blocked &= ~(1ULL << (SIGSTOP - 1));
    }
    
    return 0;
}

void check_and_deliver_signals(void) {
    struct process *proc = get_current_process();
    if (!proc) return;
    
    uint64_t pending = proc->sig_pending & ~proc->sig_blocked;
    
    if (pending == 0) return;
    
    // Find the lowest-numbered pending signal
    for (int sig = 1; sig <= 31; sig++) {
        if (pending & (1ULL << (sig - 1))) {
            // Clear the signal
            proc->sig_pending &= ~(1ULL << (sig - 1));
            
            // Get the action
            void (*handler)(int) = proc->sig_actions[sig - 1].sa_handler;
            
            if (handler == SIG_IGN) {
                // Ignore the signal
                continue;
            } else if (handler == SIG_DFL || handler == NULL) {
                // Default action
                void (*default_handler)(int) = get_default_action(sig);
                default_handler(sig);
            } else if ((uint64_t)handler >= 0xFFFFFFFF80000000ULL) {
                // Kernel-space handler address - safe to call directly
                handler(sig);
            } else {
                // User-space handler - would need signal frame setup.
                // Fall back to default action for now.
                printk("SIGNAL: user-space handler for sig %d not supported, using default\n", sig);
                void (*default_handler)(int) = get_default_action(sig);
                default_handler(sig);
            }
            
            // Only handle one signal at a time for simplicity
            break;
        }
    }
}
