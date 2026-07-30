#ifndef SHADOWBOX_BLOCK_H
#define SHADOWBOX_BLOCK_H

#include "types.h"
#include "spinlock.h"

#define BLOCK_SIZE 512
#define MAX_REQUESTS 64

#define REQ_READ  0
#define REQ_WRITE 1
#define REQ_FLUSH 2

#define IOSCHED_NOOP   0
#define IOSCHED_DEADLINE 1
#define IOSCHED_CFQ    2

/*
 * block_request_t - Block I/O request
 * @lba:       Logical block address
 * @count:     Number of blocks
 * @buffer:    Data buffer
 * @type:      REQ_READ, REQ_WRITE, or REQ_FLUSH
 * @completed: Completion flag
 * @timestamp: Request submission time
 * @next, @prev: Linked list pointers
 */
typedef struct block_request {
    uint64_t lba;
    uint32_t count;
    void *buffer;
    int type;
    int completed;
    uint64_t timestamp;
    struct block_request *next;
    struct block_request *prev;
} block_request_t;

/*
 * block_device_t - Block device descriptor
 * @name:          Device name
 * @block_size:    Size of each block
 * @total_blocks:  Total number of blocks
 * @read_block:    Read blocks from device
 * @write_block:   Write blocks to device
 * @queue_lock:    I/O queue spinlock
 * @request_queue: Pending I/O requests list
 * @active_request: Currently active request
 * @scheduler:     I/O scheduler type
 * @queue_depth:   Current queue depth
 * @max_sectors:   Maximum sectors per request
 * @reads, @writes: I/O operation counts
 * @read_bytes, @write_bytes: Byte counts
 * @read_time, @write_time:  Timing stats
 * @next:          Next device in list
 */
typedef struct block_device {
    const char *name;
    uint32_t block_size;
    uint64_t total_blocks;
    int (*read_block)(struct block_device *dev, uint64_t lba, uint32_t count, void *buffer);
    int (*write_block)(struct block_device *dev, uint64_t lba, uint32_t count, void *buffer);
    spinlock_t queue_lock;
    block_request_t *request_queue;
    block_request_t *active_request;
    int scheduler;
    uint64_t queue_depth;
    uint64_t max_sectors;
    uint64_t reads;
    uint64_t writes;
    uint64_t read_bytes;
    uint64_t write_bytes;
    uint64_t read_time;
    uint64_t write_time;
    struct block_device *next;
} block_device_t;

/*
 * block_init - Initialize block I/O subsystem
 */
void block_init(void);

/*
 * block_register_device - Register a block device
 * @dev: Block device to register
 */
void block_register_device(block_device_t *dev);

/*
 * block_get_device - Get a block device by name
 * @name: Device name
 * Returns: Pointer to device, or NULL
 */
block_device_t* block_get_device(const char *name);

/*
 * block_read - Read blocks from a device
 * @dev:    Block device
 * @lba:    Starting LBA
 * @count:  Number of blocks
 * @buffer: Destination buffer
 * Returns: Number of blocks read, or -1 on error
 */
int block_read(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer);

/*
 * block_write - Write blocks to a device
 * @dev:    Block device
 * @lba:    Starting LBA
 * @count:  Number of blocks
 * @buffer: Source buffer
 * Returns: Number of blocks written, or -1 on error
 */
int block_write(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer);

/*
 * block_flush - Flush write cache on a device
 * @dev: Block device
 * Returns: 0 on success, -1 on error
 */
int block_flush(block_device_t *dev);

/*
 * block_set_scheduler - Set I/O scheduler for device
 * @dev:       Block device
 * @scheduler: Scheduler type (IOSCHED_*)
 */
void block_set_scheduler(block_device_t *dev, int scheduler);

/*
 * block_submit_request - Submit I/O request to device queue
 * @dev: Block device
 * @req: I/O request
 * Returns: 0 on success, -1 on error
 */
int block_submit_request(block_device_t *dev, block_request_t *req);

/*
 * block_complete_request - Mark request as completed
 * @req: I/O request to complete
 */
void block_complete_request(block_request_t *req);

/*
 * block_get_stats - Get device I/O statistics
 * @dev:        Block device
 * @reads:      Output for read count
 * @writes:     Output for write count
 * @read_bytes: Output for bytes read
 * @write_bytes: Output for bytes written
 */
void block_get_stats(block_device_t *dev, uint64_t *reads, uint64_t *writes,
                     uint64_t *read_bytes, uint64_t *write_bytes);

#endif
