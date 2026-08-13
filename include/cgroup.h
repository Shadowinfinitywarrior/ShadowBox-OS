#ifndef SHADOWBOX_CGROUP_H
#define SHADOWBOX_CGROUP_H

#include "types.h"
#include "spinlock.h"

// Resource types controlled by cgroup
#define CGROUP_SUBSYS_CPU    0
#define CGROUP_SUBSYS_MEMORY 1
#define CGROUP_SUBSYS_IO     2
#define CGROUP_SUBSYS_PIDS   3

typedef struct cgroup {
    char name[64];
    
    // Resource limits
    uint64_t memory_limit_bytes;
    uint64_t memory_usage_bytes;
    
    uint32_t cpu_shares;
    uint32_t cpu_quota_us;
    uint32_t cpu_period_us;
    
    uint32_t pids_max;
    uint32_t pids_current;
    
    // I/O limits
    uint64_t blkio_weight;
    
    spinlock_t lock;
    struct cgroup *parent;
    struct cgroup *children;
    struct cgroup *sibling;
} cgroup_t;

void cgroup_init(void);
cgroup_t* cgroup_create(cgroup_t *parent, const char *name);
int cgroup_attach_task(cgroup_t *cg, struct process *p);

#endif
