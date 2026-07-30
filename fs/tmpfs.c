#include "vfs.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"
#include "errno.h"

#define TMPFS_MAX_ENTRIES 256
#define TMPFS_MAX_NAME    128
#define TMPFS_MAX_FILESIZE (1024 * 1024) // 1MB max per file

struct tmpfs_entry {
    char name[TMPFS_MAX_NAME];
    uint32_t flags;        // FS_FILE or FS_DIRECTORY
    uint8_t *data;
    uint32_t length;
    uint32_t capacity;
    struct tmpfs_entry *parent;
    struct tmpfs_entry *children[TMPFS_MAX_ENTRIES];
    int child_count;
    uint32_t inode;
};

// Forward declarations for circular references
static vfs_node_t *tmpfs_finddir(vfs_node_t *node, const char *name);
static struct dirent *tmpfs_readdir(vfs_node_t *node, uint32_t index);
vfs_node_t *tmpfs_finddir_func(vfs_node_t *node, const char *name);
struct dirent *tmpfs_readdir_func(vfs_node_t *node, uint32_t index);

static uint32_t tmpfs_next_inode = 10000;

static struct tmpfs_entry *tmpfs_create_entry(const char *name, uint32_t flags, struct tmpfs_entry *parent) {
    struct tmpfs_entry *e = kmalloc(sizeof(struct tmpfs_entry));
    if (!e) return NULL;
    memset(e, 0, sizeof(struct tmpfs_entry));
    int i = 0;
    while (name[i] && i < TMPFS_MAX_NAME - 1) { e->name[i] = name[i]; i++; }
    e->name[i] = 0;
    e->flags = flags;
    e->parent = parent;
    e->inode = tmpfs_next_inode++;
    if (flags & FS_FILE) {
        e->capacity = 4096;
        e->data = kmalloc(e->capacity);
        if (e->data) memset(e->data, 0, e->capacity);
    }
    return e;
}

// Read from a tmpfs file
static uint32_t tmpfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    struct tmpfs_entry *e = (struct tmpfs_entry *)node->impl;
    if (!e || !e->data) return 0;
    if (offset >= e->length) return 0;
    if (offset + size > e->length) size = e->length - offset;
    for (uint32_t i = 0; i < size; i++) buffer[i] = e->data[offset + i];
    return size;
}

// Write to a tmpfs file
static uint32_t tmpfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    struct tmpfs_entry *e = (struct tmpfs_entry *)node->impl;
    if (!e) return 0;
        if (!e->data) {
            e->capacity = 4096;
            e->data = kmalloc(e->capacity);
            if (!e->data) return 0;
            memset(e->data, 0, e->capacity);
        }
    // Grow if needed
    while (offset + size > e->capacity) {
        uint32_t new_cap = e->capacity * 2;
        if (new_cap > TMPFS_MAX_FILESIZE) new_cap = TMPFS_MAX_FILESIZE;
        if (new_cap == e->capacity) break;
        uint8_t *new_data = kmalloc(new_cap);
        if (!new_data) break;
        memset(new_data, 0, new_cap);
        for (uint32_t j = 0; j < e->length; j++) new_data[j] = e->data[j];
        kfree(e->data);
        e->data = new_data;
        e->capacity = new_cap;
    }
    for (uint32_t i = 0; i < size; i++) e->data[offset + i] = buffer[i];
    if (offset + size > e->length) e->length = offset + size;
    node->length = e->length;
    return size;
}

// Find a child entry by name
static struct tmpfs_entry *tmpfs_find_child(struct tmpfs_entry *dir, const char *name);
static struct tmpfs_entry *tmpfs_mkdir_entry(struct tmpfs_entry *dir, const char *name);
static struct tmpfs_entry *tmpfs_create_file_entry(struct tmpfs_entry *dir, const char *name);
static int tmpfs_remove_entry(struct tmpfs_entry *dir, const char *name);

