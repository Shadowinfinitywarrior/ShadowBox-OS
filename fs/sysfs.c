#include "sysfs.h"
#include "kernel.h"
#include "vfs.h"
#include "malloc.h"
#include "kstring.h"

vfs_node_t *sysfs_root;

static uint32_t sysfs_class_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    const char *data =
        "net\n"
        "block\n"
        "input\n"
        "drm\n"
        "sound\n"
        "misc\n";
    uint32_t len = 0;
    while (data[len]) len++;
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    for (uint32_t i = 0; i < size; i++) buffer[i] = data[offset + i];
    return size;
}

static uint32_t sysfs_devices_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    const char *data =
        "system\n"
        "virtual\n"
        "pci0000:00\n";
    uint32_t len = 0;
    while (data[len]) len++;
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    for (uint32_t i = 0; i < size; i++) buffer[i] = data[offset + i];
    return size;
}

static uint32_t sysfs_power_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    const char *data = "state: mem\n";
    uint32_t len = 0;
    while (data[len]) len++;
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    for (uint32_t i = 0; i < size; i++) buffer[i] = data[offset + i];
    return size;
}

static vfs_node_t *sysfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    vfs_node_t *n;
    if (strcmp(name, "class") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "class");
        n->flags = FS_FILE;
        n->read = sysfs_class_read;
        n->length = 64;
        return n;
    }
    if (strcmp(name, "devices") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "devices");
        n->flags = FS_FILE;
        n->read = sysfs_devices_read;
        n->length = 64;
        return n;
    }
    if (strcmp(name, "power") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "power");
        n->flags = FS_FILE;
        n->read = sysfs_power_read;
        n->length = 32;
        return n;
    }
    if (strcmp(name, "kernel") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "kernel");
        n->flags = FS_DIRECTORY;
        n->finddir_func = sysfs_finddir;
        return n;
    }
    return NULL;
}

static struct dirent *sysfs_readdir(vfs_node_t *node, uint32_t index) {
    (void)node;
    static const char *entries[] = {"class", "devices", "power", "kernel", 0};
    if (index >= 4) return NULL;
    struct dirent *d = kmalloc(sizeof(struct dirent));
    if (!d) return NULL;
    memset(d, 0, sizeof(struct dirent));
    strcpy(d->name, entries[index]);
    d->ino = index + 1;
    return d;
}

void sysfs_init(void) {
    printk(KERN_INFO "SYSFS: Initializing /sys virtual filesystem...\n");
    sysfs_root = kmalloc(sizeof(vfs_node_t));
    if (!sysfs_root) return;
    memset(sysfs_root, 0, sizeof(vfs_node_t));
    strcpy(sysfs_root->name, "sys");
    sysfs_root->flags = FS_DIRECTORY;
    sysfs_root->finddir_func = sysfs_finddir;
    sysfs_root->readdir_func = sysfs_readdir;
}
