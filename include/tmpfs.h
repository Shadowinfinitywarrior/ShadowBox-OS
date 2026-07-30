#ifndef SHADOWBOX_TMPFS_H
#define SHADOWBOX_TMPFS_H

#include "vfs.h"

/*
 * tmpfs_init - Initialize tmpfs filesystem
 */
void tmpfs_init(void);

/*
 * tmpfs_mkdir - Create a directory
 * @path: Directory path
 * Returns: 0 on success, -1 on error
 */
int tmpfs_mkdir(const char *path);

/*
 * tmpfs_rmdir - Remove a directory
 * @path: Directory path
 * Returns: 0 on success, -1 on error
 */
int tmpfs_rmdir(const char *path);

/*
 * tmpfs_unlink - Remove a file
 * @path: File path
 * Returns: 0 on success, -1 on error
 */
int tmpfs_unlink(const char *path);

/*
 * tmpfs_rename - Rename a file or directory
 * @oldpath: Current path
 * @newpath: New path
 * Returns: 0 on success, -1 on error
 */
int tmpfs_rename(const char *oldpath, const char *newpath);

/*
 * tmpfs_open_file - Open a file by name
 * @name:  File name
 * @flags: Open flags
 * Returns: vfs_node pointer, or NULL
 */
vfs_node_t *tmpfs_open_file(const char *name, int flags);

/*
 * tmpfs_access - Check access to a path
 * @path: Path to check
 * Returns: 0 on success, -1 on error
 */
int tmpfs_access(const char *path);

/*
 * tmpfs_finddir_func - Find directory entry callback
 * @node: Directory node
 * @name: Entry name to find
 * Returns: Found node, or NULL
 */
vfs_node_t *tmpfs_finddir_func(vfs_node_t *node, const char *name);

/*
 * tmpfs_readdir_func - Read directory entry callback
 * @node:  Directory node
 * @index: Entry index
 * Returns: dirent structure, or NULL
 */
struct dirent *tmpfs_readdir_func(vfs_node_t *node, uint32_t index);

/*
 * tmpfs_get_root_entry - Get tmpfs root entry
 * Returns: Opaque root entry pointer
 */
void *tmpfs_get_root_entry(void);

extern vfs_node_t *tmpfs_root;

#endif
