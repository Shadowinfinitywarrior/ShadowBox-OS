#include "security.h"
#include "kernel.h"
#include "task.h"
#include "errno.h"
#include "spinlock.h"
#include "kstring.h"
#include "malloc.h"

static lsm_module_t *lsm_modules[SECURITY_MAX_LSM_MODULES];
static int lsm_module_count;
static spinlock_t lsm_lock;

static sandbox_t *sandboxes[SECURITY_MAX_SANDBOXES];
static int sandbox_count;
static int sandbox_next_id;
static spinlock_t sandbox_lock;

static audit_entry_t audit_buffer[SEC_AUDIT_BUFFER_SIZE];
static int audit_head;
static int audit_count;
static spinlock_t audit_lock;

int cap_valid(int cap) {
    return (cap >= 0 && cap <= CAP_LAST_CAP) ? 1 : 0;
}

int cap_raise(kernel_cap_t *set, int cap) {
    if (!cap_valid(cap)) return -EINVAL;
    if (cap < 32) set->cap[0] |= (1U << cap);
    else set->cap[1] |= (1U << (cap - 32));
    return 0;
}

int cap_lower(kernel_cap_t *set, int cap) {
    if (!cap_valid(cap)) return -EINVAL;
    if (cap < 32) set->cap[0] &= ~(1U << cap);
    else set->cap[1] &= ~(1U << (cap - 32));
    return 0;
}

int cap_isclear(const kernel_cap_t *set, int cap) {
    if (!cap_valid(cap)) return 1;
    if (cap < 32) return (set->cap[0] & (1U << cap)) == 0;
    return (set->cap[1] & (1U << (cap - 32))) == 0;
}

int cap_isfull(const kernel_cap_t *set) {
    return (set->cap[0] == ~0U && set->cap[1] == ~0U) ? 1 : 0;
}

void cap_clear(kernel_cap_t *set) {
    set->cap[0] = 0;
    set->cap[1] = 0;
}

void cap_set_full(kernel_cap_t *set) {
    set->cap[0] = ~0U;
    set->cap[1] = ~0U;
}

void cap_and(kernel_cap_t *dst, const kernel_cap_t *a, const kernel_cap_t *b) {
    dst->cap[0] = a->cap[0] & b->cap[0];
    dst->cap[1] = a->cap[1] & b->cap[1];
}

void cap_or(kernel_cap_t *dst, const kernel_cap_t *a, const kernel_cap_t *b) {
    dst->cap[0] = a->cap[0] | b->cap[0];
    dst->cap[1] = a->cap[1] | b->cap[1];
}

int security_context_init(security_context_t *ctx) {
    if (!ctx) return -EINVAL;
    ctx->uid = 0;
    ctx->gid = 0;
    ctx->euid = 0;
    ctx->egid = 0;
    ctx->suid = 0;
    ctx->sgid = 0;
    ctx->fsuid = 0;
    ctx->fsgid = 0;
    cap_clear(&ctx->cap_inheritable);
    cap_clear(&ctx->cap_permitted);
    cap_clear(&ctx->cap_effective);
    cap_clear(&ctx->cap_bset);
    cap_clear(&ctx->cap_ambient);
    return 0;
}

int security_context_copy(security_context_t *dst, const security_context_t *src) {
    if (!dst || !src) return -EINVAL;
    *dst = *src;
    return 0;
}

int security_context_setuid(security_context_t *ctx, uint32_t uid) {
    if (!ctx) return -EINVAL;
    ctx->uid = uid;
    return 0;
}

int security_context_setgid(security_context_t *ctx, uint32_t gid) {
    if (!ctx) return -EINVAL;
    ctx->gid = gid;
    return 0;
}

int security_context_seteuid(security_context_t *ctx, uint32_t euid) {
    if (!ctx) return -EINVAL;
    ctx->euid = euid;
    ctx->fsuid = euid;
    return 0;
}

int security_context_setegid(security_context_t *ctx, uint32_t egid) {
    if (!ctx) return -EINVAL;
    ctx->egid = egid;
    ctx->fsgid = egid;
    return 0;
}

int security_context_has_cap(const security_context_t *ctx, int cap) {
    if (!ctx || !cap_valid(cap)) return 0;
    if (cap < 32)
        return (ctx->cap_effective.cap[0] & (1U << cap)) != 0;
    return (ctx->cap_effective.cap[1] & (1U << (cap - 32))) != 0;
}

int inode_permission(const struct vfs_node *inode, int mask) {
    (void)inode;
    (void)mask;
    return 0;
}

int file_permission(const struct vfs_node *inode, const security_context_t *ctx, int mask) {
    int ret = inode_permission(inode, mask);
    if (ret < 0) return ret;
    ret = lsm_hook_invoke(LSM_HOOK_FILE_PERMISSION, get_current_process(), (void *)(uint64_t)mask);
    if (ret == LSM_DENY) return -EACCES;
    (void)ctx;
    return 0;
}

