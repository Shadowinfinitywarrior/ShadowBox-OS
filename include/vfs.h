#ifndef SHADOWBOX_VFS_H
#define SHADOWBOX_VFS_H

#include "types.h"

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

struct vfs_node;

typedef uint32_t (*read_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);
typedef struct dirent *(*readdir_type_t)(struct vfs_node*, uint32_t);
typedef struct vfs_node *(*finddir_type_t)(struct vfs_node*, const char *name);
typedef uint32_t (*seek_type_t)(struct vfs_node*, uint32_t, uint32_t);
typedef int (*ioctl_type_t)(struct vfs_node*, uint32_t, void*);

/*
 * vfs_node_t - Virtual filesystem node
 * @name:        File/directory name
 * @mask:        Permission mask
 * @uid, @gid:   Owner user/group ID
 * @flags:       Node type flags (FS_*)
 * @inode:       Inode number
 * @length:      File size
 * @impl:        Filesystem-specific data
 * @read, @write, @open, @close: File operation callbacks
 * @seek, @ioctl: Position and control callbacks
 * @readdir_func, @finddir_func: Directory operation callbacks
 * @ptr:         Pointer for symlinks or mountpoints
 * @lock_count, @lock_type, @lock_owner, @lock_start, @lock_end: File locking
 * @atime, @mtime, @ctime: Timestamps
 * @nlink:       Hard link count
 * @block_size, @blocks: Block allocation info
 * @refcount:    Reference count
 */
typedef struct vfs_node {
    char name[128];
    uint32_t mask;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t inode;
    uint32_t length;
    uint64_t impl;

    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    seek_type_t seek;
    ioctl_type_t ioctl;

    void *readdir_func;
    void *finddir_func;
    void *mkdir_func;
    void *create_func;
    void *rmdir_func;
    void *unlink_func;

    struct vfs_node *ptr;

    uint32_t lock_count;
    uint32_t lock_type;
    uint32_t lock_owner;
    uint64_t lock_start;
    uint64_t lock_end;

    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
    uint32_t nlink;
    uint32_t block_size;
    uint64_t blocks;

    uint32_t refcount;
} vfs_node_t;

/*
 * dirent - Directory entry structure
 * @name: Entry name
 * @ino:  Inode number
 */
struct dirent {
    char name[128];
    uint32_t ino;
};

extern vfs_node_t *fs_root;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2

/*
 * vfs_init - Initialize VFS subsystem
 */
void vfs_init(void);

/*
 * vfs_mount - Mount a filesystem at a path
 * @path: Mount point path
 * @node: Root node of the filesystem
 * @flags: Mount flags (MS_RDONLY, etc.)
 * Returns: 0 on success, -1 on error
 */
int vfs_mount(const char *path, vfs_node_t *node, uint64_t flags);

/*
 * vfs_unmount - Unmount a filesystem
 * @path: Mount point path
 * Returns: 0 on success, -1 on error
 */
int vfs_unmount(const char *path);

/*
 * vfs_read - Read from a VFS node
 * @node:   Node to read from
 * @offset: Starting offset
 * @size:   Number of bytes to read
 * @buffer: Destination buffer
 * Returns: Bytes read
 */
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

/*
 * vfs_write - Write to a VFS node
 * @node:   Node to write to
 * @offset: Starting offset
 * @size:   Number of bytes to write
 * @buffer: Source buffer
 * Returns: Bytes written
 */
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

/*
 * vfs_open - Open a VFS node
 * @node: Node to open
 */
void vfs_open(vfs_node_t *node);

/*
 * vfs_close - Close a VFS node
 * @node: Node to close
 */
void vfs_close(vfs_node_t *node);

/*
 * vfs_finddir - Find a directory entry by name
 * @node: Directory node
 * @name: Entry name to find
 * Returns: Found node, or NULL
 */
vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name);

/*
 * vfs_create - Create a new file under a directory
 * @parent: Parent directory node
 * @name: Name to create
 * @mode: Flags / mode
 * Returns: newly created node, or NULL
 */
vfs_node_t *vfs_create(vfs_node_t *parent, const char *name, uint32_t mode);

int vfs_mkdir(const char *path, vfs_node_t *start, uint32_t mode);
int vfs_rmdir(const char *path, vfs_node_t *start);
int vfs_unlink(const char *path, vfs_node_t *start);

/*
 * vfs_resolve_path - Resolve a full path to a vfs_node
 * @path:  Path to resolve (absolute or relative)
 * @start: Starting node for relative paths (NULL = fs_root)
 * @out_name: Optional buffer for the last component name
 * Returns: The parent directory node of the final component, or the node itself if it's a mount point
 */
vfs_node_t *vfs_resolve_path(const char *path, vfs_node_t *start, char *out_name);

/*
 * vfs_readdir - Read a directory entry by index
 * @node:  Directory node
 * @index: Entry index
 * Returns: dirent structure, or NULL
 */
struct dirent *vfs_readdir(vfs_node_t *node, uint32_t index);

/*
 * vfs_seek - Seek to a position in a node
 * @node:   Node to seek in
 * @offset: Target offset
 * @whence: Seek mode (SEEK_SET, SEEK_CUR, SEEK_END)
 * Returns: New offset
 */
uint32_t vfs_seek(vfs_node_t *node, uint32_t offset, uint32_t whence);

/*
 * vfs_ioctl - I/O control operation
 * @node:    Node to control
 * @request: Request code
 * @arg:     Argument pointer
 * Returns: 0 on success, -1 on error
 */
int vfs_ioctl(vfs_node_t *node, uint32_t request, void *arg);

/*
 * vfs_flock - File locking operation
 * @node:      Node to lock
 * @operation: Lock operation
 * Returns: 0 on success, -1 on error
 */
int vfs_flock(vfs_node_t *node, int operation);

/*
 * vfs_lock_file - Lock a file region
 * @node:  Node to lock
 * @start: Start offset
 * @end:   End offset
 * @type:  Lock type (F_RDLCK, F_WRLCK)
 * @owner: Lock owner PID
 * Returns: 0 on success, -1 on error
 */
int vfs_lock_file(vfs_node_t *node, uint64_t start, uint64_t end, int type, uint32_t owner);

/*
 * vfs_unlock_file - Unlock a file region
 * @node:  Node to unlock
 * @start: Start offset
 * @end:   End offset
 * @owner: Lock owner PID
 * Returns: 0 on success, -1 on error
 */
int vfs_unlock_file(vfs_node_t *node, uint64_t start, uint64_t end, uint32_t owner);

/*
 * vfs_update_times - Update node timestamps
 * @node:  Node to update
 * @atime: Update access time flag
 * @mtime: Update modification time flag
 * @ctime: Update change time flag
 */
void vfs_update_times(vfs_node_t *node, int atime, int mtime, int ctime);

/*
 * vfs_get_inode - Get a VFS node by inode number
 * @inode: Inode number
 * Returns: vfs_node pointer, or NULL
 */
vfs_node_t *vfs_get_inode(uint32_t inode);

/*
 * vfs_put_inode - Release an inode reference
 * @node: Node to release
 */
void vfs_put_inode(vfs_node_t *node);

#endif
