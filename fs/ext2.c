#include "ext2.h"
#include "kernel.h"
#include "block.h"
#include "vfs.h"
#include "kstring.h"
#include "malloc.h"
#include "errno.h"

static struct ext2_fs ext2_data;
static int ext2_mounted = 0;
static uint32_t ext2_allocate_block(void);
static vfs_node_t *ext2_create_file(vfs_node_t *dir, const char *name, uint32_t flags);

static uint32_t ext2_read_blocks(uint32_t block_num, uint32_t count, void *buffer) {
    if (!ext2_data.dev) return 0;
    uint32_t sector = block_num * (ext2_data.block_size / 512);
    uint32_t sectors = count * (ext2_data.block_size / 512);
    return block_read(ext2_data.dev, sector, sectors, buffer);
}

static uint32_t ext2_write_blocks(uint32_t block_num, uint32_t count, void *buffer) {
    if (!ext2_data.dev) return 0;
    uint32_t sector = block_num * (ext2_data.block_size / 512);
    uint32_t sectors = count * (ext2_data.block_size / 512);
    return block_write(ext2_data.dev, sector, sectors, buffer);
}

static uint32_t ext2_read_inode(uint32_t inode_num, ext2_inode_t *inode) {
    if (inode_num < 1 || inode_num > ext2_data.sb.inodes_count) return 0;
    uint32_t group = (inode_num - 1) / ext2_data.inodes_per_group;
    uint32_t index = (inode_num - 1) % ext2_data.inodes_per_group;
    uint32_t tbl_block = ext2_data.group_descs[group].inode_table;
    uint32_t offset = index * ext2_data.inode_size;
    uint32_t block_offset = offset / ext2_data.block_size;
    uint32_t within_block = offset % ext2_data.block_size;

    uint8_t *block_buf = kmalloc(ext2_data.block_size);
    if (!block_buf) return 0;
    ext2_read_blocks(tbl_block + block_offset, 1, block_buf);
    memcpy(inode, block_buf + within_block, sizeof(ext2_inode_t));
    kfree(block_buf);
    return 1;
}

static uint32_t ext2_write_inode(uint32_t inode_num, ext2_inode_t *inode) {
    if (inode_num < 1 || inode_num > ext2_data.sb.inodes_count) return 0;
    uint32_t group = (inode_num - 1) / ext2_data.inodes_per_group;
    uint32_t index = (inode_num - 1) % ext2_data.inodes_per_group;
    uint32_t tbl_block = ext2_data.group_descs[group].inode_table;
    uint32_t offset = index * ext2_data.inode_size;
    uint32_t block_offset = offset / ext2_data.block_size;
    uint32_t within_block = offset % ext2_data.block_size;

    uint8_t *block_buf = kmalloc(ext2_data.block_size);
    if (!block_buf) return 0;
    ext2_read_blocks(tbl_block + block_offset, 1, block_buf);
    memcpy(block_buf + within_block, inode, sizeof(ext2_inode_t));
    ext2_write_blocks(tbl_block + block_offset, 1, block_buf);
    kfree(block_buf);
    return 1;
}

static uint32_t ext2_get_block_addr(ext2_inode_t *inode, uint32_t logical_block, int create) {
    uint32_t ptrs_per_block = ext2_data.block_size / 4;
    uint32_t block_buf[1024];
    uint32_t addr;

    if (logical_block < EXT2_NDIR_BLOCKS) {
        addr = inode->block[logical_block];
        if (addr == 0 && create) {
            addr = ext2_allocate_block();
            inode->block[logical_block] = addr;
        }
        return addr;
    }
    logical_block -= EXT2_NDIR_BLOCKS;

    if (logical_block < ptrs_per_block) {
        if (inode->block[EXT2_IND_BLOCK] == 0) {
            if (!create) return 0;
            inode->block[EXT2_IND_BLOCK] = ext2_allocate_block();
        }
        ext2_read_blocks(inode->block[EXT2_IND_BLOCK], 1, block_buf);
        addr = block_buf[logical_block];
        if (addr == 0 && create) {
            addr = ext2_allocate_block();
            block_buf[logical_block] = addr;
            ext2_write_blocks(inode->block[EXT2_IND_BLOCK], 1, block_buf);
        }
        return addr;
    }
    logical_block -= ptrs_per_block;

    if (logical_block < ptrs_per_block * ptrs_per_block) {
        if (inode->block[EXT2_DIND_BLOCK] == 0) {
            if (!create) return 0;
            inode->block[EXT2_DIND_BLOCK] = ext2_allocate_block();
        }
        uint32_t dind_block = inode->block[EXT2_DIND_BLOCK];
        ext2_read_blocks(dind_block, 1, block_buf);
        uint32_t ind_block_idx = logical_block / ptrs_per_block;
        uint32_t block_idx = logical_block % ptrs_per_block;
        if (block_buf[ind_block_idx] == 0) {
            if (!create) return 0;
            block_buf[ind_block_idx] = ext2_allocate_block();
            ext2_write_blocks(dind_block, 1, block_buf);
        }
        uint32_t ind_block = block_buf[ind_block_idx];
        ext2_read_blocks(ind_block, 1, block_buf);
        addr = block_buf[block_idx];
        if (addr == 0 && create) {
            addr = ext2_allocate_block();
            block_buf[block_idx] = addr;
            ext2_write_blocks(ind_block, 1, block_buf);
        }
        return addr;
    }
    return 0;
}

