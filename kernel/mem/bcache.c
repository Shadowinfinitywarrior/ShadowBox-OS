/*
 * bcache.c – Minimal block cache stub for the kernel.
 *
 * This file provides placeholder implementations that compile but do not
 * contain any real caching logic. The functions simply forward to the
 * underlying block device API defined in include/block.h. They exist so
 * that other kernel components can link against the expected symbols.
 */

#include "block.h"
#include "kernel.h"

/* Initialize the block cache – currently a no‑op. */
void bcache_init(void) {
    /* No initialization required for the stub implementation. */
}

/* Read through the cache.  The stub forwards directly to block_read. */
int bcache_read(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    return block_read(dev, lba, count, buffer);
}

/* Write through the cache.  The stub forwards directly to block_write. */
int bcache_write(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    return block_write(dev, lba, count, buffer);
}

/* Flush cached writes – forwarded to block_flush. */
int bcache_flush(block_device_t *dev) {
    return block_flush(dev);
}