int capable(int cap) {
    struct process *proc = get_current_process();
    if (!proc) return 0;
    return process_security_check_cap(proc, cap);
}

int ns_capable(int cap, int ns) {
    (void)ns;
    return capable(cap);
}

void sec_label_init(security_label_t *label, const char *user, const char *role, const char *type, const char *level) {
    if (!label) return;
    strncpy(label->user, user ? user : "system_u", sizeof(label->user));
    strncpy(label->role, role ? role : "object_r", sizeof(label->role));
    strncpy(label->type, type ? type : "unlabeled_t", sizeof(label->type));
    strncpy(label->level, level ? level : "s0", sizeof(label->level));
}

int sec_label_transition(security_label_t *new, const security_label_t *old, const security_label_t *file) {
    if (!new || !old) return -EINVAL;
    if (file) {
        strncpy(new->user, file->user, sizeof(new->user));
        strncpy(new->role, file->role, sizeof(new->role));
        strncpy(new->type, file->type, sizeof(new->type));
        strncpy(new->level, old->level, sizeof(new->level));
    } else {
        *new = *old;
    }
    return 0;
}

static int default_lsm_capable(struct process *proc, void *args) {
    (void)proc;
    (void)args;
    return LSM_CONTINUE;
}

static lsm_module_t default_cap_lsm = {
    .name = "capability",
    .hooks = {
        [LSM_HOOK_CAPABLE] = default_lsm_capable,
    },
    .enabled = 1,
};

void security_init(void) {
    spinlock_init(&lsm_lock);
    spinlock_init(&sandbox_lock);
    spinlock_init(&audit_lock);

    for (int i = 0; i < SECURITY_MAX_LSM_MODULES; i++)
        lsm_modules[i] = NULL;
    lsm_module_count = 0;

    for (int i = 0; i < SECURITY_MAX_SANDBOXES; i++)
        sandboxes[i] = NULL;
    sandbox_count = 0;
    sandbox_next_id = 1;

    audit_head = 0;
    audit_count = 0;

    lsm_register(&default_cap_lsm);

    printk(KERN_INFO "Security: Capability-based security subsystem initialized\n");
}

int lsm_register(lsm_module_t *mod) {
    if (!mod || lsm_module_count >= SECURITY_MAX_LSM_MODULES) return -ENOMEM;
    spin_lock(&lsm_lock);
    lsm_modules[lsm_module_count++] = mod;
    spin_unlock(&lsm_lock);
    printk(KERN_DEBUG "Security: LSM module '%s' registered\n", mod->name);
    return 0;
}

int lsm_unregister(lsm_module_t *mod) {
    if (!mod) return -EINVAL;
    spin_lock(&lsm_lock);
    for (int i = 0; i < lsm_module_count; i++) {
        if (lsm_modules[i] == mod) {
            lsm_modules[i] = lsm_modules[--lsm_module_count];
            lsm_modules[lsm_module_count] = NULL;
            spin_unlock(&lsm_lock);
            return 0;
        }
    }
    spin_unlock(&lsm_lock);
    return -ENOENT;
}

int lsm_hook_invoke(enum lsm_hook_type hook, struct process *proc, void *args) {
    spin_lock(&lsm_lock);
    for (int i = 0; i < lsm_module_count; i++) {
        lsm_module_t *mod = lsm_modules[i];
        if (!mod || !mod->enabled) continue;
        if (mod->hooks[hook]) {
            int ret = mod->hooks[hook](proc, args);
            if (ret == LSM_DENY || ret == LSM_ERROR) {
                spin_unlock(&lsm_lock);
                audit_log(proc ? (int)proc->pid : -1, hook, ret, "LSM denied");
                return ret;
            }
        }
    }
    spin_unlock(&lsm_lock);
    return LSM_ALLOW;
}

int sandbox_create(const char *name, uint32_t flags, kernel_cap_t *cap_bound) {
    spin_lock(&sandbox_lock);
    if (sandbox_count >= SECURITY_MAX_SANDBOXES) {
        spin_unlock(&sandbox_lock);
        return -ENOMEM;
    }
    sandbox_t *sb = kmalloc(sizeof(sandbox_t));
    if (!sb) {
        spin_unlock(&sandbox_lock);
        return -ENOMEM;
    }
    sb->id = sandbox_next_id++;
    strncpy(sb->name, name ? name : "unnamed", sizeof(sb->name));
    sb->flags = flags;
    if (cap_bound)
        sb->cap_bound = *cap_bound;
    else
        cap_set_full(&sb->cap_bound);
    for (int i = 0; i < 4; i++)
        sb->restricted_syscalls[i] = 0;
    sb->refcount = 0;
    sandboxes[sb->id % SECURITY_MAX_SANDBOXES] = sb;
    sandbox_count++;
    spin_unlock(&sandbox_lock);
    return sb->id;
}

