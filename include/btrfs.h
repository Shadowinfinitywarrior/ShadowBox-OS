#ifndef SHADOWBOX_BTRFS_H
#define SHADOWBOX_BTRFS_H

#include "types.h"
#include "vfs.h"
#include "block.h"

void btrfs_init(void);
vfs_node_t *btrfs_mount(block_device_t *dev);

// BTRFS CoW and Snapshot APIs
int btrfs_create_snapshot(vfs_node_t *source_dir, const char *snapshot_name);

#endif
