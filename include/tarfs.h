#ifndef SHADOWBOX_TARFS_H
#define SHADOWBOX_TARFS_H

#include "vfs.h"

/*
 * tar_header - TAR archive file header
 */
struct tar_header {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};

/*
 * tarfs_init - Initialize tarfs from memory
 * @address: Base address of tar archive
 * @size:    Size of tar archive
 */
void tarfs_init(uint64_t address, uint64_t size);

/*
 * tarfs_get_root - Get root node of tarfs
 * Returns: Root vfs_node pointer
 */
vfs_node_t *tarfs_get_root(void);

/*
 * tarfs_finddir - Find directory entry by name
 * @node: Directory node to search
 * @name: Entry name to find
 * Returns: Found node, or NULL
 */
vfs_node_t *tarfs_finddir(vfs_node_t *node, const char *name);

#endif
