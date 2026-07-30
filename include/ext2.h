#ifndef SHADOWBOX_EXT2_H
#define SHADOWBOX_EXT2_H

#include "types.h"
#include "vfs.h"
#include "block.h"

#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK  0xA000
#define EXT2_S_IFREG  0x8000
#define EXT2_S_IFBLK  0x6000
#define EXT2_S_IFDIR  0x4000
#define EXT2_S_IFCHR  0x2000
#define EXT2_S_IFIFO  0x1000

#define EXT2_ROOT_INO 2

#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV  1

#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK   12
#define EXT2_DIND_BLOCK  13
#define EXT2_TIND_BLOCK  14
#define EXT2_N_BLOCKS    15

/*
 * ext2_superblock_t - Ext2 superblock structure
 */
typedef struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    char     uuid[16];
    char     volume_name[16];
    char     last_mounted[64];
    uint32_t algo_bitmap;
    uint8_t  prealloc_blocks;
    uint8_t  prealloc_dir_blocks;
    uint16_t padding;
    char     journal_uuid[16];
    uint32_t journal_inum;
    uint32_t journal_dev;
    uint32_t last_orphan;
    uint32_t hash_seed[4];
    uint8_t  def_hash_version;
    uint8_t  reserved[3];
    uint32_t default_mount_options;
    uint32_t first_meta_bg;
    char     reserved2[760];
} __attribute__((packed)) ext2_superblock_t;

/*
 * ext2_group_desc_t - Ext2 block group descriptor
 */
typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint32_t reserved[3];
} __attribute__((packed)) ext2_group_desc_t;

/*
 * ext2_inode_t - Ext2 inode structure
 */
typedef struct {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[EXT2_N_BLOCKS];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint32_t osd2[3];
} __attribute__((packed)) ext2_inode_t;

/*
 * ext2_dir_entry_t - Ext2 directory entry (variable-length name)
 */
typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[0];
} __attribute__((packed)) ext2_dir_entry_t;

#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7

/*
 * ext2_fs - Mounted ext2 filesystem instance
 */
struct ext2_fs {
    ext2_superblock_t sb;
    block_device_t *dev;
    uint32_t block_size;
    uint32_t groups_count;
    ext2_group_desc_t *group_descs;
    uint32_t inodes_per_group;
    uint32_t inode_size;
    vfs_node_t *root_node;
    int mounted;
    uint32_t blocks_per_group;
    uint32_t group_desc_blocks;
};

/*
 * ext2_init - Initialize ext2 filesystem driver
 */
void ext2_init(void);

/*
 * ext2_mount - Mount the ext2 root filesystem
 * Returns: 0 on success, -1 on error
 */
int ext2_mount(void);

#endif