static uint32_t ext2_read_file_data(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (!node || !buffer) return 0;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;

    uint32_t inode_num = node->inode;
    ext2_inode_t inode;
    if (!ext2_read_inode(inode_num, &inode)) return 0;

    uint32_t read = 0;
    uint32_t block_size = ext2_data.block_size;
    uint8_t *block_buf = kmalloc(block_size);
    if (!block_buf) return 0;

    while (read < size) {
        uint32_t logical_block = (offset + read) / block_size;
        uint32_t block_offset = (offset + read) % block_size;
        uint32_t to_read = size - read;
        if (to_read > block_size - block_offset) to_read = block_size - block_offset;

        uint32_t phys_block = ext2_get_block_addr(&inode, logical_block, 0);
        if (phys_block == 0) break;

        ext2_read_blocks(phys_block, 1, block_buf);
        memcpy(buffer + read, block_buf + block_offset, to_read);
        read += to_read;
    }
    kfree(block_buf);
    return read;
}

static uint32_t ext2_write_file_data(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (!node || !buffer) return 0;

    uint32_t inode_num = node->inode;
    ext2_inode_t inode;
    if (!ext2_read_inode(inode_num, &inode)) return 0;

    uint32_t written = 0;
    uint32_t block_size = ext2_data.block_size;
    uint8_t *block_buf = kmalloc(block_size);
    if (!block_buf) return 0;

    while (written < size) {
        uint32_t logical_block = (offset + written) / block_size;
        uint32_t block_offset = (offset + written) % block_size;
        uint32_t to_write = size - written;
        if (to_write > block_size - block_offset) to_write = block_size - block_offset;

        uint32_t phys_block = ext2_get_block_addr(&inode, logical_block, 1);
        if (phys_block == 0) break;

        if (to_write < block_size) {
            ext2_read_blocks(phys_block, 1, block_buf);
        }
        memcpy(block_buf + block_offset, buffer + written, to_write);
        ext2_write_blocks(phys_block, 1, block_buf);
        written += to_write;
    }

    if (offset + written > inode.size) {
        inode.size = offset + written;
        node->length = inode.size;
    }
    inode.blocks = (inode.size + block_size - 1) / block_size;
    ext2_write_inode(inode_num, &inode);
    kfree(block_buf);
    return written;
}

static struct dirent *ext2_readdir(vfs_node_t *node, uint32_t index) {
    if (!node || !(node->flags & FS_DIRECTORY)) return NULL;

    uint32_t inode_num = node->inode;
    ext2_inode_t inode;
    if (!ext2_read_inode(inode_num, &inode)) return NULL;

    uint32_t block_size = ext2_data.block_size;
    uint8_t *block_buf = kmalloc(block_size);
    if (!block_buf) return NULL;

    uint32_t curr_idx = 0;
    uint32_t num_blocks = (inode.size + block_size - 1) / block_size;

