#include "bluetooth.h"
#include "kernel.h"
#include "pci.h"
#include "vmm.h"
#include "pmm.h"
#include "io.h"
#include "malloc.h"
#include "kstring.h"
#include "spinlock.h"

static bt_hci_dev_t bt_hci;
static bt_remote_device_t *bt_scan_results = NULL;
static bt_remote_device_t *bt_remote_devices = NULL;
static bt_driver_ops_t bt_ops;
static spinlock_t bt_lock;

static void *bt_alloc_dma(uint32_t size) {
    uint64_t phys = (uint64_t)pmm_alloc_page();
    if (!phys) return NULL;
    void *virt = vmap_phys(phys, 4096);
    if (!virt) {
        pmm_free_page((void *)phys);
        return NULL;
    }
    memset(virt, 0, size > 4096 ? 4096 : size);
    bt_hci.dma_buffer = virt;
    bt_hci.dma_phys = phys;
    bt_hci.dma_size = size;
    return virt;
}

static void bt_write_reg(uint32_t offset, uint32_t value) {
    if (bt_hci.mmio_base) {
        *(volatile uint32_t *)((uintptr_t)bt_hci.mmio_base + offset) = value;
    }
}

static uint32_t bt_read_reg(uint32_t offset) {
    if (bt_hci.mmio_base) {
        return *(volatile uint32_t *)((uintptr_t)bt_hci.mmio_base + offset);
    }
    return 0;
}

void bt_hci_send_command(bt_hci_dev_t *dev, uint16_t opcode, void *data, uint8_t len) {
    if (!dev || !dev->initialized) return;

    /* In a real driver, this would format and send an HCI command packet:
       [0x01] [opcode_lo] [opcode_hi] [param_len] [params...]
       via the HCI transport (USB, UART, or SDIO) */
    uint8_t cmd_buf[256];
    cmd_buf[0] = BT_HCI_PACKET_COMMAND;
    cmd_buf[1] = opcode & 0xFF;
    cmd_buf[2] = (opcode >> 8) & 0xFF;
    cmd_buf[3] = len;

    if (data && len > 0) {
        memcpy(&cmd_buf[4], data, len > 252 ? 252 : len);
    }

    /* Write to HCI command register */
    bt_write_reg(0x0000, opcode);

    printk(KERN_DEBUG "BT: Sent HCI command 0x%04x (len=%u)\n", opcode, len);
}

static int bt_hardware_init(pci_device_t *pci_dev) {
    bt_hci.vendor_id = pci_dev->vendor_id;
    bt_hci.device_id = pci_dev->device_id;

    pci_enable_bus_mastering(pci_dev);

    /* Map BAR0 */
    uint32_t bar0_low = pci_config_read(pci_dev->bus, pci_dev->device,
                                         pci_dev->function, 0x10);
    uint64_t bar0 = bar0_low & 0xFFFFFFF0;
    if ((bar0_low & 0x06) == 0x04) {
        uint32_t bar0_high = pci_config_read(pci_dev->bus, pci_dev->device,
                                              pci_dev->function, 0x14);
        bar0 |= ((uint64_t)bar0_high << 32);
    }

    bt_hci.mmio_phys = bar0;
    bt_hci.mmio_base = vmap_phys(bar0, 0x10000);

    if (!bt_hci.mmio_base) {
        printk(KERN_ERR "BT: Failed to map MMIO at 0x%llx\n", bar0);
        return -1;
    }

    /* Reset HCI device */
    bt_write_reg(0x0000, 0xFFFFFFFF);
    for (volatile int i = 0; i < 100000; i++) __builtin_ia32_pause();
    bt_write_reg(0x0000, 0x00000000);

    /* Send HCI Reset command */
    uint8_t reset_params = 0;
    bt_hci_send_command(&bt_hci, BT_HCI_OP_RESET, &reset_params, 0);

    /* Wait for command complete */
    for (volatile int i = 0; i < 500000; i++) {
        uint32_t status = bt_read_reg(0x0010);
        if (status & 0x01) break;
        __builtin_ia32_pause();
    }

    /* Read local version info */
    uint32_t version = bt_read_reg(0x0020);

    /* Determine device type */
    if (bt_hci.vendor_id == BT_HCI_VENDOR_INTEL) {
        bt_hci.class_major = BT_CLASS_MAJOR_BREDR_BLE;
        bt_hci.sub_class = 0x01;
    } else if (bt_hci.vendor_id == BT_HCI_VENDOR_ATHEROS) {
        bt_hci.class_major = BT_CLASS_MAJOR_BREDR;
        bt_hci.sub_class = 0x01;
    }

    /* Generate default BD_ADDR */
    bt_hci.bd_addr[0] = 0x00;
    bt_hci.bd_addr[1] = 0x1A;
    bt_hci.bd_addr[2] = 0x7D;
    bt_hci.bd_addr[3] = 0xDA;
    bt_hci.bd_addr[4] = (pci_dev->device_id >> 8) & 0xFF;
    bt_hci.bd_addr[5] = pci_dev->device_id & 0xFF;

    return 0;
}

