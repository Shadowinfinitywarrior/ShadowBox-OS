#include "tarfs.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"
#include "devfs.h"
#include "procfs.h"

static vfs_node_t *tarfs_root_node;
static uint64_t tar_address;
static uint64_t tar_size;

static uint32_t parse_size(const char *in) {
    uint32_t size = 0;
    uint32_t j;
    uint32_t count = 1;
    
    for (j = 11; j > 0; j--, count *= 8) {
        size += ((in[j - 1] - '0') * count);
    }
    
    return size;
}

// Forward declaration
static struct dirent *tarfs_readdir(vfs_node_t *node, uint32_t index);

static uint32_t tarfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (offset >= node->length) return 0;
    if (offset + size > node->length) {
        size = node->length - offset;
    }
    uint8_t *file_data = (uint8_t*)(uint64_t)node->impl;
    memcpy(buffer, file_data + offset, size);
    return size;
}

vfs_node_t *tarfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    if (strcmp(name, "dev") == 0) {
        return devfs_root;
    }
    if (strcmp(name, "proc") == 0) {
        return procfs_root;
    }

    uint64_t ptr = tar_address;
    uint64_t end = tar_address + tar_size;
    while (ptr + 512 <= end) {
        struct tar_header *header = (struct tar_header *)ptr;
        if (header->filename[0] == '\0') {
            break;
        }
        uint32_t size = parse_size(header->size);
        
        // Calculate if this file is directly inside our requested directory
        const char *prefix = (const char *)node->impl;
        int prefix_len = 0;
        if (prefix) {
            while (prefix[prefix_len]) prefix_len++;
        }
        
        // Does it match the prefix?
        int matches = 1;
        for (int i=0; i<prefix_len; i++) {
            if (header->filename[i] != prefix[i]) {
                matches = 0; break;
            }
        }
        
        if (matches) {
            // Check the remaining string
            const char *rem = &header->filename[prefix_len];
            
            // Extract the next token (file or directory name)
            char token[100] = {0};
            int ti = 0;
            int is_dir = 0;
            while (rem[ti] && rem[ti] != '/') {
                token[ti] = rem[ti];
                ti++;
            }
            if (rem[ti] == '/') {
                is_dir = 1;
            }
            
            if (strcmp(token, name) == 0) {
                vfs_node_t *fnode = kmalloc(sizeof(vfs_node_t));
                if (!fnode) return NULL;
                memset(fnode, 0, sizeof(vfs_node_t));
                
                strcpy(fnode->name, token);
                
                if (is_dir) {
                    fnode->flags = FS_DIRECTORY;
                    fnode->finddir_func = tarfs_finddir;
                    fnode->readdir_func = tarfs_readdir;
                    // Store new prefix string pointer (just alloc it)
                    char *new_pref = kmalloc(prefix_len + ti + 2);
                    if (prefix) strcpy(new_pref, prefix);
                    else new_pref[0] = 0;
                    strcat(new_pref, token);
                    strcat(new_pref, "/");
                    fnode->impl = (uint64_t)new_pref;
                } else {
                    fnode->flags = FS_FILE;
                    fnode->length = size;
                    fnode->read = tarfs_read;
                    fnode->impl = (uint64_t)(ptr + 512); // Re-use impl for data ptr since files don't search subdirs
                }
                
                return fnode;
            }
        }
        
        uint32_t blocks = (size + 511) / 512;
        ptr += 512 + blocks * 512;
    }
    return NULL;
}

struct dirent *tarfs_readdir(vfs_node_t *node, uint32_t index) {
    (void)node;
    uint64_t ptr = tar_address;
    uint64_t end = tar_address + tar_size;
    uint32_t curr_idx = 0;
    while (ptr + 512 <= end) {
        struct tar_header *header = (struct tar_header *)ptr;
        if (header->filename[0] == '\0') {
            break;
        }
        uint32_t size = parse_size(header->size);
        
        const char *prefix = (const char *)node->impl;
        int prefix_len = 0;
        if (prefix) {
            while (prefix[prefix_len]) prefix_len++;
        }
        
        int matches = 1;
        for (int i=0; i<prefix_len; i++) {
            if (header->filename[i] != prefix[i]) {
                matches = 0; break;
            }
        }
        
        if (matches && header->filename[prefix_len] != '\0') {
            // Is it directly in this directory?
            const char *rem = &header->filename[prefix_len];
            int has_slash = 0;
            int ti = 0;
            while (rem[ti]) {
                if (rem[ti] == '/') { has_slash = 1; break; }
                ti++;
            }
            
            // We only list it if it's a file with NO slash, OR it's a directory (first slash is at the end)
            // Wait, tar usually has explicit directory entries (ending in slash)
            if (!has_slash || (has_slash && rem[ti+1] == '\0')) {
                if (curr_idx == index) {
                    struct dirent *d = kmalloc(sizeof(struct dirent));
                    if (!d) return NULL;
                    memset(d, 0, sizeof(struct dirent));
                    
                    for (int i=0; i<ti; i++) d->name[i] = rem[i];
                    d->name[ti] = '\0';
                    d->ino = curr_idx;
                    return d;
                }
                curr_idx++;
            }
        }
        uint32_t blocks = (size + 511) / 512;
        ptr += 512 + blocks * 512;
    }
    
    if (curr_idx == index) {
        struct dirent *d = kmalloc(sizeof(struct dirent));
        if (!d) return NULL;
        memset(d, 0, sizeof(struct dirent));
        strcpy(d->name, "dev");
        d->ino = curr_idx;
        return d;
    }
    if (curr_idx + 1 == index) {
        struct dirent *d = kmalloc(sizeof(struct dirent));
        if (!d) return NULL;
        memset(d, 0, sizeof(struct dirent));
        strcpy(d->name, "proc");
        d->ino = curr_idx + 1;
        return d;
    }

    return NULL;
}

void tarfs_init(uint64_t address, uint64_t size) {
    tar_address = address;
    tar_size = size;
    
    tarfs_root_node = kmalloc(sizeof(vfs_node_t));
    if (!tarfs_root_node) return;
    memset(tarfs_root_node, 0, sizeof(vfs_node_t));
    
    tarfs_root_node->name[0] = '/';
    tarfs_root_node->name[1] = '\0';
    tarfs_root_node->flags = FS_DIRECTORY;
    tarfs_root_node->finddir_func = tarfs_finddir;
    tarfs_root_node->readdir_func = tarfs_readdir;
    tarfs_root_node->impl = (uint64_t)NULL; // NULL prefix for root
    
    fs_root = tarfs_root_node;
    
    printk(KERN_INFO "TarFS mounted at root, starting at %x\n", address);
}
