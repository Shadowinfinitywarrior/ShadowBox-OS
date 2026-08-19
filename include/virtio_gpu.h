#ifndef SHADOWBOX_VIRTIO_GPU_H
#define SHADOWBOX_VIRTIO_GPU_H

#include "types.h"
#include "pci.h"

#define VIRTIO_GPU_VENDOR_ID  0x1AF4
#define VIRTIO_GPU_DEVICE_ID  0x1012

#define VIRTIO_GPU_PCI_DEVICE_ID 0x1012

/* Virtio GPU Register Offsets (MMIO) */
#define VIRTIO_GPU_REG_MAGIC    0x00
#define VIRTIO_GPU_REG_VERSION  0x04
#define VIRTIO_GPU_REG_DEVICE_ID 0x08
#define VIRTIO_GPU_REG_VENDOR   0x0C
#define VIRTIO_GPU_REG_HOST_FEATURES 0x10
#define VIRTIO_GPU_REG_GUEST_FEATURES 0x20
#define VIRTIO_GPU_REG_ADDR_Q 0x40  /* Queue 0 address */
#define VIRTIO_GPU_REG_SIZE_Q 0x48
#define VIRTIO_GPU_REG_NOTIFY_Q 0x50
#define VIRTIO_GPU_REG_STATUS 0x60
#define VIRTIO_GPU_REG_Q_SELECT 0x70
#define VIRTIO_GPU_REG_Q_READY 0x72
#define VIRTIO_GPU_REG_Q_PFN 0x80
#define VIRTIO_GPU_REG_Q_NUM 0x84
#define VIRTIO_GPU_REG_Q_ACK 0x90
#define VIRTIO_GPU_REG_IRQ_ACK 0xA0
#define VIRTIO_GPU_REG_IRQ_NUM 0xC0

#define VIRTIO_GPU_STATUS_ACKNOWLEDGE 1
#define VIRTIO_GPU_STATUS_DRIVER 2
#define VIRTIO_GPU_STATUS_FAILED 128
#define VIRTIO_GPU_STATUS_FEATURES_OK 8
#define VIRTIO_GPU_STATUS_DRIVER_OK 4

#define VIRTIO_GPU_QUEUE_SIZE 64

/* Virtio GPU Commands */
#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO 0x100
#define VIRTIO_GPU_CMD_RESOURCE_CREATE_2D 0x200
#define VIRTIO_GPU_CMD_RESOURCE_UNREF 0x201
#define VIRTIO_GPU_CMD_RESOURCE_MAP_BLOB 0x202
#define VIRTIO_GPU_CMD_SET_SCANOUT 0x300
#define VIRTIO_GPU_CMD_SET_SCANOUT_BLOB 0x301
#define VIRTIO_GPU_CMD_COMMIT 0x302
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH 0x303
#define VIRTIO_GPU_CMD_RES_CREATE_BLOB 0x106

/* Virtio GPU Config */
#define VIRTIO_GPU_CONFIG_INFO_LENGTH 4
#define VIRTIO_GPU_CONFIG_FLAG_EVENTS 1

typedef struct virtio_gpu_display_info {
    uint32_t phandle;
    uint32_t enabled;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} __attribute__((packed)) virtio_gpu_display_info_t;

typedef struct virtio_gpu_config {
    uint32_t events_read;
    uint32_t events_clear;
    uint32_t num_scanouts;
    uint32_t resource_scanout;
} __attribute__((packed)) virtio_gpu_config_t;

typedef struct virtio_gpu_resource {
    uint32_t resource_id;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    void *back_buffer;
    uint64_t back_buffer_phys;
} virtio_gpu_resource_t;

typedef struct virtio_gpu_device {
    pci_device_t *pci_dev;
    void *mmio_base;
    uint64_t mmio_phys;
    uint32_t resource_id_next;
    uint32_t scanout_resource_id;
    virtio_gpu_resource_t *current_resource;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
    uint8_t initialized;
    void *framebuffer;
    uint64_t framebuffer_phys;
} virtio_gpu_device_t;

/* Virtqueue descriptor */
typedef struct virtio_gpu_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed)) virtio_gpu_desc_t;

/* Virtio GPU command header */
typedef struct virtio_gpu_ctrl_hdr {
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t context_init;
    uint32_t _padding;
} __attribute__((packed)) virtio_gpu_ctrl_hdr_t;

typedef struct virtio_gpu_resource_create_2d {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t width;
    uint32_t height;
    uint32_t format;
} __attribute__((packed)) virtio_gpu_resource_create_2d_t;

typedef struct virtio_gpu_set_scanout {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t scanout_id;
    uint32_t resource_id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} __attribute__((packed)) virtio_gpu_set_scanout_t;

typedef struct virtio_gpu_resource_flush {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} __attribute__((packed)) virtio_gpu_resource_flush_t;

typedef struct virtio_gpu_resp_display_info {
    virtio_gpu_ctrl_hdr_t hdr;
    struct {
        uint32_t phandle;
        uint32_t enabled;
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
        uint32_t padding;
    } pmodes[16];
} __attribute__((packed)) virtio_gpu_resp_display_info_t;

void virtio_gpu_init(pci_device_t *pci_dev);
void virtio_gpu_irq_handler(void);
int virtio_gpu_create_resource(uint32_t width, uint32_t height, uint32_t format);
int virtio_gpu_set_scanout(uint32_t resource_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void virtio_gpu_flush_resource(uint32_t resource_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void *virtio_gpu_get_framebuffer(uint32_t *width, uint32_t *height, uint32_t *pitch);

#endif
