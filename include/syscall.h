#ifndef SHADOWBOX_SYSCALL_H
#define SHADOWBOX_SYSCALL_H

#include "types.h"

#define SB_PULL_DATA        0
#define SB_PUSH_DATA       1
#define SB_ACQUIRE        2
#define SB_RELEASE       3
#define SYS_STAT        4
#define SYS_FSTAT       5
#define SYS_LSEEK       8
#define SYS_MMAP        9
#define SYS_MPROTECT    10
#define SYS_MUNMAP      11
#define SYS_BRK         12
#define SYS_RT_SIGACTION    13
#define SYS_RT_SIGPROCMASK  14
#define SYS_RT_SIGRETURN    15
#define SYS_IOCTL       16
#define SYS_PIPE        22
#define SYS_SCHED_YIELD 24
#define SYS_DUP         32
#define SYS_DUP2        33
#define SYS_NANOSLEEP   35
#define SYS_GETPID      39
#define SYS_CLONE       56
#define SB_REPLICATE        57
#define SB_MORPH      59
#define SB_TERMINATE        60
#define SYS_WAIT4       61
#define SYS_KILL        62
#define SYS_UNAME       63
#define SYS_GETDENTS    78
#define SYS_GETCWD      79
#define SYS_CHDIR       80
#define SYS_GETUID      102
#define SYS_GETGID      104
#define SYS_GETEUID     107
#define SYS_GETEGID     108
#define SYS_GETPPID     110
#define SYS_ARCH_PRCTL  158
#define SYS_GETTID      186
#define SYS_TIME        201
#define SYS_CLOCK_GETTIME 228
#define SB_TERMINATE_GROUP  231
#define SYS_TIMES       100
#define SYS_PROC_INFO   101
#define SYS_MEM_INFO    120
#define SYS_ACCESS      21
#define SYS_RENAME      82
#define SYS_MKDIR       83
#define SYS_RMDIR       84
#define SYS_UNLINK      87
#define SYS_GETTIMEOFDAY 96
#define SYS_SETUID      105
#define SYS_SETGID      106
#define SB_SOCKET_CREATE      41
#define SB_SOCKET_CONNECT     42
#define SB_SOCKET_ACCEPT      43
#define SB_SOCKET_SENDTO      44
#define SB_SOCKET_RECVFROM    45
#define SB_SOCKET_BIND        49
#define SB_SOCKET_LISTEN      50

#define SB_IPC_CALL           250
#define SB_IPC_REPLY_WAIT     251

#define SYS_GETPRIORITY 140
#define SYS_SETPRIORITY 141
#define SYS_GETRLIMIT   97
#define SYS_SETRLIMIT   160
#define SYS_GETRUSAGE   165
#define SYS_SYNC        162
#define SYS_FSYNC       74
#define SYS_GETGROUPS   115
#define SYS_SETGROUPS   116
#define SYS_MOUNT       169
#define SYS_FB_MMAP      200
#define SYS_INPUT_FD    201
#define SYS_FB_INFO      202
#define SYS_UMOUNT2     52

/*
 * syscall_init - Initialize syscall handler (MSR_LSTAR setup)
 */
void syscall_init(void);

/*
 * syscall_handler - Main syscall dispatch function
 * @rax:  Syscall number
 * @arg1-arg5: Syscall arguments
 * Returns: Syscall return value
 */
uint64_t syscall_handler(uint64_t rax, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

/*
 * syscall_set_kernel_stack - Set kernel stack for syscalls
 * @stack: Stack pointer value
 */
void syscall_set_kernel_stack(uint64_t stack);

/*
 * syscall_get_user_stack - Get user stack from syscall entry
 * Returns: User stack pointer
 */
uint64_t syscall_get_user_stack(void);

/*
 * syscall_set_user_stack - Set user stack for syscall return
 * @stack: Stack pointer value
 */
void syscall_set_user_stack(uint64_t stack);

#endif