    for (uint32_t b = 0; b < num_blocks; b++) {
        uint32_t phys_block = ext2_get_block_addr(&inode, b, 0);
        if (phys_block == 0) continue;
        ext2_read_blocks(phys_block, 1, block_buf);

        uint32_t pos = 0;
        while (pos < block_size) {
            ext2_dir_entry_t *de = (ext2_dir_entry_t *)(block_buf + pos);
            if (de->inode == 0) { pos += de->rec_len; continue; }
            if (de->rec_len == 0) break;

            if (curr_idx == index) {
                char name[256];
                uint32_t nl = de->name_len;
                if (nl > 255) nl = 255;
                memcpy(name, de->name, nl);
                name[nl] = 0;

                struct dirent *d = kmalloc(sizeof(struct dirent));
                memset(d, 0, sizeof(struct dirent));
                strcpy(d->name, name);
                d->ino = de->inode;
                kfree(block_buf);
                return d;
            }
            curr_idx++;
            pos += de->rec_len;
        }
    }
    kfree(block_buf);
    return NULL;
}

static vfs_node_t *ext2_finddir(vfs_node_t *node, const char *name) {
    if (!node || !name) return NULL;
    if (!(node->flags & FS_DIRECTORY)) return NULL;
    if (strcmp(name, ".") == 0) return node;
    if (strcmp(name, "..") == 0) return node;

    uint32_t inode_num = node->inode;
    ext2_inode_t inode;
    if (!ext2_read_inode(inode_num, &inode)) return 0;

    uint32_t block_size = ext2_data.block_size;
    uint8_t *block_buf = kmalloc(block_size);
    if (!block_buf) return 0;

    uint32_t num_blocks = (inode.size + block_size - 1) / block_size;
    for (uint32_t b = 0; b < num_blocks; b++) {
        uint32_t phys_block = ext2_get_block_addr(&inode, b, 0);
        if (phys_block == 0) continue;
        ext2_read_blocks(phys_block, 1, block_buf);

        uint32_t pos = 0;
        while (pos < block_size) {
            ext2_dir_entry_t *de = (ext2_dir_entry_t *)(block_buf + pos);
            if (de->inode == 0) { pos += de->rec_len; continue; }
            if (de->rec_len == 0) break;

            if (de->name_len == (uint8_t)strlen(name)) {
                int match = 1;
                for (uint32_t i = 0; i < de->name_len; i++) {
                    if (de->name[i] != name[i]) { match = 0; break; }
                }
                if (match) {
                    uint32_t found_inode = de->inode;
                    kfree(block_buf);

                    ext2_inode_t finode;
                    if (!ext2_read_inode(found_inode, &finode)) return 0;

                    vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
                    memset(n, 0, sizeof(vfs_node_t));
                    int i = 0;
                    while (name[i] && i < 127) { n->name[i] = name[i]; i++; }
                    n->name[i] = 0;
                    n->inode = found_inode;
                    n->length = finode.size;
                    n->flags = (finode.mode & EXT2_S_IFDIR) ? FS_DIRECTORY : FS_FILE;
                    n->impl = (uint64_t)node; // parent directory for reference
                    n->read = ext2_read_file_data;
                    n->write = ext2_write_file_data;
                    n->finddir_func = (n->flags & FS_DIRECTORY) ? (void*)ext2_finddir : 0;
                    n->readdir_func = (n->flags & FS_DIRECTORY) ? (void*)ext2_readdir : 0;
                    n->create_func = (n->flags & FS_DIRECTORY) ? (void*)ext2_create_file : 0;
                    return n;
                }
            }
            pos += de->rec_len;
        }
    }
    kfree(block_buf);
    return NULL;
}

static uint32_t ext2_alloc_block_from_group(uint32_t group) {
    uint32_t block_size = ext2_data.block_size;
    uint8_t *bitmap_block = kmalloc(block_size);
    if (!bitmap_block) return 0;

    uint32_t bitmap_addr = ext2_data.group_descs[group].block_bitmap;
    ext2_read_blocks(bitmap_addr, 1, bitmap_block);

    uint32_t blocks_in_group = ext2_data.blocks_per_group;
    if (group == 0 && ext2_data.sb.first_data_block > 0)
        blocks_in_group--;

    for (uint32_t i = 0; i < blocks_in_group; i++) {
        uint32_t byte = i / 8;
        uint32_t bit = i % 8;
        if (!(bitmap_block[byte] & (1 << bit))) {
            bitmap_block[byte] |= (1 << bit);
            ext2_write_blocks(bitmap_addr, 1, bitmap_block);
            kfree(bitmap_block);
            return group * ext2_data.blocks_per_group + ext2_data.sb.first_data_block + i;
        }
    }
    kfree(bitmap_block);
    return 0;
}