vfs_node_t *tmpfs_mkdir_func(vfs_node_t *dir, const char *name);
vfs_node_t *tmpfs_create_func(vfs_node_t *dir, const char *name, uint32_t mode);
int tmpfs_rmdir_func(vfs_node_t *dir, const char *name);
int tmpfs_unlink_func(vfs_node_t *dir, const char *name);

static struct tmpfs_entry *tmpfs_find_child(struct tmpfs_entry *dir, const char *name) {
    if (!dir || !(dir->flags & FS_DIRECTORY)) return 0;
    for (int i = 0; i < dir->child_count; i++) {
        if (dir->children[i]) {
            int match = 1;
            for (int j = 0; dir->children[i]->name[j] || name[j]; j++) {
                if (dir->children[i]->name[j] != name[j]) { match = 0; break; }
            }
            if (match) return dir->children[i];
        }
    }
    return 0;
}

// Create a VFS node from a tmpfs entry
static vfs_node_t *tmpfs_make_vfs_node(struct tmpfs_entry *e) {
    if (!e) return 0;
    vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
    if (!n) return NULL;
    memset(n, 0, sizeof(vfs_node_t));
    int i = 0;
    while (e->name[i] && i < 127) { n->name[i] = e->name[i]; i++; }
    n->name[i] = 0;
    n->flags = e->flags;
    n->inode = e->inode;
    n->length = e->length;
    n->impl = (uint64_t)e;
    n->refcount = 1;
    if (e->flags & FS_FILE) {
        n->read = tmpfs_read;
        n->write = tmpfs_write;
    }
    if (e->flags & FS_DIRECTORY) {
        n->finddir_func = (void*)tmpfs_finddir_func;
        n->readdir_func = (void*)tmpfs_readdir_func;
        n->mkdir_func = (void*)tmpfs_mkdir_func;
        n->rmdir_func = (void*)tmpfs_rmdir_func;
        n->unlink_func = (void*)tmpfs_unlink_func;
        n->create_func = (void*)tmpfs_create_func;
    }
    return n;
}

vfs_node_t *tmpfs_create_func(vfs_node_t *dir, const char *name, uint32_t mode) {
    (void)mode;
    if (!dir) return NULL;
    struct tmpfs_entry *d = (struct tmpfs_entry *)dir->impl;
    struct tmpfs_entry *e = tmpfs_create_file_entry(d, name);
    if (!e) return NULL;
    return tmpfs_make_vfs_node(e);
}

vfs_node_t *tmpfs_mkdir_func(vfs_node_t *dir, const char *name) {
    if (!dir) return NULL;
    struct tmpfs_entry *d = (struct tmpfs_entry *)dir->impl;
    struct tmpfs_entry *e = tmpfs_mkdir_entry(d, name);
    if (!e) return NULL;
    return tmpfs_make_vfs_node(e);
}

int tmpfs_rmdir_func(vfs_node_t *dir, const char *name) {
    if (!dir) return -EIO;
    struct tmpfs_entry *d = (struct tmpfs_entry *)dir->impl;
    struct tmpfs_entry *e = tmpfs_find_child(d, name);
    if (!e || !(e->flags & FS_DIRECTORY)) return -ENOENT;
    if (e->child_count > 0) return -ENOTEMPTY;
    return tmpfs_remove_entry(d, name);
}

int tmpfs_unlink_func(vfs_node_t *dir, const char *name) {
    if (!dir) return -EIO;
    struct tmpfs_entry *d = (struct tmpfs_entry *)dir->impl;
    struct tmpfs_entry *e = tmpfs_find_child(d, name);
    if (!e || (e->flags & FS_DIRECTORY)) return -ENOENT;
    return tmpfs_remove_entry(d, name);
}

