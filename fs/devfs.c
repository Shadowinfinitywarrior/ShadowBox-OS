#include "devfs.h"
#include "kernel.h"
#include "vfs.h"
#include "malloc.h"
#include "kstring.h"
#include "keyboard.h"

vfs_node_t *devfs_root;

static uint32_t dev_null_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return 0; // EOF
}

static uint32_t dev_null_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)buffer;
    return size; // Discard
}

static uint32_t dev_zero_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset;
    memset(buffer, 0, size);
    return size;
}

static uint32_t dev_zero_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)buffer;
    return size; // Discard
}

static uint32_t dev_input_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset;
    if (size < sizeof(input_event_t)) return 0;
    input_event_t ev;
    if (!input_poll_event(&ev)) return 0;
    memcpy(buffer, &ev, sizeof(input_event_t));
    return sizeof(input_event_t);
}

void devfs_register_input(void) {
    if (!devfs_root) return;
    printk(KERN_INFO "DEVFS: Registered /dev/input\n");
}

static vfs_node_t *devfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    if (strcmp(name, "null") == 0) {
        vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "null");
        n->flags = FS_CHARDEVICE;
        n->read = dev_null_read;
        n->write = dev_null_write;
        return n;
    }
    if (strcmp(name, "zero") == 0) {
        vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "zero");
        n->flags = FS_CHARDEVICE;
        n->read = dev_zero_read;
        n->write = dev_zero_write;
        return n;
    }
    if (strcmp(name, "input") == 0) {
        vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "input");
        n->flags = FS_CHARDEVICE;
        n->read = dev_input_read;
        return n;
    }
    return NULL;
}

static struct dirent *devfs_readdir(vfs_node_t *node, uint32_t index) {
    (void)node;
    const char *entries[] = {"null", "zero", "input"};
    if (index >= 3) return NULL;
    struct dirent *d = kmalloc(sizeof(struct dirent));
    if (!d) return NULL;
    memset(d, 0, sizeof(struct dirent));
    strcpy(d->name, entries[index]);
    d->ino = index + 1;
    return d;
}

void devfs_init(void) {
    printk(KERN_INFO "DEVFS: Initializing /dev pseudo-filesystem...\n");
    
    devfs_root = kmalloc(sizeof(vfs_node_t));
    if (!devfs_root) return;
    memset(devfs_root, 0, sizeof(vfs_node_t));
    strcpy(devfs_root->name, "dev");
    devfs_root->flags = FS_DIRECTORY;
    devfs_root->finddir_func = devfs_finddir;
    devfs_root->readdir_func = devfs_readdir;
}