static uint32_t ext2_allocate_block(void) {
    for (uint32_t g = 0; g < ext2_data.groups_count; g++) {
        if (ext2_data.group_descs[g].free_blocks_count > 0) {
            uint32_t block = ext2_alloc_block_from_group(g);
            if (block) {
                ext2_data.group_descs[g].free_blocks_count--;
                ext2_data.sb.free_blocks_count--;

                uint32_t sb_sector = 2 * (ext2_data.block_size / 512);
                uint32_t gdt_sector = sb_sector + (ext2_data.block_size / 512);
                ext2_write_blocks(1, 1, &ext2_data.sb);
                ext2_write_blocks(gdt_sector, ext2_data.group_desc_blocks, ext2_data.group_descs);

                uint8_t *zero_buf = kmalloc(ext2_data.block_size);
                if (zero_buf) {
                    memset(zero_buf, 0, ext2_data.block_size);
                    ext2_write_blocks(block, 1, zero_buf);
                    kfree(zero_buf);
                }
                return block;
            }
        }
    }
    return 0;
}

static uint32_t ext2_allocate_inode(void) {
    for (uint32_t g = 0; g < ext2_data.groups_count; g++) {
        if (ext2_data.group_descs[g].free_inodes_count > 0) {
            uint32_t bitmap_addr = ext2_data.group_descs[g].inode_bitmap;
            uint8_t *bitmap_block = kmalloc(ext2_data.block_size);
            if (!bitmap_block) return 0;
            ext2_read_blocks(bitmap_addr, 1, bitmap_block);

            for (uint32_t i = 0; i < ext2_data.inodes_per_group; i++) {
                uint32_t byte = i / 8;
                uint32_t bit = i % 8;
                if (!(bitmap_block[byte] & (1 << bit))) {
                    bitmap_block[byte] |= (1 << bit);
                    ext2_write_blocks(bitmap_addr, 1, bitmap_block);
                    kfree(bitmap_block);

                    ext2_data.group_descs[g].free_inodes_count--;
                    ext2_data.sb.free_inodes_count--;

                    uint32_t sb_sector = 2 * (ext2_data.block_size / 512);
                    uint32_t gdt_sector = sb_sector + (ext2_data.block_size / 512);
                    ext2_write_blocks(1, 1, &ext2_data.sb);
                    ext2_write_blocks(gdt_sector, ext2_data.group_desc_blocks, ext2_data.group_descs);

                    uint32_t inode_num = g * ext2_data.inodes_per_group + i + 1;
                    return inode_num;
                }
            }
            kfree(bitmap_block);
        }
    }
    return 0;
}

static int ext2_add_dir_entry(uint32_t dir_ino, const char *name, uint32_t file_inode, uint32_t file_type) {
    ext2_inode_t dir_inode;
    if (!ext2_read_inode(dir_ino, &dir_inode)) return -EIO;

    uint32_t block_size = ext2_data.block_size;
    uint32_t name_len = strlen(name);
    uint32_t entry_size = sizeof(ext2_dir_entry_t) + name_len;
    entry_size = (entry_size + 3) & ~3;
    if (entry_size < 8) entry_size = 8;

    uint8_t *block_buf = kmalloc(block_size);
    if (!block_buf) return -ENOMEM;

    uint32_t num_blocks = (dir_inode.size + block_size - 1) / block_size;
    if (num_blocks == 0) num_blocks = 1;

    int found = 0;
    for (uint32_t b = 0; b < num_blocks && !found; b++) {
        uint32_t phys_block = ext2_get_block_addr(&dir_inode, b, 1);
        if (phys_block == 0) continue;
        ext2_read_blocks(phys_block, 1, block_buf);

        uint32_t pos = 0;
        while (pos < block_size) {
            ext2_dir_entry_t *de = (ext2_dir_entry_t *)(block_buf + pos);
            if (de->inode == 0) {
                if (de->rec_len >= entry_size) {
                    de->inode = file_inode;
                    de->name_len = name_len;
                    de->file_type = file_type;
                    memcpy(de->name, name, name_len);
                    if (de->rec_len - entry_size >= 8) {
                        ext2_dir_entry_t *next = (ext2_dir_entry_t *)(block_buf + pos + entry_size);
                        next->rec_len = de->rec_len - entry_size;
                        next->inode = 0;
                        de->rec_len = entry_size;
                    }
                    ext2_write_blocks(phys_block, 1, block_buf);
                    found = 1;
                }
                break;
            }
            pos += de->rec_len;
        }
    }

    if (!found) {
        uint32_t new_block = ext2_allocate_block();
        if (!new_block) { kfree(block_buf); return -ENOSPC; }
        dir_inode.block[dir_inode.size / block_size] = new_block;
        memset(block_buf, 0, block_size);
        ext2_dir_entry_t *de = (ext2_dir_entry_t *)block_buf;
        de->inode = file_inode;
        de->rec_len = block_size;
        de->name_len = name_len;
        de->file_type = file_type;
        memcpy(de->name, name, name_len);
        ext2_write_blocks(new_block, 1, block_buf);
        dir_inode.size += block_size;
        dir_inode.blocks = dir_inode.size / block_size;
    }

    ext2_write_inode(dir_ino, &dir_inode);
    kfree(block_buf);
    return 0;
}