// finddir for tmpfs
static vfs_node_t *tmpfs_finddir(vfs_node_t *node, const char *name) {
    struct tmpfs_entry *dir = (struct tmpfs_entry *)node->impl;
    if (!dir) return NULL;
    if (name[0] == '.' && name[1] == '.' && name[2] == 0) {
        if (dir->parent) return tmpfs_make_vfs_node(dir->parent);
        return tmpfs_make_vfs_node(dir);
    }
    struct tmpfs_entry *child = tmpfs_find_child(dir, name);
    if (!child) return NULL;
    return tmpfs_make_vfs_node(child);
}

// readdir for tmpfs
static struct dirent *tmpfs_readdir(vfs_node_t *node, uint32_t index) {
    struct tmpfs_entry *dir = (struct tmpfs_entry *)node->impl;
    if (!dir || index >= (uint32_t)dir->child_count) return NULL;
    struct tmpfs_entry *child = dir->children[index];
    if (!child) return NULL;
    struct dirent *d = kmalloc(sizeof(struct dirent));
    if (!d) return NULL;
    memset(d, 0, sizeof(struct dirent));
    int i = 0;
    while (child->name[i] && i < 127) { d->name[i] = child->name[i]; i++; }
    d->name[i] = 0;
    d->ino = child->inode;
    return d;
}

// Public API called via function pointers cast through finddir_func/readdir_func
vfs_node_t *tmpfs_finddir_func(vfs_node_t *node, const char *name) {
    return tmpfs_finddir(node, name);
}

struct dirent *tmpfs_readdir_func(vfs_node_t *node, uint32_t index) {
    return tmpfs_readdir(node, index);
}

// Create a file in tmpfs directory
static struct tmpfs_entry *tmpfs_mkdir_entry(struct tmpfs_entry *dir, const char *name) {
    if (!dir || dir->child_count >= TMPFS_MAX_ENTRIES) return NULL;
    if (tmpfs_find_child(dir, name)) return NULL;
    struct tmpfs_entry *e = tmpfs_create_entry(name, FS_DIRECTORY, dir);
    if (!e) return NULL;
    dir->children[dir->child_count++] = e;
    return e;
}

static struct tmpfs_entry *tmpfs_create_file_entry(struct tmpfs_entry *dir, const char *name) {
    if (!dir || dir->child_count >= TMPFS_MAX_ENTRIES) return NULL;
    struct tmpfs_entry *existing = tmpfs_find_child(dir, name);
    if (existing) return existing;
    struct tmpfs_entry *e = tmpfs_create_entry(name, FS_FILE, dir);
    if (!e) return NULL;
    dir->children[dir->child_count++] = e;
    return e;
}

static int tmpfs_remove_entry(struct tmpfs_entry *dir, const char *name) {
    if (!dir) return -EIO;
    for (int i = 0; i < dir->child_count; i++) {
        if (dir->children[i]) {
            int match = 1;
            for (int j = 0; dir->children[i]->name[j] || name[j]; j++) {
                if (dir->children[i]->name[j] != name[j]) { match = 0; break; }
            }
            if (match) {
                struct tmpfs_entry *e = dir->children[i];
                // Free file data
                if (e->data) kfree(e->data);
                // Recursively free children
                if (e->flags & FS_DIRECTORY) {
                    for (int c = 0; c < e->child_count; c++) {
                        if (e->children[c]) {
                            if (e->children[c]->data) kfree(e->children[c]->data);
                            kfree(e->children[c]);
                        }
                    }
                }
                kfree(e);
                // Shift remaining entries
                for (int j = i; j < dir->child_count - 1; j++) {
                    dir->children[j] = dir->children[j + 1];
                }
                dir->child_count--;
                return 0;
            }
        }
    }
    return -ENOENT;
}

static int tmpfs_rename_entry(struct tmpfs_entry *dir, const char *oldname, const char *newname) {
    struct tmpfs_entry *e = tmpfs_find_child(dir, oldname);
    if (!e) return -ENOENT;
    if (tmpfs_find_child(dir, newname)) return -EEXIST;
    int i = 0;
    while (newname[i] && i < TMPFS_MAX_NAME - 1) { e->name[i] = newname[i]; i++; }
    e->name[i] = 0;
    return 0;
}