static void bt_init_net_device(void) {
    memset(&bt_hci.base, 0, sizeof(net_device_t));
    bt_hci.base.name = "bt0";
    memcpy(bt_hci.base.mac, bt_hci.bd_addr, 6);
    bt_hci.base.ip = 0;
    bt_hci.base.netmask = 0;
    bt_hci.base.gateway = 0;
    bt_hci.base.send_packet = bt_send_packet;
    bt_hci.base.next = NULL;
}

int bt_send_packet(net_device_t *dev, void *data, uint32_t len) {
    (void)dev;
    (void)data;
    (void)len;
    /* In a real driver, this would send raw HCI data */
    return (int)len;
}

int bt_start_le_scan(bt_hci_dev_t *dev, uint8_t active) {
    if (!dev || !dev->initialized) return -1;

    printk(KERN_INFO "BT: Starting LE scan (active=%d)\n", active);

    /* Set LE scan parameters */
    uint8_t scan_params[7];
    scan_params[0] = active ? 1 : 0;  /* Active scan */
    scan_params[1] = 0x10;             /* Scan interval (12.5ms * 16 = 200ms) */
    scan_params[2] = 0x00;
    scan_params[3] = 0x10;             /* Scan window */
    scan_params[4] = 0x00;
    scan_params[5] = 0x00;             /* Own address type (public) */
    scan_params[6] = 0x00;             /* Scan filter */

    bt_hci_send_command(dev, BT_HCI_OP_LE_SET_SCAN_PARAM, scan_params, sizeof(scan_params));

    /* Enable scanning */
    uint8_t enable_param = 0x01 | (active ? 0x02 : 0x00);
    bt_hci_send_command(dev, BT_HCI_OP_LE_SET_SCAN_ENABLE, &enable_param, 1);

    return 0;
}

