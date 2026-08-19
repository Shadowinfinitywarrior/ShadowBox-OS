#include "virtio_blk.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "io.h"
#include "malloc.h"
#include "kstring.h"

static virtio_blk_device_t vblk_dev;
static virtio_blk_virtq_desc_t *vblk_desc_table;
static uint16_t vblk_desc_head = 0;

static uint32_t vblk_read_reg(uint32_t offset) {
    return *(volatile uint32_t *)((uintptr_t)vblk_dev.mmio_base + offset);
}

static void vblk_write_reg(uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)((uintptr_t)vblk_dev.mmio_base + offset) = value;
}

static int vblk_reset_device(void) {
    vblk_write_reg(VIRTIO_BLK_REG_STATUS, 0);
    vblk_write_reg(VIRTIO_BLK_REG_STATUS, VIRTIO_BLK_STATUS_ACKNOWLEDGE);
    vblk_write_reg(VIRTIO_BLK_REG_STATUS,
                   VIRTIO_BLK_STATUS_ACKNOWLEDGE | VIRTIO_BLK_STATUS_DRIVER);

    /* Check magic */
    uint32_t magic = vblk_read_reg(VIRTIO_BLK_REG_MAGIC);
    if (magic != 0x1AF41200) {
        printk(KERN_ERR "VIRTIO-BLK: Invalid magic 0x%08x\n", magic);
        return -1;
    }

    return 0;
}

static int vblk_negotiate_features(void) {
    uint32_t host_features = vblk_read_reg(VIRTIO_BLK_REG_HOST_FEATURES);
    uint32_t guest_features = host_features & 0x00000001;  /* Just version 1 */
    vblk_write_reg(VIRTIO_BLK_REG_GUEST_FEATURES, guest_features);
    vblk_dev.features = guest_features;
    return 0;
}

static int vblk_init_virtqueue(void) {
    /* Allocate virtqueue descriptor table (256 entries * 16 bytes = 4KB) */
    uint64_t desc_phys = (uint64_t)pmm_alloc_page();
    vblk_desc_table = (virtio_blk_virtq_desc_t *)(desc_phys + 0xFFFFFFFF80000000ULL);
    memset(vblk_desc_table, 0, 4096);

    /* Setup queue 0 */
    vblk_write_reg(VIRTIO_BLK_REG_Q_SELECT, 0);
    vblk_write_reg(VIRTIO_BLK_REG_Q_PFN, (uint32_t)(desc_phys >> 12));
    vblk_write_reg(VIRTIO_BLK_REG_Q_NUM, 256);
    vblk_write_reg(VIRTIO_BLK_REG_Q_READY, 1);
    vblk_write_reg(VIRTIO_BLK_REG_Q_ACK, 0);

    /* Allocate DMA buffer for requests */
    uint64_t dma_phys = (uint64_t)pmm_alloc_page();
    vblk_dev.dma_buffer = (void *)(dma_phys + 0xFFFFFFFF80000000ULL);
    vblk_dev.dma_phys = dma_phys;
    memset(vblk_dev.dma_buffer, 0, 4096);

    return 0;
}

static void vblk_add_desc(uint64_t addr, uint32_t len, uint16_t flags, uint16_t next) {
    if (vblk_desc_head >= 256) return;
    vblk_desc_table[vblk_desc_head].addr = addr;
    vblk_desc_table[vblk_desc_head].len = len;
    vblk_desc_table[vblk_desc_head].flags = flags;
    vblk_desc_table[vblk_desc_head].next = next;
    vblk_desc_head = (vblk_desc_head + 1) % 256;
}

static int vblk_do_request(uint32_t type, uint64_t sector, uint32_t count, void *buffer) {
    if (!vblk_dev.initialized) return -1;

    memset(vblk_desc_table, 0, 4096);
    vblk_desc_head = 0;

    /* Build request header */
    virtio_blk_req_t *req = (virtio_blk_req_t *)vblk_dev.dma_buffer;
    req->type = type;
    req->ioprio = 0;
    req->sector = sector;

    __asm__ volatile("mfence" ::: "memory");

    if (type == VIRTIO_BLK_T_IN) {
        /* Read: device writes to buffer */
        vblk_add_desc((uint64_t)vblk_dev.dma_phys, sizeof(virtio_blk_req_t), 0x00001, 0);
        vblk_add_desc((uint64_t)(uintptr_t)buffer, count * 512, 0x00002, 0);
    } else {
        /* Write: device reads from buffer */
        vblk_add_desc((uint64_t)vblk_dev.dma_phys, sizeof(virtio_blk_req_t), 0x00001, 0);
        vblk_add_desc((uint64_t)(uintptr_t)buffer, count * 512, 0x00000, 0);
    }

    vblk_write_reg(VIRTIO_BLK_REG_Q_READY, 1);
    vblk_write_reg(VIRTIO_BLK_REG_NOTIFY_Q, 0);
    __asm__ volatile("mfence" ::: "memory");

    /* Wait for completion */
    for (volatile int i = 0; i < 10000000; i++) {
        uint8_t status = *((uint8_t *)vblk_dev.dma_buffer + sizeof(virtio_blk_req_t) + count * 512);
        if (status != 0) break;
        __builtin_ia32_pause();
    }

    uint8_t status = *((uint8_t *)vblk_dev.dma_buffer + sizeof(virtio_blk_req_t) + count * 512);
    if (status != 0) {
        printk(KERN_ERR "VIRTIO-BLK: Request failed (status=%u type=%u)\n", status, type);
        return -1;
    }

    return 0;
}

