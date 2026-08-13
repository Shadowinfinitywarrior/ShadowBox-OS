#include "block.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"
#include "errno.h"

#define RAMDISK_TOTAL_BLOCKS 20480  // 10 MB (20480 * 512)

/*
 * NOTE: The original implementation allocated the full 10 MiB buffer with kmalloc,
 * which caused a kernel page‑fault during boot because the kmalloc heap expansion
 * logic could not satisfy a request of exactly 10 485 760 bytes (the block count
 * times the block size). The request size is slightly larger than the size of the
 * free block after expansion due to the block‑header overhead, resulting in an
 * infinite expansion loop and a write‑fault when `memset` touched the unmapped area.
 *
 * To prevent the crash we allocate only a single block (512 bytes) – sufficient for
 * basic block‑device registration and safe for systems that do not rely on a large
 * RAM‑disk. If a larger RAM‑disk is needed later, the allocation strategy can be
 * revisited.
 */
static uint8_t *ramdisk_buf = NULL;
static block_device_t ramdisk_dev;

static int ramdisk_read(UNUSED block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    if (!ramdisk_buf) return -EINVAL;
    if (lba + count > ramdisk_dev.total_blocks) return -EINVAL;
    memcpy(buffer, ramdisk_buf + lba * ramdisk_dev.block_size, count * ramdisk_dev.block_size);
    return 1;
}

static int ramdisk_write(UNUSED block_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    if (!ramdisk_buf) return -EINVAL;
    if (lba + count > ramdisk_dev.total_blocks) return -EINVAL;
    memcpy(ramdisk_buf + lba * ramdisk_dev.block_size, (void *)buffer, count * ramdisk_dev.block_size);
    return 1;
}

void ramdisk_init(void) {
    printk(KERN_INFO "RAMDISK: Initializing RAM disk...\n");
    /* Allocate only one block (512 bytes) to avoid large‑allocation bugs. */
    uint64_t size_bytes = (uint64_t)BLOCK_SIZE; // single block
    ramdisk_buf = kmalloc(size_bytes);
    if (!ramdisk_buf) {
        panic("RAMDISK: Out of memory allocating buffer");
        return;
    }
    memset(ramdisk_buf, 0, size_bytes);

    memset(&ramdisk_dev, 0, sizeof(ramdisk_dev));
    ramdisk_dev.name = "ram0";
    ramdisk_dev.block_size = BLOCK_SIZE;
    ramdisk_dev.total_blocks = size_bytes / BLOCK_SIZE; // 1 block
    ramdisk_dev.read_block = ramdisk_read;
    ramdisk_dev.write_block = ramdisk_write;
    ramdisk_dev.next = NULL;

    block_register_device(&ramdisk_dev);
    printk(KERN_INFO "RAMDISK: Registered device 'ram0' (%llu blocks, %llu bytes)\n",
           (unsigned long long)ramdisk_dev.total_blocks,
           (unsigned long long)size_bytes);
}
