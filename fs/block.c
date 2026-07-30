#include "block.h"
#include "kernel.h"
#include "kstring.h"
#include "errno.h"

static block_device_t *block_devices = NULL;

void block_init(void) {
    printk(KERN_INFO "BLOCK: Initializing block device layer...\n");
    block_devices = NULL;
}

void block_register_device(block_device_t *dev) {
    if (!dev) return;
    dev->next = block_devices;
    block_devices = dev;
    printk(KERN_INFO "BLOCK: Registered device '%s' (size=%llu blocks)\n", dev->name, dev->total_blocks);
}

block_device_t* block_get_device(const char *name) {
    block_device_t *curr = block_devices;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int block_read(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    if (!dev || !dev->read_block || !buffer) return -EINVAL;
    dev->reads++;
    dev->read_bytes += count * dev->block_size;
    return dev->read_block(dev, lba, count, buffer);
}

int block_write(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    if (!dev || !dev->write_block || !buffer) return -EINVAL;
    dev->writes++;
    dev->write_bytes += count * dev->block_size;
    return dev->write_block(dev, lba, count, buffer);
}
