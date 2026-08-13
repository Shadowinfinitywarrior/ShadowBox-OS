#ifndef SHADOWBOX_VFS_CACHE_H
#define SHADOWBOX_VFS_CACHE_H

#include "types.h"
#include "vfs.h"

// Dentry Cache (Directory Entry Cache)
typedef struct dentry {
    char name[256];
    uint32_t hash;
    vfs_node_t *inode;
    struct dentry *parent;
    struct dentry *next_hash; // Hash collision chain
    struct dentry *next_child;
    struct dentry *first_child;
    uint32_t ref_count;
} dentry_t;

// Page Cache (File data caching in RAM)
typedef struct page_cache_entry {
    vfs_node_t *inode;
    uint64_t file_offset;
    void *physical_page; // 4KB RAM frame
    uint8_t dirty;       // Needs flush to disk
    struct page_cache_entry *next;
} page_cache_entry_t;

// Inode Cache (In-memory inode objects)
typedef struct inode_cache_entry {
    uint32_t inode_num;
    vfs_node_t *node;
    uint32_t ref_count;
    struct inode_cache_entry *next;
} inode_cache_entry_t;

// File Descriptor Table (per process, maps fd -> vfs_node_t)
#define MAX_PROCESS_FDS 256
typedef struct fd_table {
    vfs_node_t *nodes[MAX_PROCESS_FDS];
    uint64_t offsets[MAX_PROCESS_FDS];
    uint32_t flags[MAX_PROCESS_FDS];
} fd_table_t;

void vfs_cache_init(void);

// Dentry API
dentry_t* dcache_lookup(dentry_t *parent, const char *name);
void dcache_add(dentry_t *parent, dentry_t *dentry);

// Page Cache API
void* page_cache_read(vfs_node_t *inode, uint64_t offset);
int page_cache_write(vfs_node_t *inode, uint64_t offset, void *data, size_t len);
void page_cache_flush(vfs_node_t *inode);

// Inode Cache API
vfs_node_t* icache_get(uint32_t inode_num);
void icache_put(vfs_node_t *node);

#endif
