#ifndef SYS_H
#define SYS_H

typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int64_t;
typedef long ssize_t;
typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef unsigned long uintptr_t;
typedef int int32_t;
typedef short int16_t;
typedef __attribute__((may_alias)) struct { uint64_t tv_sec; uint64_t tv_nsec; } timespec;

#define NULL ((void*)0)

#define SB_PULL_DATA     0
#define SB_PUSH_DATA    1
#define SB_ACQUIRE     2
#define SB_RELEASE    3
#define SYS_BRK      12
#define SYS_GETPID   39
#define SYS_GETPPID  110
#define SB_TERMINATE     60
#define SYS_UNAME    63
#define SYS_TIMES    100
#define SYS_PROC_INFO 101
#define SYS_MEM_INFO 120
#define SYS_KILL     62
#define SYS_SCHED_YIELD 24
#define SB_REPLICATE     57
#define SB_MORPH   59
#define SYS_WAIT4    61
#define SYS_RENAME   82
#define SYS_MKDIR    83
#define SYS_RMDIR    84
#define SYS_UNLINK   87
#define SYS_CHDIR    80
#define SYS_DUP      32
#define SYS_DUP2     33
#define SYS_PIPE     22
#define SYS_LSEEK    8
#define SYS_ACCESS   21
#define SYS_MOUNT    169
#define SYS_UMOUNT2  52
#define SYS_CLIPBOARD_SET 204
#define SYS_CLIPBOARD_GET 205
#define SYS_CONSOLE_PENDING 206
#define SYS_POWER        210
#define SYS_TIMEZONE     211
#define SYS_DNS_RESOLVE  212
#define SYS_NTP_SYNC     213
#define SYS_PING         214
#define SYS_NETINFO      215

// Unified input event struct matching kernel/include/input.h
typedef struct input_event {
    uint8_t type;
    uint8_t code;
    int16_t x;
    int16_t y;
    int16_t value;
} input_event_t;

struct proc_info {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint64_t kstack;
    uint64_t cr3;
};