static vfs_node_t *ext2_create_file(vfs_node_t *dir, const char *name, uint32_t flags) {
    if (!dir || !name) return 0;
    if (!(dir->flags & FS_DIRECTORY)) return 0;

    uint32_t new_ino = ext2_allocate_inode();
    if (!new_ino) return 0;

    ext2_inode_t inode;
    memset(&inode, 0, sizeof(ext2_inode_t));
    inode.mode = EXT2_S_IFREG | 0644;
    inode.uid = 0;
    inode.gid = 0;
    inode.size = 0;
    inode.links_count = 1;
    inode.blocks = 0;

    uint32_t now = 1000000;
    inode.atime = now;
    inode.ctime = now;
    inode.mtime = now;

    ext2_write_inode(new_ino, &inode);

    if (ext2_add_dir_entry(dir->inode, name, new_ino, EXT2_FT_REG_FILE) < 0) return 0;

    vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
    memset(n, 0, sizeof(vfs_node_t));
    int i = 0;
    while (name[i] && i < 127) { n->name[i] = name[i]; i++; }
    n->name[i] = 0;
    n->inode = new_ino;
    n->length = 0;
    n->flags = FS_FILE;
    n->impl = (uint64_t)dir;
    n->read = ext2_read_file_data;
    n->write = ext2_write_file_data;
    return n;
}

static vfs_node_t *ext2_mkdir(vfs_node_t *dir, const char *name) {
    if (!dir || !name) return 0;
    if (!(dir->flags & FS_DIRECTORY)) return 0;

    uint32_t new_ino = ext2_allocate_inode();
    if (!new_ino) return 0;

    ext2_inode_t inode;
    memset(&inode, 0, sizeof(ext2_inode_t));
    inode.mode = EXT2_S_IFDIR | 0755;
    inode.uid = 0;
    inode.gid = 0;
    inode.size = 0;
    inode.links_count = 2;
    inode.blocks = 0;
    uint32_t now = 1000000;
    inode.atime = now;
    inode.ctime = now;
    inode.mtime = now;

    ext2_write_inode(new_ino, &inode);

    ext2_add_dir_entry(new_ino, ".", new_ino, EXT2_FT_DIR);
    ext2_add_dir_entry(new_ino, "..", dir->inode, EXT2_FT_DIR);
    ext2_add_dir_entry(dir->inode, name, new_ino, EXT2_FT_DIR);

    dir->flags |= FS_DIRECTORY;

    vfs_node_t *n = kmalloc(sizeof(vfs_node_t));
    memset(n, 0, sizeof(vfs_node_t));
    int i = 0;
    while (name[i] && i < 127) { n->name[i] = name[i]; i++; }
    n->name[i] = 0;
    n->inode = new_ino;
    n->length = 0;
    n->flags = FS_DIRECTORY;
    n->impl = (uint64_t)dir;
    n->read = 0;
    n->write = 0;
    n->finddir_func = (void*)ext2_finddir;
    n->readdir_func = (void*)ext2_readdir;
    return n;
}

