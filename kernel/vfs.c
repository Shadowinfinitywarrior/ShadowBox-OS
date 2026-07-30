#include "vfs.h"
#include "mount.h"
#include "kernel.h"
#include "malloc.h"
#include "spinlock.h"
#include "kstring.h"
#include "errno.h"

vfs_node_t *fs_root = 0;
static spinlock_t vfs_lock;

// Mount table
#define MAX_MOUNTS 16
struct mount_entry {
    char path[128];
    vfs_node_t *node;
    uint64_t flags;
};
static struct mount_entry mount_table[MAX_MOUNTS];
static int mount_count = 0;

void vfs_init(void) {
    spinlock_init(&vfs_lock);
    mount_count = 0;
    printk("VFS initialized.\n");
}

int vfs_mount(const char *path, vfs_node_t *node, uint64_t flags) {
    if (mount_count >= MAX_MOUNTS) return -ENOMEM;
    spin_lock_irqsave(&vfs_lock);
    int len = 0;
    while (path[len] && len < 127) { mount_table[mount_count].path[len] = path[len]; len++; }
    mount_table[mount_count].path[len] = 0;
    mount_table[mount_count].node = node;
    mount_table[mount_count].flags = flags;
    mount_count++;
    spin_unlock_irqrestore(&vfs_lock);
    return 0;
}

int vfs_unmount(const char *path) {
    spin_lock_irqsave(&vfs_lock);
    for (int i = 0; i < mount_count; i++) {
        int match = 1;
        for (int j = 0; mount_table[i].path[j] || path[j]; j++) {
            if (mount_table[i].path[j] != path[j]) { match = 0; break; }
        }
        if (match) {
            for (int j = i; j < mount_count - 1; j++) mount_table[j] = mount_table[j + 1];
            mount_count--;
            spin_unlock_irqrestore(&vfs_lock);
            return 0;
        }
    }
    spin_unlock_irqrestore(&vfs_lock);
    return -1;
}

vfs_node_t *vfs_get_mount(const char *path) {
    for (int i = 0; i < mount_count; i++) {
        int match = 1;
        for (int j = 0; mount_table[i].path[j] || path[j]; j++) {
            if (mount_table[i].path[j] != path[j]) { match = 0; break; }
        }
        if (match) return mount_table[i].node;
    }
    return 0;
}

