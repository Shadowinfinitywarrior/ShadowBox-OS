/*
 * Minimal NUMA (Non-Uniform Memory Access) stub implementation.
 *
 * This file provides placeholder functions so that the kernel builds even
 * though NUMA support is not required for the current project. The functions
 * deliberately contain no real logic – they simply satisfy the compiler.
 */

#include <stddef.h>
#include <stdint.h>
#include "malloc.h"

/* Initialise NUMA subsystem. Returns 0 on success. */
int numa_init(void) {
    return 0;
}

/* Allocate `size` bytes on a specific NUMA node.
 * The `node` argument is ignored in this stub. Returns a pointer to a
 * zero‑filled block using the kernel's basic allocator if available, or
 * NULL if allocation fails. For simplicity we use a weak reference to the
 * standard C `malloc`; in the real kernel this would be replaced by a proper
 * allocator.
 */
void *numa_alloc_onnode(size_t size, int node) {
    (void)node;               /* suppress unused‑parameter warning */
    return kmalloc(size);
}

/* Free memory allocated by `numa_alloc_onnode`. */
void numa_free(void *ptr) {
    kfree(ptr);
}

/* Query the number of NUMA nodes. Stub returns 1 indicating a single node. */
int numa_num_nodes(void) {
    return 1;
}

/* Get the current node for the calling CPU. Stub returns 0. */
int numa_get_node(void) {
    return 0;
}
