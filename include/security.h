#ifndef SHADOWBOX_SECURITY_H
#define SHADOWBOX_SECURITY_H

#include "types.h"
#include "capability.h"

struct vfs_node;
struct process;

#define SECURITY_LSM_NAME_MAX 32
#define SECURITY_MAX_LSM_MODULES 16
#define SECURITY_MAX_SANDBOXES 64
#define SECURITY_LABEL_MAX 64
#define SEC_AUDIT_BUFFER_SIZE 256

#define SANDBOX_FLAG_NETWORK     0x0001
#define SANDBOX_FLAG_FILESYSTEM  0x0002
#define SANDBOX_FLAG_IPC         0x0004
#define SANDBOX_FLAG_RAWIO       0x0008
#define SANDBOX_FLAG_SYSCTL      0x0010
#define SANDBOX_FLAG_PRIVILEGED  0x0020

enum lsm_hook_type {
    LSM_HOOK_FILE_PERMISSION = 0,
    LSM_HOOK_TASK_CREATE,
    LSM_HOOK_TASK_KILL,
    LSM_HOOK_INODE_CREATE,
    LSM_HOOK_INODE_LINK,
    LSM_HOOK_INODE_UNLINK,
    LSM_HOOK_IPC_PERMISSION,
    LSM_HOOK_NET_SEND,
    LSM_HOOK_CAPABLE,
    LSM_HOOK_SANDBOX_ENTER,
    LSM_HOOK_COUNT
};

enum lsm_return {
    LSM_CONTINUE = 0,
    LSM_ALLOW,
    LSM_DENY,
    LSM_ERROR
};

typedef int (*lsm_hook_t)(struct process *proc, void *args);

typedef struct lsm_module {
    char name[SECURITY_LSM_NAME_MAX];
    lsm_hook_t hooks[LSM_HOOK_COUNT];
    int enabled;
} lsm_module_t;

typedef struct sandbox {
    int id;
    char name[SECURITY_LABEL_MAX];
    uint32_t flags;
    kernel_cap_t cap_bound;
    uint64_t restricted_syscalls[4];
    int refcount;
} sandbox_t;

typedef struct audit_entry {
    uint64_t timestamp;
    int pid;
    int event_type;
    int result;
    char detail[80];
} audit_entry_t;

typedef struct security_label {
    char user[16];
    char role[16];
    char type[16];
    char level[16];
} security_label_t;

void security_init(void);

int lsm_register(lsm_module_t *mod);
int lsm_unregister(lsm_module_t *mod);
int lsm_hook_invoke(enum lsm_hook_type hook, struct process *proc, void *args);

int sandbox_create(const char *name, uint32_t flags, kernel_cap_t *cap_bound);
int sandbox_attach(int sandbox_id, struct process *proc);
int sandbox_detach(struct process *proc);
sandbox_t *sandbox_get(int sandbox_id);
int sandbox_destroy(int sandbox_id);
int sandbox_check(struct process *proc, int syscall_num);

void audit_log(int pid, int event_type, int result, const char *detail);
int audit_get_entries(audit_entry_t *buf, int max);

void sec_label_init(security_label_t *label, const char *user, const char *role, const char *type, const char *level);
int sec_label_transition(security_label_t *new, const security_label_t *old, const security_label_t *file);

int process_security_init(struct process *proc);
void process_security_fork(struct process *parent, struct process *child);
int process_security_check_cap(struct process *proc, int cap);
int process_security_check_kill(struct process *proc, struct process *target);

#endif