// Full path resolution: resolves a path relative to a start node
// Returns the vfs_node for the final component, and optionally the parent and last component name
vfs_node_t *vfs_resolve_path(const char *path, vfs_node_t *start, char *out_name) {
    if (!path || !*path) return start;

    vfs_node_t *current = start ? start : fs_root;
    int is_absolute = (path[0] == '/');

    if (is_absolute) {
        current = fs_root;
        path++;
    }

    char component[128];
    char full_path[128];
    full_path[0] = 0;

    while (*path) {
        while (*path == '/') path++;
        if (!*path) break;

        int ci = 0;
        while (*path && *path != '/' && ci < 127) {
            component[ci++] = *path++;
        }
        component[ci] = 0;

        // Build full path for mount lookup: "/dev", "/proc", etc.
        int flen = 0;
        while (full_path[flen]) flen++;
        full_path[flen++] = '/';
        for (int i = 0; i < ci && flen < 127; i++)
            full_path[flen++] = component[i];
        full_path[flen] = 0;

        vfs_node_t *mnt = vfs_get_mount(full_path);
        if (mnt) {
            current = mnt;
            continue;
        }

        if (ci == 1 && component[0] == '.') continue;
        if (ci == 2 && component[0] == '.' && component[1] == '.') {
            if (current->finddir_func) {
                vfs_node_t *parent = vfs_finddir(current, "..");
                if (parent) current = parent;
            }
            continue;
        }

        while (*path == '/') path++;
        if (*path) {
            vfs_node_t *next = vfs_finddir(current, component);
            if (!next) return 0;
            current = next;
        } else {
            if (out_name) {
                int i = 0;
                while (component[i]) { out_name[i] = component[i]; i++; }
                out_name[i] = 0;
            }
            return current;
        }
    }
    if (out_name) out_name[0] = 0;
    return current;
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (!node) return 0;
    // Check if mounted read-only
    for (int i = 0; i < mount_count; i++) {
        if (mount_table[i].node == node && (mount_table[i].flags & MS_RDONLY)) {
            return 0;
        }
    }
    if (node->write) {
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

void vfs_open(vfs_node_t *node) {
    if (node && node->open) {
        node->open(node);
    }
}

void vfs_close(vfs_node_t *node) {
    if (node && node->close) {
        node->close(node);
    }
}

vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name) {
    spin_lock_irqsave(&vfs_lock);
    vfs_node_t *ret = 0;
    if (node && (node->flags & FS_DIRECTORY) && node->finddir_func) {
        finddir_type_t fd = (finddir_type_t)node->finddir_func;
        ret = fd(node, (char *)name);
    }
    spin_unlock_irqrestore(&vfs_lock);
    return ret;
}

vfs_node_t *vfs_create(vfs_node_t *parent, const char *name, uint32_t mode) {
    spin_lock_irqsave(&vfs_lock);
    vfs_node_t *ret = NULL;
    if (parent && (parent->flags & FS_DIRECTORY) && parent->create_func) {
        typedef vfs_node_t* (*create_t)(vfs_node_t*, const char*, uint32_t);
        ret = ((create_t)parent->create_func)(parent, name, mode);
    }
    spin_unlock_irqrestore(&vfs_lock);
    return ret;
}

int vfs_mkdir(const char *path, vfs_node_t *start, uint32_t mode) {
    char name[128];
    vfs_node_t *parent = vfs_resolve_path(path, start, name);
    if (!parent) return -ENOENT;
    int ret = -EROFS;
    spin_lock_irqsave(&vfs_lock);
    if ((parent->flags & FS_DIRECTORY) && parent->mkdir_func) {
        typedef vfs_node_t* (*mkdir_t)(vfs_node_t*, const char*);
        vfs_node_t *n = ((mkdir_t)parent->mkdir_func)(parent, name);
        ret = n ? 0 : -EIO;
    }
    spin_unlock_irqrestore(&vfs_lock);
    return ret;
}

int vfs_rmdir(const char *path, vfs_node_t *start) {
    char name[128];
    vfs_node_t *parent = vfs_resolve_path(path, start, name);
    if (!parent) return -ENOENT;
    int ret = -EROFS;
    spin_lock_irqsave(&vfs_lock);
    if ((parent->flags & FS_DIRECTORY) && parent->rmdir_func) {
        typedef int (*rmdir_t)(vfs_node_t*, const char*);
        ret = ((rmdir_t)parent->rmdir_func)(parent, name);
    }
    spin_unlock_irqrestore(&vfs_lock);
    return ret;
}

int vfs_unlink(const char *path, vfs_node_t *start) {
    char name[128];
    vfs_node_t *parent = vfs_resolve_path(path, start, name);
    if (!parent) return -ENOENT;
    int ret = -EROFS;
    spin_lock_irqsave(&vfs_lock);
    if ((parent->flags & FS_DIRECTORY) && parent->unlink_func) {
        typedef int (*unlink_t)(vfs_node_t*, const char*);
        ret = ((unlink_t)parent->unlink_func)(parent, name);
    }
    spin_unlock_irqrestore(&vfs_lock);
    return ret;
}

struct dirent *vfs_readdir(vfs_node_t *node, uint32_t index) {
    spin_lock_irqsave(&vfs_lock);
    struct dirent *ret = 0;
    if (node && (node->flags & FS_DIRECTORY) && node->readdir_func) {
        readdir_type_t rd = (readdir_type_t)node->readdir_func;
        ret = rd(node, index);
    }
    spin_unlock_irqrestore(&vfs_lock);
    return ret;
}

uint32_t vfs_seek(vfs_node_t *node, uint32_t offset, uint32_t whence) {
    if (!node) return 0;
    uint32_t new_offset;
    switch (whence) {
        case SEEK_SET: new_offset = offset; break;
        case SEEK_CUR: new_offset = offset; break; // caller tracks current offset in file struct
        case SEEK_END: new_offset = node->length + offset; break;
        default: return 0;
    }
    return new_offset;
}

int vfs_ioctl(vfs_node_t *node, uint32_t request, void *arg) {
    if (node && node->ioctl) {
        return node->ioctl(node, request, arg);
    }
    return -1;
}

vfs_node_t *vfs_get_inode(uint32_t inode) {
    (void)inode;
    return 0;
}

void vfs_put_inode(vfs_node_t *node) {
    (void)node;
}

void vfs_update_times(vfs_node_t *node, int atime, int mtime, int ctime) {
    if (!node) return;
    node->atime = atime;
    node->mtime = mtime;
    node->ctime = ctime;
}

int vfs_flock(vfs_node_t *node, int operation) {
    if (!node) return -1;
    if (operation == F_UNLCK) {
        node->lock_count = 0;
        return 0;
    }
    if (node->lock_count > 0) return -1;
    node->lock_count = 1;
    node->lock_type = operation;
    return 0;
}

int vfs_lock_file(vfs_node_t *node, uint64_t start, uint64_t end, int type, uint32_t owner) {
    if (!node) return -1;
    node->lock_type = type;
    node->lock_owner = owner;
    node->lock_start = start;
    node->lock_end = end;
    node->lock_count = (type != F_UNLCK) ? 1 : 0;
    return 0;
}

int vfs_unlock_file(vfs_node_t *node, uint64_t start, uint64_t end, uint32_t owner) {
    if (!node) return -1;
    if (node->lock_owner == owner) {
        node->lock_count = 0;
        node->lock_type = F_UNLCK;
    }
    return 0;
}