static int ext2_unlink(vfs_node_t *dir, const char *name) {
    if (!dir || !name) return -EINVAL;
    if (!(dir->flags & FS_DIRECTORY)) return -ENOTDIR;

    uint32_t dir_ino = dir->inode;
    ext2_inode_t dir_inode;
    if (!ext2_read_inode(dir_ino, &dir_inode)) return -EIO;

    uint32_t block_size = ext2_data.block_size;
    uint8_t *block_buf = kmalloc(block_size);
    if (!block_buf) return -ENOMEM;

    uint32_t num_blocks = (dir_inode.size + block_size - 1) / block_size;
    int found = 0;

    for (uint32_t b = 0; b < num_blocks && !found; b++) {
        uint32_t phys_block = ext2_get_block_addr(&dir_inode, b, 0);
        if (phys_block == 0) continue;
        ext2_read_blocks(phys_block, 1, block_buf);

        uint32_t pos = 0;
        while (pos < block_size) {
            ext2_dir_entry_t *de = (ext2_dir_entry_t *)(block_buf + pos);
            if (de->inode == 0) { pos += de->rec_len; continue; }
            if (de->rec_len == 0) break;

            if (de->name_len == (uint8_t)strlen(name)) {
                int match = 1;
                for (uint32_t i = 0; i < de->name_len; i++) {
                    if (de->name[i] != name[i]) { match = 0; break; }
                }
                if (match) {
                    de->inode = 0;
                    ext2_write_blocks(phys_block, 1, block_buf);
                    found = 1;
                    break;
                }
            }
            pos += de->rec_len;
        }
    }
    kfree(block_buf);
    return found ? 0 : -ENOENT;
}

int ext2_mount(void) {
    block_device_t *dev = block_get_device("sda");
    if (!dev) {
        printk(KERN_ERR "EXT2: Could not find block device 'sda'\n");
        return -ENODEV;
    }

    ext2_data.dev = dev;

    uint32_t sb_sector = 2 * (dev->block_size / 512);
    uint8_t *sb_buf = kmalloc(1024);
    if (!sb_buf) return -ENOMEM;
    block_read(dev, sb_sector, 1024 / dev->block_size, sb_buf);
    memcpy(&ext2_data.sb, sb_buf, sizeof(ext2_superblock_t));
    kfree(sb_buf);

    if (ext2_data.sb.magic != EXT2_SUPER_MAGIC) {
        printk(KERN_ERR "EXT2: Invalid magic 0x%x\n", ext2_data.sb.magic);
        return -EINVAL;
    }

    ext2_data.block_size = 1024 << ext2_data.sb.log_block_size;
    ext2_data.inodes_per_group = ext2_data.sb.inodes_per_group;
    ext2_data.blocks_per_group = ext2_data.sb.blocks_per_group;

    if (ext2_data.sb.rev_level >= EXT2_DYNAMIC_REV)
        ext2_data.inode_size = ext2_data.sb.inode_size;
    else
        ext2_data.inode_size = 128;

    ext2_data.groups_count = (ext2_data.sb.blocks_count + ext2_data.blocks_per_group - 1) / ext2_data.blocks_per_group;

    uint32_t group_desc_size = ext2_data.groups_count * sizeof(ext2_group_desc_t);
    ext2_data.group_desc_blocks = (group_desc_size + ext2_data.block_size - 1) / ext2_data.block_size;

    ext2_data.group_descs = kmalloc(ext2_data.group_desc_blocks * ext2_data.block_size);
    if (!ext2_data.group_descs) return -ENOMEM;
    uint32_t gdt_block = 2;
    if (ext2_data.block_size > 1024) gdt_block = 1;
    ext2_read_blocks(gdt_block, ext2_data.group_desc_blocks, ext2_data.group_descs);

    ext2_inode_t root_inode;
    if (!ext2_read_inode(EXT2_ROOT_INO, &root_inode)) {
        printk(KERN_ERR "EXT2: Failed to read root inode\n");
        return -EIO;
    }

    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    if (!root) return -ENOMEM;
    memset(root, 0, sizeof(vfs_node_t));
    strcpy(root->name, "/");
    root->inode = EXT2_ROOT_INO;
    root->length = root_inode.size;
    root->flags = FS_DIRECTORY;
    root->read = ext2_read_file_data;
    root->write = ext2_write_file_data;
    root->finddir_func = (void*)ext2_finddir;
    root->readdir_func = (void*)ext2_readdir;
    root->create_func = (void*)ext2_create_file;
    root->impl = (uint64_t)ext2_data.dev;

    ext2_data.root_node = root;
    ext2_mounted = 1;

    printk(KERN_INFO "EXT2: Mounted ext2 filesystem (%u blocks, %u inodes)\n",
           ext2_data.sb.blocks_count, ext2_data.sb.inodes_count);
    return 0;
}

void ext2_init(void) {
    printk(KERN_INFO "EXT2: Initializing Ext2 filesystem driver...\n");
    if (ext2_mount() == 0) {
        vfs_mount("/mnt", ext2_data.root_node, 0);
        printk(KERN_INFO "EXT2: Mounted at /mnt\n");
    }
}