int bt_connect_le(bt_hci_dev_t *dev, uint8_t *addr, uint8_t addr_type) {
    if (!dev || !dev->initialized || !addr) return -1;

    printk(KERN_INFO "BT: Connecting to %02x:%02x:%02x:%02x:%02x:%02x (type=%d)\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr_type);

    /* LE Create Connection command parameters */
    uint8_t conn_params[25];
    memset(conn_params, 0, sizeof(conn_params));

    /* Scan interval, window */
    conn_params[0] = 0x20; conn_params[1] = 0x00;  /* scan_interval */
    conn_params[2] = 0x20; conn_params[3] = 0x00;  /* scan_window */
    conn_params[4] = addr_type;                    /* initiator addr type */
    memcpy(&conn_params[5], addr, BT_BD_ADDR_LEN);
    conn_params[11] = addr_type;                   /* peer addr type */
    memcpy(&conn_params[12], addr, BT_BD_ADDR_LEN);
    conn_params[18] = 0x00; conn_params[19] = 0x00;  /* conn_interval_min */
    conn_params[20] = 0x00; conn_params[21] = 0x00;  /* conn_interval_max */
    conn_params[22] = 0x00;                          /* conn_latency */
    conn_params[23] = 0x00;                          /* supervision_timeout */
    conn_params[24] = 0x00;                          /* minimum CE pulse */

    bt_hci_send_command(dev, BT_HCI_OP_LE_CREATE_CONN, conn_params, sizeof(conn_params));

    return 0;
}

int bt_disconnect(bt_hci_dev_t *dev, uint16_t conn_handle, uint8_t reason) {
    if (!dev) return -1;

    uint8_t params[3];
    params[0] = conn_handle & 0xFF;
    params[1] = (conn_handle >> 8) & 0x0F;
    params[2] = reason;

    printk(KERN_INFO "BT: Disconnecting handle 0x%04x (reason=%d)\n", conn_handle, reason);
    bt_hci_send_command(dev, 0x0C06, params, sizeof(params));
    return 0;
}

int bt_set_le_tx_power(bt_hci_dev_t *dev, uint8_t power) {
    if (!dev) return -1;
    dev->le_tx_power = power;
    printk(KERN_INFO "BT: LE TX power set to %d dBm\n", power);
    return 0;
}

int bt_set_adv_params(bt_hci_dev_t *dev, uint8_t adv_type, uint16_t interval,
                      uint8_t *addr, uint8_t addr_type) {
    if (!dev || !addr) return -1;

    uint8_t params[15];
    memset(params, 0, sizeof(params));

    params[0] = adv_type & 0x03;
    params[1] = interval & 0xFF;
    params[2] = (interval >> 8) & 0xFF;
    params[3] = 0x00;
    params[4] = 0x01;  /* Own address type - public */
    params[5] = addr_type;
    memcpy(&params[6], addr, BT_BD_ADDR_LEN);
    params[12] = 0x00;  /* Advertising channel map (all channels) */
    params[13] = 0x00;  /* Advertising filter policy */
    params[14] = 0x00;  /* Min/ max TX power */

    bt_hci_send_command(dev, 0x2006, params, sizeof(params));
    return 0;
}

void bt_irq_handler(void) {
    if (!bt_hci.initialized) return;

    uint32_t status = bt_read_reg(0x0010);

    if (status & 0x01) {
        bt_write_reg(0x0010, 0x01);
        printk(KERN_DEBUG "BT: HCI command complete\n");
    }
    if (status & 0x02) {
        bt_write_reg(0x0010, 0x02);
        printk(KERN_DEBUG "BT: HCI event received\n");
    }
    if (status & 0x04) {
        bt_write_reg(0x0010, 0x04);
        printk(KERN_DEBUG "BT: ACL data received\n");
    }
    if (status & 0x08) {
        bt_write_reg(0x0010, 0x08);
        printk(KERN_DEBUG "BT: SCO data received\n");
    }
}

bt_hci_dev_t *bt_get_device(void) {
    return &bt_hci;
}

void bt_handle_packet(bt_hci_dev_t *dev, uint8_t *packet, uint32_t len) {
    if (!dev || !packet || len == 0) return;

    uint8_t pkt_type = packet[0];
    switch (pkt_type) {
        case BT_HCI_PACKET_EVENT:
            /* Parse event code */
            if (len >= 2) {
                uint8_t evt_code = packet[1];
                printk(KERN_DEBUG "BT: Event code 0x%02x (len=%u)\n", evt_code, len);
            }
            break;
        case BT_HCI_PACKET_ACL_DATA:
            printk(KERN_DEBUG "BT: ACL data received (len=%u)\n", len);
            break;
        case BT_HCI_PACKET_SCO_DATA:
            printk(KERN_DEBUG "BT: SCO data received (len=%u)\n", len);
            break;
    }
}

void bt_hci_init(pci_device_t *pci_dev) {
    if (!pci_dev) {
        printk(KERN_ERR "BT: No PCI device\n");
        return;
    }

    memset(&bt_hci, 0, sizeof(bt_hci_dev_t));
    bt_hci.pci_dev = pci_dev;
    spinlock_init(&bt_lock);

    if (bt_hardware_init(pci_dev) != 0) {
        printk(KERN_ERR "BT: Hardware initialization failed\n");
        return;
    }

    bt_init_net_device();

    /* Allocate DMA buffer */
    bt_alloc_dma(4096);

    /* Register with network stack */
    net_register_device(&bt_hci.base);

    /* Set up driver ops */
    bt_ops.init = bt_hci_init;
    bt_ops.send_command = bt_hci_send_command;
    bt_ops.send_acl = NULL;
    bt_ops.send_sco = NULL;
    bt_ops.start_le_scan = bt_start_le_scan;
    bt_ops.connect_le = bt_connect_le;
    bt_ops.disconnect = bt_disconnect;
    bt_ops.set_scan_params = NULL;
    bt_ops.set_adv_params = bt_set_adv_params;
    bt_ops.set_le_tx_power = bt_set_le_tx_power;
    bt_ops.irq_handler = bt_irq_handler;
    bt_ops.led_on = bt_led_on;
    bt_ops.led_off = bt_led_off;

    bt_hci.driver_data = &bt_ops;
    bt_hci.initialized = 1;

    /* Send initial setup commands */
    bt_hci_send_command(&bt_hci, BT_HCI_OP_SET_EVENT_MASK, NULL, 0);
    bt_hci_send_command(&bt_hci, BT_HCI_OP_RESET, NULL, 0);

    printk(KERN_INFO "BT: HCI initialized (vendor=0x%04x dev=0x%04x bd_addr=%02x:%02x:%02x:%02x:%02x:%02x)\n",
           pci_dev->vendor_id, pci_dev->device_id,
           bt_hci.bd_addr[0], bt_hci.bd_addr[1], bt_hci.bd_addr[2],
           bt_hci.bd_addr[3], bt_hci.bd_addr[4], bt_hci.bd_addr[5]);
}

void bt_led_on(bt_hci_dev_t *dev) {
    if (!dev) return;
    bt_write_reg(0x0030, 0x01);
    printk(KERN_DEBUG "BT: LED on\n");
}

void bt_led_off(bt_hci_dev_t *dev) {
    if (!dev) return;
    bt_write_reg(0x0030, 0x00);
    printk(KERN_DEBUG "BT: LED off\n");
}
