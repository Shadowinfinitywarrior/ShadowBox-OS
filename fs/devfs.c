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
    for (uint32_t i = 0; i < size; i++) buffer[i] = 0;
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

static vfs_node_t *devfs_finddir(vfs_node_t *node, char *name) {
    (void)node;
    if (strcmp(name, "null") == 0) {
        vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
        for (uint64_t i = 0; i < sizeof(vfs_node_t); i++) ((char*)n)[i] = 0;
        strcpy(n->name, "null");
        n->flags = FS_CHARDEVICE;
        n->read = dev_null_read;
        n->write = dev_null_write;
        return n;
    }
    if (strcmp(name, "zero") == 0) {
        vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
        for (uint64_t i = 0; i < sizeof(vfs_node_t); i++) ((char*)n)[i] = 0;
        strcpy(n->name, "zero");
        n->flags = FS_CHARDEVICE;
        n->read = dev_zero_read;
        n->write = dev_zero_write;
        return n;
    }
    if (strcmp(name, "input") == 0) {
        vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
        for (uint64_t i = 0; i < sizeof(vfs_node_t); i++) ((char*)n)[i] = 0;
        strcpy(n->name, "input");
        n->flags = FS_CHARDEVICE;
        n->read = dev_input_read;
        return n;
    }
    return 0;
}

void devfs_init(void) {
    printk("DEVFS: Initializing /dev pseudo-filesystem...\n");
    
    devfs_root = kmalloc(sizeof(vfs_node_t));
    for (uint64_t i = 0; i < sizeof(vfs_node_t); i++) ((char*)devfs_root)[i] = 0;
    strcpy(devfs_root->name, "dev");
    devfs_root->flags = FS_DIRECTORY;
    devfs_root->finddir_func = devfs_finddir;
    // Note: /dev/input is created on-demand by devfs_finddir()
}
