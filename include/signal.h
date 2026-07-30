#ifndef SHADOWBOX_SIGNAL_H
#define SHADOWBOX_SIGNAL_H

#include "types.h"

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGSTKFLT 16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPWR    30
#define SIGSYS    31

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

#define SA_NOCLDSTOP 1
#define SA_NOCLDWAIT 2
#define SA_SIGINFO   4
#define SA_ONSTACK   0x08000000
#define SA_RESTART   0x10000000
#define SA_NODEFER   0x40000000
#define SA_RESETHAND 0x80000000

struct sigaction {
    void (*sa_handler)(int);
    uint64_t sa_flags;
    void (*sa_restorer)(void);
    uint64_t sa_mask;
};

void signal_init(void);
int send_signal(int pid, int sig);
int sys_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
void (*sys_signal(int sig, void (*handler)(int)))(int);
int sys_sigprocmask(int how, const uint64_t *set, uint64_t *oldset);
void check_and_deliver_signals(void);

#endif