int sandbox_attach(int sandbox_id, struct process *proc) {
    if (!proc) return -EINVAL;
    spin_lock(&sandbox_lock);
    sandbox_t *sb = sandboxes[sandbox_id % SECURITY_MAX_SANDBOXES];
    if (!sb || sb->id != sandbox_id) {
        spin_unlock(&sandbox_lock);
        return -ENOENT;
    }
    sb->refcount++;
    spin_unlock(&sandbox_lock);
    lsm_hook_invoke(LSM_HOOK_SANDBOX_ENTER, proc, &sandbox_id);
    return 0;
}

int sandbox_detach(struct process *proc) {
    if (!proc) return -EINVAL;
    (void)proc;
    return 0;
}

sandbox_t *sandbox_get(int sandbox_id) {
    spin_lock(&sandbox_lock);
    sandbox_t *sb = sandboxes[sandbox_id % SECURITY_MAX_SANDBOXES];
    if (!sb || sb->id != sandbox_id) {
        spin_unlock(&sandbox_lock);
        return NULL;
    }
    spin_unlock(&sandbox_lock);
    return sb;
}

int sandbox_destroy(int sandbox_id) {
    spin_lock(&sandbox_lock);
    sandbox_t *sb = sandboxes[sandbox_id % SECURITY_MAX_SANDBOXES];
    if (!sb || sb->id != sandbox_id) {
        spin_unlock(&sandbox_lock);
        return -ENOENT;
    }
    sandboxes[sandbox_id % SECURITY_MAX_SANDBOXES] = NULL;
    sandbox_count--;
    spin_unlock(&sandbox_lock);
    kfree(sb);
    return 0;
}

int sandbox_check(struct process *proc, int syscall_num) {
    if (!proc) return 0;
    (void)proc;
    (void)syscall_num;
    return 0;
}

void audit_log(int pid, int event_type, int result, const char *detail) {
    spin_lock(&audit_lock);
    audit_entry_t *entry = &audit_buffer[audit_head];
    entry->timestamp = 0;
    entry->pid = pid;
    entry->event_type = event_type;
    entry->result = result;
    strncpy(entry->detail, detail ? detail : "", sizeof(entry->detail));
    audit_head = (audit_head + 1) % SEC_AUDIT_BUFFER_SIZE;
    if (audit_count < SEC_AUDIT_BUFFER_SIZE) audit_count++;
    spin_unlock(&audit_lock);
}

int audit_get_entries(audit_entry_t *buf, int max) {
    if (!buf || max <= 0) return -EINVAL;
    spin_lock(&audit_lock);
    int to_copy = (audit_count < max) ? audit_count : max;
    int start = (audit_head - audit_count + SEC_AUDIT_BUFFER_SIZE) % SEC_AUDIT_BUFFER_SIZE;
    for (int i = 0; i < to_copy; i++) {
        buf[i] = audit_buffer[(start + i) % SEC_AUDIT_BUFFER_SIZE];
    }
    spin_unlock(&audit_lock);
    return to_copy;
}

int process_security_init(struct process *proc) {
    if (!proc) return -EINVAL;
    security_context_t tmp;
    security_context_init(&tmp);
    cap_set_full(&tmp.cap_permitted);
    cap_set_full(&tmp.cap_effective);
    cap_set_full(&tmp.cap_bset);
    proc->uid = tmp.uid;
    proc->gid = tmp.gid;
    proc->euid = tmp.euid;
    proc->egid = tmp.egid;
    return 0;
}

void process_security_fork(struct process *parent, struct process *child) {
    if (!parent || !child) return;
    child->uid = parent->uid;
    child->gid = parent->gid;
    child->euid = parent->euid;
    child->egid = parent->egid;
}

int process_security_check_cap(struct process *proc, int cap) {
    if (!proc || !cap_valid(cap)) return 0;
    if (proc->euid == 0) return 1;
    int lsm_ret = lsm_hook_invoke(LSM_HOOK_CAPABLE, proc, &cap);
    if (lsm_ret == LSM_DENY) return 0;
    return 0;
}

int process_security_check_kill(struct process *proc, struct process *target) {
    if (!proc || !target) return -EINVAL;
    if (proc->euid == 0) return 0;
    if (proc->euid == target->euid) return 0;
    if (process_security_check_cap(proc, CAP_KILL)) return 0;
    audit_log(proc->pid, LSM_HOOK_TASK_KILL, LSM_DENY, "kill denied: no CAP_KILL");
    return -EPERM;
}
