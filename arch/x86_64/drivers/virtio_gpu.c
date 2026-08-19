#include "virtio_gpu.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "kstring.h"
#include "io.h"
#include "malloc.h"
#include "fb.h"
#include "display.h"

static virtio_gpu_device_t gpu_dev;
static volatile virtio_gpu_desc_t *gpu_desc_table = NULL;
static uint16_t gpu_desc_head = 0;
static uint16_t gpu_desc_tail = 0;
static uint32_t gpu_fence_id = 1;

static volatile uint32_t *gpu_mmio = NULL;

static void virtio_gpu_outb(uint16_t port, uint8_t val) {
    outb(port, val);
}

static uint8_t virtio_gpu_inb(uint16_t port) {
    return inb(port);
}

static void virtio_gpu_write_reg(uint32_t offset, uint32_t value) {
    if (gpu_mmio) {
        *(volatile uint32_t *)((uintptr_t)gpu_mmio + offset) = value;
    }
}

static uint32_t virtio_gpu_read_reg(uint32_t offset) {
    if (gpu_mmio) {
        return *(volatile uint32_t *)((uintptr_t)gpu_mmio + offset);
    }
    return 0;
}

static void virtio_gpu_notify_queue(uint32_t queue_index) {
    /* Notify host that descriptor table has entries */
    virtio_gpu_write_reg(VIRTIO_GPU_REG_NOTIFY_Q, queue_index);
    __asm__ volatile("mfence" ::: "memory");
}

static uint16_t virtio_gpu_add_desc(uint64_t addr, uint32_t len, uint16_t flags, uint16_t next) {
    if (gpu_desc_head >= VIRTIO_GPU_QUEUE_SIZE) return 0xFFFF;
    uint16_t index = gpu_desc_head;
    gpu_desc_table[index].addr = addr;
    gpu_desc_table[index].len = len;
    gpu_desc_table[index].flags = flags;
    gpu_desc_table[index].next = next;
    gpu_desc_head = (gpu_desc_head + 1) % VIRTIO_GPU_QUEUE_SIZE;
    return index;
}

static void virtio_gpu_kick(void) {
    __asm__ volatile("mfence" ::: "memory");
    virtio_gpu_write_reg(VIRTIO_GPU_REG_NOTIFY_Q, 0);
}

static void virtio_gpu_send_command(void *cmd, uint32_t cmd_size, void *resp, uint32_t resp_size) {
    uint16_t desc_index = 0xFFFF;

    if (resp && resp_size > 0) {
        uint16_t resp_desc = virtio_gpu_add_desc((uint64_t)(uintptr_t)resp, resp_size, 0x10002, 0); /* write-only, next */
        uint16_t cmd_desc = virtio_gpu_add_desc((uint64_t)(uintptr_t)cmd, cmd_size, 0x00001, resp_desc); /* write, next */
        desc_index = cmd_desc;
    } else {
        desc_index = virtio_gpu_add_desc((uint64_t)(uintptr_t)cmd, cmd_size, 0x00001, 0); /* write */
    }

    if (desc_index != 0xFFFF) {
        virtio_gpu_write_reg(VIRTIO_GPU_REG_Q_PFN, 0);
        virtio_gpu_write_reg(VIRTIO_GPU_REG_Q_READY, 1);
        virtio_gpu_kick();
        __asm__ volatile("mfence" ::: "memory");
    }
}

static int virtio_gpu_negotiate_features(void) {
    uint32_t host_features = virtio_gpu_read_reg(VIRTIO_GPU_REG_HOST_FEATURES);

    /* Negotiate: just accept what the host offers */
    uint32_t guest_features = host_features & 0x00000001; /* just version 1 */
    virtio_gpu_write_reg(VIRTIO_GPU_REG_GUEST_FEATURES, guest_features);

    return (virtio_gpu_read_reg(VIRTIO_GPU_REG_GUEST_FEATURES) == guest_features) ? 0 : -1;
}

static int virtio_gpu_init_virtqueue(void) {
    uint32_t align = 4096;
    uint32_t desc_size = VIRTIO_GPU_QUEUE_SIZE * sizeof(virtio_gpu_desc_t);

    /* Allocate descriptor table */
    uint64_t desc_phys = (uint64_t)pmm_alloc_page();
    gpu_desc_table = (volatile virtio_gpu_desc_t *)(desc_phys + 0xFFFFFFFF80000000ULL);
    memset((void *)gpu_desc_table, 0, desc_size);

    /* Configure virtqueue */
    virtio_gpu_write_reg(VIRTIO_GPU_REG_Q_SELECT, 0);
    virtio_gpu_write_reg(VIRTIO_GPU_REG_Q_PFN, (uint32_t)(desc_phys >> 12));
    virtio_gpu_write_reg(VIRTIO_GPU_REG_Q_NUM, VIRTIO_GPU_QUEUE_SIZE);

    /* Enable the queue */
    virtio_gpu_write_reg(VIRTIO_GPU_REG_Q_READY, 1);

    /* Acknowledge queue */
    virtio_gpu_write_reg(VIRTIO_GPU_REG_Q_ACK, 0);
    (void)align;

    return 0;
}

