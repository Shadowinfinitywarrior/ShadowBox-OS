#include "shadowfs.h"
#include "vfs.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"

vfs_node_t *shadowfs_root = NULL;

static uint32_t shadowfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return 0; // No data
}

static uint32_t shadowfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)buffer;
    // Discard data but report full write for compatibility
    return size;
}

static vfs_node_t *shadowfs_finddir(vfs_node_t *node, const char *name) {
    (void)node; (void)name;
    return NULL; // No entries
}

static struct dirent *shadowfs_readdir(vfs_node_t *node, uint32_t index) {
    (void)node; (void)index;
    return NULL; // Empty directory
}

void shadowfs_init(void) {
    printk(KERN_INFO "ShadowFS: initializing...\n");
    shadowfs_root = kmalloc(sizeof(vfs_node_t));
    if (!shadowfs_root) {
        printk(KERN_ERR "ShadowFS: failed to allocate root node\n");
        return;
    }
    memset(shadowfs_root, 0, sizeof(vfs_node_t));
    strcpy(shadowfs_root->name, "shadow");
    shadowfs_root->flags = FS_DIRECTORY;
    shadowfs_root->finddir_func = (void*)shadowfs_finddir;
    shadowfs_root->readdir_func = (void*)shadowfs_readdir;
    // Optionally mount at /shadow (handled by caller)
}
