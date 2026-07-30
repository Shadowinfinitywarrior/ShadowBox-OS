#ifndef SHADOWBOX_DEVFS_H
#define SHADOWBOX_DEVFS_H

struct vfs_node;

/*
 * devfs_init - Initialize device filesystem
 */
void devfs_init(void);

/*
 * devfs_register_input - Register the input device (/dev/input)
 */
void devfs_register_input(void);

extern struct vfs_node *devfs_root;

#endif