static void virtio_gpu_reset(void) {
    virtio_gpu_write_reg(VIRTIO_GPU_REG_STATUS, 0);
    virtio_gpu_write_reg(VIRTIO_GPU_REG_STATUS, VIRTIO_GPU_STATUS_ACKNOWLEDGE);
    virtio_gpu_write_reg(VIRTIO_GPU_REG_STATUS,
                         VIRTIO_GPU_STATUS_ACKNOWLEDGE | VIRTIO_GPU_STATUS_DRIVER);
}

static int virtio_gpu_get_display_info(uint32_t *width, uint32_t *height) {
    virtio_gpu_ctrl_hdr_t cmd = {
        .type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
        .flags = 0x00000001, /* VIRTIO_GPU_FLAG_FENCE */
        .fence_id = 0,
        .context_init = 0,
    };

    virtio_gpu_resp_display_info_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.hdr.type = 0; /* Will be filled by device */

    virtio_gpu_send_command(&cmd, sizeof(cmd), &resp, sizeof(resp));

    /* Wait for response */
    for (volatile int i = 0; i < 1000000; i++) {
        __builtin_ia32_pause();
        if (resp.hdr.type != 0) break;
    }

    /* Find first enabled mode */
    for (int i = 0; i < 16; i++) {
        if (resp.pmodes[i].enabled) {
            if (width) *width = resp.pmodes[i].width;
            if (height) *height = resp.pmodes[i].height;
            return 0;
        }
    }

    /* Fallback to default */
    if (width) *width = 1024;
    if (height) *height = 768;
    return 0;
}

void virtio_gpu_init(pci_device_t *pci_dev) {
    if (!pci_dev) {
        printk(KERN_ERR "VIRTIO-GPU: No PCI device\n");
        return;
    }

    memset(&gpu_dev, 0, sizeof(gpu_dev));
    gpu_dev.pci_dev = pci_dev;

    /* Verify it's a Virtio GPU device */
    if (pci_dev->vendor_id != VIRTIO_GPU_VENDOR_ID) {
        printk(KERN_WARN "VIRTIO-GPU: Unexpected vendor 0x%04x (expected 0x%04x)\n",
               pci_dev->vendor_id, VIRTIO_GPU_VENDOR_ID);
    }

    if (pci_dev->device_id != VIRTIO_GPU_DEVICE_ID) {
        printk(KERN_WARN "VIRTIO-GPU: Unexpected device 0x%04x (expected 0x%04x)\n",
               pci_dev->device_id, VIRTIO_GPU_DEVICE_ID);
    }

    pci_enable_bus_mastering(pci_dev);

    /* Read BAR0 (Virtio MMIO region) */
    uint32_t bar0_low = pci_config_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x10);
    uint64_t bar0 = bar0_low & 0xFFFFFFF0;
    if ((bar0_low & 0x06) == 0x04) {
        uint32_t bar0_high = pci_config_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x14);
        bar0 |= ((uint64_t)bar0_high << 32);
    }

    gpu_dev.mmio_phys = bar0;
    gpu_mmio = (volatile uint32_t *)vmap_phys(bar0, 0x1000);

    if (!gpu_mmio) {
        printk(KERN_ERR "VIRTIO-GPU: Failed to map MMIO at 0x%lx\n", bar0);
        return;
    }

    /* Reset device */
    virtio_gpu_reset();

    /* Check magic value */
    uint32_t magic = virtio_gpu_read_reg(VIRTIO_GPU_REG_MAGIC);
    if (magic != 0x1AF41200) {
        printk(KERN_ERR "VIRTIO-GPU: Invalid magic 0x%08x\n", magic);
        return;
    }

    printk(KERN_INFO "VIRTIO-GPU: Magic=0x%08x Version=0x%x DeviceID=0x%x Vendor=0x%x\n",
           magic, virtio_gpu_read_reg(VIRTIO_GPU_REG_VERSION),
           virtio_gpu_read_reg(VIRTIO_GPU_REG_DEVICE_ID),
           virtio_gpu_read_reg(VIRTIO_GPU_REG_VENDOR));

    /* Negotiate features */
    if (virtio_gpu_negotiate_features() != 0) {
        printk(KERN_ERR "VIRTIO-GPU: Feature negotiation failed\n");
        return;
    }

    /* Set up virtqueue 0 */
    if (virtio_gpu_init_virtqueue() != 0) {
        printk(KERN_ERR "VIRTIO-GPU: Failed to initialize virtqueue\n");
        return;
    }

    /* Mark device as driver-ok */
    virtio_gpu_write_reg(VIRTIO_GPU_REG_STATUS,
                         VIRTIO_GPU_STATUS_ACKNOWLEDGE |
                         VIRTIO_GPU_STATUS_DRIVER |
                         VIRTIO_GPU_STATUS_FEATURES_OK |
                         VIRTIO_GPU_STATUS_DRIVER_OK);

    /* Get display info */
    uint32_t width, height;
    virtio_gpu_get_display_info(&width, &height);
    gpu_dev.fb_width = width;
    gpu_dev.fb_height = height;
    gpu_dev.fb_pitch = width * 4; /* 32 BPP */

    /* Allocate framebuffer */
    uint64_t fb_phys = (uint64_t)pmm_alloc_page();
    gpu_dev.framebuffer = (void *)(fb_phys + 0xFFFFFFFF80000000ULL);
    gpu_dev.framebuffer_phys = fb_phys;
    memset(gpu_dev.framebuffer, 0, 4096);

    /* Create GPU resource and set scanout */
    gpu_dev.resource_id_next = 1;
    int ret = virtio_gpu_create_resource(width, height, 0x34424758); /* XBGR */
    if (ret < 0) {
        printk(KERN_ERR "VIRTIO-GPU: Failed to create resource\n");
        return;
    }

    gpu_dev.current_resource = kmalloc(sizeof(virtio_gpu_resource_t));
    gpu_dev.current_resource->resource_id = ret;
    gpu_dev.current_resource->width = width;
    gpu_dev.current_resource->height = height;
    gpu_dev.current_resource->stride = width * 4;
    gpu_dev.current_resource->format = 0x34424758;
    gpu_dev.scanout_resource_id = ret;

    virtio_gpu_set_scanout(ret, 0, 0, width, height);

    /* Update framebuffer info for fb subsystem */
    fb_set_info(gpu_dev.framebuffer_phys, width, height, gpu_dev.fb_pitch, 32);

    gpu_dev.initialized = 1;

    printk(KERN_INFO "VIRTIO-GPU: Initialized (%zux%zumode, framebuffer=0x%llx)\n",
           width, height, gpu_dev.framebuffer_phys);
}

