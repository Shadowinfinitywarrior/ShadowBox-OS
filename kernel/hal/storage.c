#include "hal/storage.h"
#include "kernel.h"
#include "malloc.h"
#include "block.h"
#include "ahci.h"
#include "pci.h"

static storage_device_t *storage_devices = NULL;
static int storage_count = 0;
static bool storage_initialized = false;

static storage_type_t block_to_storage_type(const char *name) {
    if (!name) return STORAGE_TYPE_UNKNOWN;
    if (name[0] == 's') return STORAGE_TYPE_SATA;
    if (name[0] == 'n') return STORAGE_TYPE_NVME;
    if (name[0] == 'v') return STORAGE_TYPE_VIRTIO;
    if (name[0] == 'r') return STORAGE_TYPE_RAMDISK;
    return STORAGE_TYPE_UNKNOWN;
}

static storage_device_t* create_storage_device(block_device_t *bdev) {
    storage_device_t *dev = kmalloc(sizeof(storage_device_t));
    if (!dev) return NULL;

    int i;
    for (i = 0; bdev->name[i] && i < 31; i++)
        dev->name[i] = bdev->name[i];
    dev->name[i] = 0;

    dev->id = storage_count;
    dev->type = block_to_storage_type(bdev->name);
    dev->interface = STORAGE_INTERFACE_SATA;
    dev->protocol = STORAGE_PROTOCOL_AHCI;
    dev->bus = 0;
    dev->device = 0;
    dev->function = 0;
    dev->private_data = (void *)bdev;
    dev->ops = NULL;
    dev->info.capacity = bdev->total_blocks * bdev->block_size;
    dev->info.sector_size = bdev->block_size;
    dev->info.sector_count = bdev->total_blocks;
    dev->info.flags = STORAGE_FLAG_READABLE | STORAGE_FLAG_WRITABLE;
    dev->initialized = true;
    dev->present = true;
    dev->next = NULL;

    return dev;
}

hal_status_t storage_init(void) {
    if (storage_initialized) return HAL_SUCCESS;

    printk(KERN_INFO "STORAGE: Initializing storage abstraction...\n");

    block_device_t *bdev = NULL;

    bdev = block_get_device("sda");
    if (bdev) {
        storage_device_t *dev = create_storage_device(bdev);
        if (dev) {
            dev->next = storage_devices;
            storage_devices = dev;
            storage_count++;
            printk(KERN_INFO "STORAGE: %s detected (%llu MB)\n",
                   dev->name, dev->info.capacity / (1024 * 1024));
        }
    }

    storage_initialized = true;
    return HAL_SUCCESS;
}

int storage_enumerate(void) {
    return storage_count;
}

storage_device_t* storage_get_devices(void) {
    return storage_devices;
}

storage_device_t* storage_find_device(const char *name) {
    storage_device_t *curr = storage_devices;
    while (curr) {
        int i = 0;
        while (curr->name[i] && name[i] && curr->name[i] == name[i]) i++;
        if (curr->name[i] == 0 && name[i] == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

hal_status_t storage_read(storage_device_t *dev, void *buf, uint64_t offset, uint64_t size) {
    if (!dev || !buf) return HAL_ERROR_IO;
    block_device_t *bdev = (block_device_t *)dev->private_data;
    if (!bdev) return HAL_ERROR_IO;

    uint64_t lba = offset / bdev->block_size;
    uint32_t count = size / bdev->block_size;
    if (size % bdev->block_size) count++;

    if (block_read(bdev, lba, count, buf) < 0)
        return HAL_ERROR_IO;
    return HAL_SUCCESS;
}

hal_status_t storage_write(storage_device_t *dev, const void *buf, uint64_t offset, uint64_t size) {
    if (!dev || !buf) return HAL_ERROR_IO;
    block_device_t *bdev = (block_device_t *)dev->private_data;
    if (!bdev) return HAL_ERROR_IO;

    uint64_t lba = offset / bdev->block_size;
    uint32_t count = size / bdev->block_size;
    if (size % bdev->block_size) count++;

    if (block_write(bdev, lba, count, (void *)buf) < 0)
        return HAL_ERROR_IO;
    return HAL_SUCCESS;
}

hal_status_t storage_flush(storage_device_t *dev) {
    (void)dev;
    return HAL_SUCCESS;
}

hal_status_t storage_trim(storage_device_t *dev, uint64_t offset, uint64_t size) {
    (void)dev; (void)offset; (void)size;
    return HAL_ERROR_UNSUPPORTED;
}

hal_status_t storage_get_info(storage_device_t *dev, storage_device_info_t *info) {
    if (!dev || !info) return HAL_ERROR_IO;
    *info = dev->info;
    return HAL_SUCCESS;
}

const char* storage_type_to_string(storage_type_t type) {
    switch (type) {
        case STORAGE_TYPE_SATA:    return "SATA";
        case STORAGE_TYPE_NVME:    return "NVMe";
        case STORAGE_TYPE_VIRTIO:  return "VirtIO";
        case STORAGE_TYPE_RAMDISK: return "RAM Disk";
        default:                   return "Unknown";
    }
}
