#ifndef SHADOWBOX_SHADOWFS_H
#define SHADOWBOX_SHADOWFS_H

#include "vfs.h"

/* Initialize ShadowFS filesystem */
void shadowfs_init(void);

/* Root node of ShadowFS */
extern vfs_node_t *shadowfs_root;

#endif // SHADOWBOX_SHADOWFS_H
