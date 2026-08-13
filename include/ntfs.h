#ifndef SHADOWBOX_NTFS_H
#define SHADOWBOX_NTFS_H

#include "types.h"
#include "vfs.h"
#include "block.h"

void ntfs_init(void);
vfs_node_t *ntfs_mount(block_device_t *dev);

#endif
