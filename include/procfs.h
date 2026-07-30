#ifndef SHADOWBOX_PROCFS_H
#define SHADOWBOX_PROCFS_H

struct vfs_node;

/*
 * procfs_init - Initialize process filesystem
 */
void procfs_init(void);

extern struct vfs_node *procfs_root;

#endif
