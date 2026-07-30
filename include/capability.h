#ifndef SHADOWBOX_CAPABILITY_H
#define SHADOWBOX_CAPABILITY_H

#include "types.h"

struct vfs_node;

#define CAP_CHOWN            0
#define CAP_DAC_OVERRIDE     1
#define CAP_DAC_READ_SEARCH   2
#define CAP_FOWNER            3
#define CAP_FSETID            4
#define CAP_KILL              5
#define CAP_SETGID            6
#define CAP_SETUID            7
#define CAP_SETPCAP           8
#define CAP_LINUX_IMMUTABLE   9
#define CAP_NET_BIND_SERVICE  10
#define CAP_NET_BROADCAST     11
#define CAP_NET_ADMIN         12
#define CAP_NET_RAW           13
#define CAP_IPC_LOCK          14
#define CAP_IPC_OWNER         15
#define CAP_SYS_MODULE        16
#define CAP_SYS_RAWIO         17
#define CAP_SYS_CHROOT        18
#define CAP_SYS_PTRACE        19
#define CAP_SYS_PACCT         20
#define CAP_SYS_ADMIN         21
#define CAP_SYS_BOOT          22
#define CAP_SYS_NICE          23
#define CAP_SYS_RESOURCE      24
#define CAP_SYS_TIME          25
#define CAP_SYS_TTY_CONFIG    26
#define CAP_MKNOD             27
#define CAP_LEASE             28
#define CAP_AUDIT_WRITE       29
#define CAP_AUDIT_CONTROL     30
#define CAP_SETFCAP           31
#define CAP_MAC_OVERRIDE      32
#define CAP_MAC_ADMIN         33
#define CAP_SYSLOG            34
#define CAP_WAKE_ALARM        35
#define CAP_BLOCK_SUSPEND     36
#define CAP_AUDIT_READ        37
#define CAP_PERFMON           38
#define CAP_BPF               39
#define CAP_CHECKPOINT_RESTORE 40

#define CAP_LAST_CAP          CAP_CHECKPOINT_RESTORE

/*
 * kernel_cap_t - Capability set (64-bit)
 * @cap: Two 32-bit words forming a 64-bit set
 */
typedef struct {
    uint32_t cap[2];
} kernel_cap_t;

/*
 * security_context_t - Security context for a process
 * @uid, @gid: Real user/group ID
 * @euid, @egid: Effective user/group ID
 * @suid, @sgid: Saved user/group ID
 * @fsuid, @fsgid: Filesystem user/group ID
 * @cap_*: Capability sets
 */
typedef struct security_context {
    uint32_t uid;
    uint32_t gid;
    uint32_t euid;
    uint32_t egid;
    uint32_t suid;
    uint32_t sgid;
    uint32_t fsuid;
    uint32_t fsgid;
    kernel_cap_t cap_inheritable;
    kernel_cap_t cap_permitted;
    kernel_cap_t cap_effective;
    kernel_cap_t cap_bset;
    kernel_cap_t cap_ambient;
} security_context_t;

/*
 * cap_valid - Check if capability number is valid
 * @cap: Capability number
 * Returns: 1 if valid, 0 otherwise
 */
int cap_valid(int cap);

/*
 * cap_raise - Set a capability bit
 * @set: Capability set
 * @cap: Capability to raise
 * Returns: 0 on success, -1 on error
 */
int cap_raise(kernel_cap_t *set, int cap);

/*
 * cap_lower - Clear a capability bit
 * @set: Capability set
 * @cap: Capability to lower
 * Returns: 0 on success, -1 on error
 */
int cap_lower(kernel_cap_t *set, int cap);

/*
 * cap_isclear - Check if a capability is set
 * @set: Capability set
 * @cap: Capability to check
 * Returns: 1 if cleared, 0 if set
 */
int cap_isclear(const kernel_cap_t *set, int cap);

/*
 * cap_isfull - Check if all capabilities are set
 * @set: Capability set
 * Returns: 1 if full, 0 otherwise
 */
int cap_isfull(const kernel_cap_t *set);

/*
 * cap_clear - Clear all capability bits
 * @set: Capability set
 */
void cap_clear(kernel_cap_t *set);

/*
 * cap_set_full - Set all capability bits
 * @set: Capability set
 */
void cap_set_full(kernel_cap_t *set);

/*
 * cap_and - AND two capability sets
 * @dst: Destination set
 * @a:   First source set
 * @b:   Second source set
 */
void cap_and(kernel_cap_t *dst, const kernel_cap_t *a, const kernel_cap_t *b);

/*
 * cap_or - OR two capability sets
 * @dst: Destination set
 * @a:   First source set
 * @b:   Second source set
 */
void cap_or(kernel_cap_t *dst, const kernel_cap_t *a, const kernel_cap_t *b);

/*
 * security_context_init - Initialize a security context
 * @ctx: Security context to initialize
 * Returns: 0 on success, -1 on error
 */
int security_context_init(security_context_t *ctx);

/*
 * security_context_copy - Copy a security context
 * @dst: Destination context
 * @src: Source context
 * Returns: 0 on success, -1 on error
 */
int security_context_copy(security_context_t *dst, const security_context_t *src);

/*
 * security_context_setuid - Set real UID in context
 * @ctx: Security context
 * @uid: User ID to set
 * Returns: 0 on success, -1 on error
 */
int security_context_setuid(security_context_t *ctx, uint32_t uid);

/*
 * security_context_setgid - Set real GID in context
 * @ctx: Security context
 * @gid: Group ID to set
 * Returns: 0 on success, -1 on error
 */
int security_context_setgid(security_context_t *ctx, uint32_t gid);

/*
 * security_context_seteuid - Set effective UID
 * @ctx:  Security context
 * @euid: Effective UID to set
 * Returns: 0 on success, -1 on error
 */
int security_context_seteuid(security_context_t *ctx, uint32_t euid);

/*
 * security_context_setegid - Set effective GID
 * @ctx:  Security context
 * @egid: Effective GID to set
 * Returns: 0 on success, -1 on error
 */
int security_context_setegid(security_context_t *ctx, uint32_t egid);

/*
 * security_context_has_cap - Check capability in context
 * @ctx: Security context
 * @cap: Capability to check
 * Returns: 1 if has capability, 0 otherwise
 */
int security_context_has_cap(const security_context_t *ctx, int cap);

/*
 * inode_permission - Check permission on an inode
 * @inode: Inode to check
 * @mask:  Permission mask
 * Returns: 0 on success, -1 on error
 */
int inode_permission(const struct vfs_node *inode, int mask);

/*
 * file_permission - Check permission against security context
 * @inode: Inode to check
 * @ctx:   Security context
 * @mask:  Permission mask
 * Returns: 0 on success, -1 on error
 */
int file_permission(const struct vfs_node *inode, const security_context_t *ctx, int mask);

/*
 * capable - Check if current process has capability
 * @cap: Capability to check
 * Returns: 1 if capable, 0 otherwise
 */
int capable(int cap);

/*
 * ns_capable - Check capability in a namespace
 * @cap: Capability to check
 * @ns:  Namespace identifier
 * Returns: 1 if capable, 0 otherwise
 */
int ns_capable(int cap, int ns);

#endif