static inline uint64_t syscall3(uint64_t n, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    register uint64_t rax __asm__("rax") = n;
    register uint64_t rdi __asm__("rdi") = arg1;
    register uint64_t rsi __asm__("rsi") = arg2;
    register uint64_t rdx __asm__("rdx") = arg3;
    __asm__ volatile (
        "syscall"
        : "+r" (rax)
        : "r" (rdi), "r" (rsi), "r" (rdx)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline uint64_t syscall2(uint64_t n, uint64_t arg1, uint64_t arg2) {
    return syscall3(n, arg1, arg2, 0);
}

static inline uint64_t syscall1(uint64_t n, uint64_t arg1) {
    return syscall3(n, arg1, 0, 0);
}

static inline uint64_t sb_push(int fd, const void *buf, uint64_t count) {
    return syscall3(SB_PUSH_DATA, (uint64_t)fd, (uint64_t)buf, count);
}

static inline uint64_t sb_pull(int fd, void *buf, uint64_t count) {
    return syscall3(SB_PULL_DATA, (uint64_t)fd, (uint64_t)buf, count);
}

static inline void sb_terminate(int code) {
    syscall3(SB_TERMINATE, (uint64_t)code, 0, 0);
    while(1);
}

typedef struct sb_msg {
    uint64_t type;
    uint64_t data1;
    uint64_t data2;
    uint64_t data3;
    uint64_t data4;
} sb_msg_t;

#define SB_IPC_CALL           250
#define SB_IPC_REPLY_WAIT     251

// WM IPC Message Types
#define WM_MSG_CREATE_WINDOW  1
#define WM_MSG_WINDOW_CREATED 2
#define WM_MSG_DRAW_RECT      3
#define WM_MSG_DRAW_TEXT      4
#define WM_MSG_FLUSH          5
#define WM_MSG_KEY_EVENT      6
#define WM_MSG_MOUSE_EVENT    7

static inline uint64_t sb_ipc_call(uint32_t target_pid, sb_msg_t *msg) {
    return syscall2(SB_IPC_CALL, (uint64_t)target_pid, (uint64_t)msg);
}

static inline uint64_t sb_ipc_reply_wait(uint32_t reply_pid, sb_msg_t *reply_msg, sb_msg_t *req_msg) {
    return syscall3(SB_IPC_REPLY_WAIT, (uint64_t)reply_pid, (uint64_t)reply_msg, (uint64_t)req_msg);
}


static inline uint64_t sys_getpid(void) {
    return syscall1(SYS_GETPID, 0);
}

static inline uint64_t sys_getppid(void) {
    return syscall1(SYS_GETPPID, 0);
}

static inline uint64_t sys_kill(uint64_t pid, uint64_t sig) {
    return syscall2(SYS_KILL, pid, sig);
}

static inline uint64_t sys_times(uint64_t *buf) {
    return syscall1(SYS_TIMES, (uint64_t)buf);
}

static inline int sys_proc_info(struct proc_info *buf, int max) {
    return (int)syscall2(SYS_PROC_INFO, (uint64_t)buf, (uint64_t)max);
}

static inline uint64_t sys_mem_info(uint64_t *buf) {
    return syscall1(SYS_MEM_INFO, (uint64_t)buf);
}

static inline uint64_t sb_replicate(void) {
    return syscall1(SB_REPLICATE, 0);
}

static inline uint64_t sb_morph(const char *pathname, char *const argv[], char *const envp[]) {
    return syscall3(SB_MORPH, (uint64_t)pathname, (uint64_t)argv, (uint64_t)envp);
}

static inline uint64_t sys_wait4(uint64_t pid, int *wstatus, uint64_t options) {
    return syscall3(SYS_WAIT4, pid, (uint64_t)wstatus, options);
}

static inline int sb_acquire(const char *pathname, int flags) {
    return (int)syscall2(SB_ACQUIRE, (uint64_t)pathname, (uint64_t)flags);
}

static inline int sb_release(int fd) {
    return (int)syscall1(SB_RELEASE, (uint64_t)fd);
}

static inline int sys_mkdir(const char *pathname, int mode) {
    return (int)syscall2(SYS_MKDIR, (uint64_t)pathname, (uint64_t)mode);
}

static inline int sys_rmdir(const char *pathname) {
    return (int)syscall1(SYS_RMDIR, (uint64_t)pathname);
}

static inline int sys_unlink(const char *pathname) {
    return (int)syscall1(SYS_UNLINK, (uint64_t)pathname);
}

static inline int sys_rename(const char *oldpath, const char *newpath) {
    return (int)syscall2(SYS_RENAME, (uint64_t)oldpath, (uint64_t)newpath);
}

static inline int sys_chdir(const char *path) {
    return (int)syscall1(80, (uint64_t)path);
}

static inline int sys_getcwd(char *buf, uint64_t size) {
    return (int)syscall2(79, (uint64_t)buf, size);
}

/*
 * Network / time zone helpers
 */
typedef struct timeval {
    uint64_t tv_sec;
    uint64_t tv_usec;
} timeval_t;

static inline int sys_gettimeofday(timeval_t *tv) {
    return (int)syscall1(96, (uint64_t)tv);
}

static inline int sys_timezone_get(void) {
    return (int)syscall1(SYS_TIMEZONE, 0);
}

static inline int sys_timezone_set(int offset_minutes) {
    return (int)syscall2(SYS_TIMEZONE, 1, (uint64_t)offset_minutes);
}

static inline int sb_resolve(const char *name, uint32_t *ip_out) {
    return (int)syscall2(SYS_DNS_RESOLVE, (uint64_t)name, (uint64_t)ip_out);
}

static inline int sb_ntp_sync(uint32_t server_ip, int64_t *offset_out) {
    return (int)syscall2(SYS_NTP_SYNC, (uint64_t)server_ip, (uint64_t)offset_out);
}

static inline int sb_ping(uint32_t ip, uint32_t timeout_ms) {
    return (int)syscall2(SYS_PING, (uint64_t)ip, (uint64_t)timeout_ms);
}

typedef struct sb_netinfo {
    uint32_t ip;        /* network byte order */
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns;
    uint8_t mac[6];
    uint8_t link;
    char name[16];
} sb_netinfo_t;

static inline int sb_netinfo(sb_netinfo_t *info) {
    return (int)syscall1(SYS_NETINFO, (uint64_t)info);
}

#endif