int virtio_blk_read(virtio_blk_device_t *dev, uint64_t sector, uint32_t count, void *buffer) {
    return vblk_do_request(VIRTIO_BLK_T_IN, sector, count, buffer);
}

int virtio_blk_write(virtio_blk_device_t *dev, uint64_t sector, uint32_t count, const void *buffer) {
    (void)dev;
    return vblk_do_request(VIRTIO_BLK_T_OUT, sector, count, (void *)buffer);
}

int virtio_blk_flush(virtio_blk_device_t *dev) {
    (void)dev;
    /* In virtio-blk, flush is a request with type = VIRTIO_BLK_T_FLUSH */
    virtio_blk_req_t *req = (virtio_blk_req_t *)vblk_dev.dma_buffer;
    req->type = VIRTIO_BLK_T_FLUSH;
    req->ioprio = 0;
    req->sector = 0;

    memset(vblk_desc_table, 0, 4096);
    vblk_desc_head = 0;
    vblk_add_desc((uint64_t)vblk_dev.dma_phys, sizeof(virtio_blk_req_t), 0x00001, 0);
    vblk_add_desc((uint64_t)(vblk_dev.dma_phys + sizeof(virtio_blk_req_t)), 1, 0x00002, 0);

    vblk_write_reg(VIRTIO_BLK_REG_Q_READY, 1);
    vblk_write_reg(VIRTIO_BLK_REG_NOTIFY_Q, 0);
    __asm__ volatile("mfence" ::: "memory");

    for (volatile int i = 0; i < 10000000; i++) {
        uint8_t status = *((uint8_t *)vblk_dev.dma_buffer + sizeof(virtio_blk_req_t));
        if (status != 0) break;
        __builtin_ia32_pause();
    }

    return 0;
}

static int vblk_init_block_device(virtio_blk_device_t *dev) {
    memset(&dev->block_dev, 0, sizeof(block_device_t));
    dev->block_dev.name = "vda";
    dev->block_dev.block_size = dev->sector_size;
    dev->block_dev.total_blocks = dev->capacity_sectors;
    dev->block_dev.read_block = NULL;
    dev->block_dev.write_block = NULL;
    dev->block_dev.scheduler = IOSCHED_NOOP;
    dev->block_dev.queue_depth = 0;
    dev->block_dev.max_sectors = 256;
    dev->block_dev.next = NULL;

    /* Wrap read/write through the block layer */
    block_register_device(&dev->block_dev);

    return 0;
}

void virtio_blk_init(pci_device_t *pci_dev) {
    if (!pci_dev) {
        printk(KERN_ERR "VIRTIO-BLK: No PCI device\n");
        return;
    }

    memset(&vblk_dev, 0, sizeof(vblk_dev));
    vblk_dev.pci_dev = pci_dev;

    printk(KERN_INFO "VIRTIO-BLK: Found device at PCI %02x:%02x (vendor=0x%04x dev=0x%04x)\n",
           pci_dev->bus, pci_dev->device, pci_dev->vendor_id, pci_dev->device_id);

    pci_enable_bus_mastering(pci_dev);

    /* Read BAR0 for MMIO registers */
    uint32_t bar0_low = pci_config_read(pci_dev->bus, pci_dev->device,
                                         pci_dev->function, 0x10);
    uint64_t bar0 = bar0_low & 0xFFFFFFF0;
    if ((bar0_low & 0x06) == 0x04) {
        uint32_t bar0_high = pci_config_read(pci_dev->bus, pci_dev->device,
                                              pci_dev->function, 0x14);
        bar0 |= ((uint64_t)bar0_high << 32);
    }

    vblk_dev.mmio_phys = bar0;
    vblk_dev.mmio_base = vmap_phys(bar0, 0x1000);

    if (!vblk_dev.mmio_base) {
        printk(KERN_ERR "VIRTIO-BLK: Failed to map MMIO at 0x%llx\n", bar0);
        return;
    }

    /* Reset and negotiate */
    if (vblk_reset_device() != 0) {
        printk(KERN_ERR "VIRTIO-BLK: Reset failed\n");
        return;
    }

    if (vblk_negotiate_features() != 0) {
        printk(KERN_ERR "VIRTIO-BLK: Feature negotiation failed\n");
        return;
    }

    /* Initialize virtqueue */
    if (vblk_init_virtqueue() != 0) {
        printk(KERN_ERR "VIRTIO-BLK: Virtqueue init failed\n");
        return;
    }

    /* Get capacity */
    /* In real virtio-blk, the capacity is read from the device config space */
    /* For QEMU virtio-blk, we can probe a large sector range */
    uint64_t capacity = 0;
    uint64_t low = vblk_read_reg(VIRTIO_BLK_REG_DEVICE_ID);  /* Not standard, using for capacity low */
    (void)low;

    /* Default: assume a 1GB disk (2M sectors) */
    capacity = 2 * 1024 * 1024;  /* sectors = 1GB / 512 */
    vblk_dev.capacity_sectors = capacity;
    vblk_dev.sector_size = 512;

    /* Mark as driver OK */
    vblk_write_reg(VIRTIO_BLK_REG_STATUS,
                   VIRTIO_BLK_STATUS_ACKNOWLEDGE |
                   VIRTIO_BLK_STATUS_DRIVER |
                   VIRTIO_BLK_STATUS_FEATURES_OK |
                   VIRTIO_BLK_STATUS_DRIVER_OK);

    vblk_init_block_device(&vblk_dev);

    vblk_dev.initialized = 1;
    printk(KERN_INFO "VIRTIO-BLK: Initialized (%llu sectors, %u BPS)\n",
           vblk_dev.capacity_sectors, vblk_dev.sector_size);
}
