#include "kernel.h"
#include "vfs.h"
#include "block.h"
#include "errno.h"

/*
 * Minimal stub for EXT4 filesystem support.
 * This file provides only placeholder definitions so that the kernel
 * build succeeds when the EXT4 subsystem is referenced. No functional
 * implementation is provided.
 */

/* Example stub initialization function */
void ext4_init(void) {
    printk(KERN_INFO "EXT4: stub filesystem initialized\n");
}

/* Stub mount function – always fails with ENOSYS (function not implemented) */
int ext4_mount(vfs_node_t *parent, const char *device) {
    (void)parent; (void)device; /* suppress unused warnings */
    printk(KERN_WARN "EXT4: mount stub not implemented\n");
    return -ENOSYS;
}

/* Stub read/write – return 0 bytes / error */
uint32_t ext4_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return 0;
}

uint32_t ext4_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return 0;
}

/* Stub readdir – always returns NULL */
struct dirent *ext4_readdir(vfs_node_t *node, uint32_t index) {
    (void)node; (void)index;
    return NULL;
}

/* Stub functions to satisfy potential references */
static int ext4_create_file(vfs_node_t *dir, const char *name, uint32_t flags) {
    (void)dir; (void)name; (void)flags;
    return -ENOSYS;
}

static int ext4_unlink(vfs_node_t *dir, const char *name) {
    (void)dir; (void)name;
    return -ENOSYS;
}

/* End of stub implementation */
