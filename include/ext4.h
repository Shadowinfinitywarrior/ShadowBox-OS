#ifndef SHADOWBOX_EXT4_H
#define SHADOWBOX_EXT4_H

#include "types.h"
#include "vfs.h"
#include "block.h"

void ext4_init(void);
vfs_node_t *ext4_mount(block_device_t *dev);

#endif