int virtio_gpu_create_resource(uint32_t width, uint32_t height, uint32_t format) {
    if (!gpu_dev.initialized) return -1;

    uint32_t rid = gpu_dev.resource_id_next++;

    virtio_gpu_resource_create_2d_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    cmd.hdr.flags = 0x00000001;
    cmd.hdr.fence_id = gpu_fence_id++;
    cmd.resource_id = rid;
    cmd.width = width;
    cmd.height = height;
    cmd.format = format;

    virtio_gpu_send_command(&cmd, sizeof(cmd), NULL, 0);
    __asm__ volatile("mfence" ::: "memory");

    printk(KERN_INFO "VIRTIO-GPU: Created resource id=%d %zux%zu\n", rid, width, height);
    return (int)rid;
}

int virtio_gpu_set_scanout(uint32_t resource_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!gpu_dev.initialized) return -1;

    virtio_gpu_set_scanout_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
    cmd.hdr.flags = 0x00000001;
    cmd.hdr.fence_id = gpu_fence_id++;
    cmd.scanout_id = 0;
    cmd.resource_id = resource_id;
    cmd.x = x;
    cmd.y = y;
    cmd.width = w;
    cmd.height = h;

    virtio_gpu_send_command(&cmd, sizeof(cmd), NULL, 0);

    gpu_dev.scanout_resource_id = resource_id;

    return 0;
}

void virtio_gpu_flush_resource(uint32_t resource_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!gpu_dev.initialized) return;

    virtio_gpu_resource_flush_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    cmd.hdr.flags = 0x00000001;
    cmd.hdr.fence_id = gpu_fence_id++;
    cmd.resource_id = resource_id;
    cmd.x = x;
    cmd.y = y;
    cmd.width = w;
    cmd.height = h;

    virtio_gpu_send_command(&cmd, sizeof(cmd), NULL, 0);
}

void *virtio_gpu_get_framebuffer(uint32_t *width, uint32_t *height, uint32_t *pitch) {
    if (!gpu_dev.initialized) return NULL;
    if (width) *width = gpu_dev.fb_width;
    if (height) *height = gpu_dev.fb_height;
    if (pitch) *pitch = gpu_dev.fb_pitch;
    return gpu_dev.framebuffer;
}

void virtio_gpu_irq_handler(void) {
    if (!gpu_dev.initialized) return;

    /* Acknowledge interrupt */
    virtio_gpu_write_reg(VIRTIO_GPU_REG_IRQ_ACK, 1);

    /* In a full implementation, we would process completed descriptors */
    printk(KERN_DEBUG "VIRTIO-GPU: IRQ handled\n");
}
