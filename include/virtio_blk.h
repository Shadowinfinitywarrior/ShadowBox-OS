#ifndef SHADOWBOX_VIRTIO_BLK_H
#define SHADOWBOX_VIRTIO_BLK_H

#include "types.h"
#include "pci.h"
#include "block.h"

#define VIRTIO_BLK_VENDOR_ID  0x1AF4
#define VIRTIO_BLK_DEVICE_ID  0x1001

/* Virtio Block Device Register Offsets */
#define VIRTIO_BLK_REG_MAGIC        0x00
#define VIRTIO_BLK_REG_VERSION      0x04
#define VIRTIO_BLK_REG_DEVICE_ID    0x08
#define VIRTIO_BLK_REG_VENDOR       0x0C
#define VIRTIO_BLK_REG_HOST_FEATURES 0x10
#define VIRTIO_BLK_REG_GUEST_FEATURES 0x20
#define VIRTIO_BLK_REG_ADDR_Q       0x40
#define VIRTIO_BLK_REG_SIZE_Q       0x48
#define VIRTIO_BLK_REG_NOTIFY_Q     0x50
#define VIRTIO_BLK_REG_STATUS       0x60
#define VIRTIO_BLK_REG_Q_SELECT     0x70
#define VIRTIO_BLK_REG_Q_READY      0x72
#define VIRTIO_BLK_REG_Q_PFN        0x80
#define VIRTIO_BLK_REG_Q_NUM        0x84
#define VIRTIO_BLK_REG_Q_ACK        0x90
#define VIRTIO_BLK_REG_IRQ_ACK      0xA0

#define VIRTIO_BLK_STATUS_ACKNOWLEDGE  0x01
#define VIRTIO_BLK_STATUS_DRIVER       0x02
#define VIRTIO_BLK_STATUS_FAILED       0x80
#define VIRTIO_BLK_STATUS_FEATURES_OK  0x08
#define VIRTIO_BLK_STATUS_DRIVER_OK    0x04

#define VIRTIO_BLK_F_SEGMENTATION  0
#define VIRTIO_BLK_F_RO            1
#define VIRTIO_BLK_F_IDENTIFIERS   2
#define VIRTIO_BLK_F_MQ            4
#define VIRTIO_BLK_F_DISCARD       10
#define VIRTIO_BLK_F_WRITE_ZEROES  13

/* Virtio Block Request Types */
#define VIRTIO_BLK_T_IN            0
#define VIRTIO_BLK_T_OUT           1
#define VIRTIO_BLK_T_FLUSH         4

/* Virtio Block Request Header */
typedef struct virtio_blk_req {
    uint32_t type;
    uint32_t ioprio;
    uint64_t sector;
} __attribute__((packed)) virtio_blk_req_t;

typedef struct virtio_blk_virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed)) virtio_blk_virtq_desc_t;

typedef struct virtio_blk_device {
    pci_device_t *pci_dev;
    void *mmio_base;
    uint64_t mmio_phys;
    block_device_t block_dev;
    uint64_t capacity_sectors;
    uint32_t sector_size;
    uint8_t  initialized;
    uint32_t features;
    void *dma_buffer;
    uint64_t dma_phys;
} virtio_blk_device_t;

int virtio_blk_init(pci_device_t *pci_dev);
int virtio_blk_read(virtio_blk_device_t *dev, uint64_t sector, uint32_t count, void *buffer);
int virtio_blk_write(virtio_blk_device_t *dev, uint64_t sector, uint32_t count, const void *buffer);
int virtio_blk_flush(virtio_blk_device_t *dev);

#endif