// Root directory entry for tmpfs
static struct tmpfs_entry *tmpfs_root_entry = 0;

vfs_node_t *tmpfs_root = 0;

void tmpfs_init(void) {
    printk(KERN_INFO "TMPFS: Initializing RAM-based writable filesystem...\n");
    tmpfs_root_entry = tmpfs_create_entry("/", FS_DIRECTORY, 0);
    if (!tmpfs_root_entry) { printk(KERN_ERR "TMPFS: Failed to create root\n"); return; }

    tmpfs_root = kmalloc(sizeof(vfs_node_t));
    if (!tmpfs_root) return;
    memset(tmpfs_root, 0, sizeof(vfs_node_t));
    strcpy(tmpfs_root->name, "tmpfs");
    tmpfs_root->flags = FS_DIRECTORY;
    tmpfs_root->inode = tmpfs_root_entry->inode;
    tmpfs_root->impl = (uint64_t)tmpfs_root_entry;
    tmpfs_root->finddir_func = (void*)tmpfs_finddir_func;
    tmpfs_root->readdir_func = (void*)tmpfs_readdir_func;
    tmpfs_root->mkdir_func = (void*)tmpfs_mkdir_func;
    tmpfs_root->rmdir_func = (void*)tmpfs_rmdir_func;
    tmpfs_root->unlink_func = (void*)tmpfs_unlink_func;
    tmpfs_root->create_func = (void*)tmpfs_create_func;
    tmpfs_root->refcount = 1;

    printk(KERN_INFO "TMPFS: Root filesystem ready (max 256 entries, 1MB per file)\n");
}

// Internal accessors for syscalls
void *tmpfs_get_root_entry(void) { return tmpfs_root_entry; }

int tmpfs_mkdir(const char *path) {
    if (!tmpfs_root_entry) return -ENODEV;
    struct tmpfs_entry *e = tmpfs_mkdir_entry(tmpfs_root_entry, path);
    return e ? 0 : -EIO;
}

int tmpfs_rmdir(const char *path) {
    if (!tmpfs_root_entry) return -ENODEV;
    struct tmpfs_entry *e = tmpfs_find_child(tmpfs_root_entry, path);
    if (!e || !(e->flags & FS_DIRECTORY)) return -ENOENT;
    if (e->child_count > 0) return -ENOTEMPTY;
    return tmpfs_remove_entry(tmpfs_root_entry, path);
}

int tmpfs_unlink(const char *path) {
    if (!tmpfs_root_entry) return -ENODEV;
    struct tmpfs_entry *e = tmpfs_find_child(tmpfs_root_entry, path);
    if (!e || (e->flags & FS_DIRECTORY)) return -ENOENT;
    return tmpfs_remove_entry(tmpfs_root_entry, path);
}

int tmpfs_rename(const char *oldpath, const char *newpath) {
    if (!tmpfs_root_entry) return -ENODEV;
    return tmpfs_rename_entry(tmpfs_root_entry, oldpath, newpath);
}

vfs_node_t *tmpfs_open_file(const char *name, int flags) {
    if (!tmpfs_root_entry) return NULL;
    struct tmpfs_entry *e;
    if (flags & 0x100) {
        e = tmpfs_create_file_entry(tmpfs_root_entry, name);
    } else {
        e = tmpfs_find_child(tmpfs_root_entry, name);
    }
    if (!e) return NULL;
    return tmpfs_make_vfs_node(e);
}

int tmpfs_access(const char *path) {
    if (!tmpfs_root_entry) return -ENODEV;
    struct tmpfs_entry *e = tmpfs_find_child(tmpfs_root_entry, path);
    return e ? 0 : -ENOENT;
}
