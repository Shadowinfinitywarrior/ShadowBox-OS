#include "e1000.h"
#include "pci.h"
#include "kernel.h"
#include "net.h"
#include "vmm.h"
#include "pmm.h"
#include "kstring.h"
#include "io.h"

static volatile uint8_t *e1000_mmio = NULL;
static uint8_t e1000_mac[6] = {0};
static int e1000_irq = 0;
static int e1000_link_up = 0;

static struct e1000_tx_desc *tx_descs_virt = NULL;
static uint64_t tx_descs_phys = 0;
static uint8_t *tx_buffers_virt[NUM_TX_DESC];
static uint64_t tx_buffers_phys[NUM_TX_DESC];
static int tx_cur = 0;
static int tx_avail = NUM_TX_DESC;

static struct e1000_rx_desc *rx_descs_virt = NULL;
static uint64_t rx_descs_phys = 0;
static uint8_t *rx_buffers_virt[NUM_RX_DESC];
static uint64_t rx_buffers_phys[NUM_RX_DESC];
static int rx_cur = 0;

static net_device_t e1000_dev;

static inline uint32_t e1000_read(uint16_t reg) {
    return *(volatile uint32_t *)(e1000_mmio + reg);
}

static inline void e1000_write(uint16_t reg, uint32_t val) {
    *(volatile uint32_t *)(e1000_mmio + reg) = val;
}

static int e1000_send_packet(net_device_t *dev, void *data, uint32_t len) {
    (void)dev;
    if (!e1000_link_up) return 0;
    if (len > RX_BUF_SIZE) return 0;

    int desc = tx_cur;
    if (!tx_avail) return 0;

    for (int i = 0; i < len; i++)
        tx_buffers_virt[desc][i] = ((uint8_t *)data)[i];
    if (len < 60) {
        for (int i = len; i < 60; i++)
            tx_buffers_virt[desc][i] = 0;
        len = 60;
    }

    tx_descs_virt[desc].length = len;
    tx_descs_virt[desc].cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    tx_descs_virt[desc].status = 0;

    tx_cur = (tx_cur + 1) % NUM_TX_DESC;
    tx_avail--;

    e1000_write(E1000_TDT, tx_cur);

    for (int timeout = 0; timeout < 10000; timeout++) {
        if (tx_descs_virt[desc].status & E1000_TXD_STAT_DD) {
            tx_avail++;
            return len;
        }
        __builtin_ia32_pause();
    }
    tx_avail++;
    return len;
}

void e1000_irq_handler(void) {
    uint32_t icr = e1000_read(E1000_ICR);
    if (!icr) return;

    if (icr & E1000_ICR_LSC) {
        uint32_t status = e1000_read(E1000_STATUS);
        e1000_link_up = (status & E1000_STATUS_LU) ? 1 : 0;
        printk(KERN_INFO "E1000: Link %s\n", e1000_link_up ? "UP" : "DOWN");
    }

    if (icr & E1000_ICR_RXT0) {
        while (rx_descs_virt[rx_cur].status & E1000_RXD_STAT_DD) {
            uint16_t len = rx_descs_virt[rx_cur].length;
            if (len > 0 && (rx_descs_virt[rx_cur].status & E1000_RXD_STAT_EOP)) {
                net_handle_packet(&e1000_dev, rx_buffers_virt[rx_cur], len);
            }
            rx_descs_virt[rx_cur].status = 0;
            e1000_write(E1000_RDT, rx_cur);
            rx_cur = (rx_cur + 1) % NUM_RX_DESC;
        }
    }

    if (icr & E1000_ICR_RXO) {
        rx_cur = e1000_read(E1000_RDH);
    }
}

void e1000_init(pci_device_t *pci_dev) {
    if (!pci_dev) { printk(KERN_ERR "E1000: No PCI device\n"); return; }

    uint32_t bar0 = pci_config_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x10);
    uint64_t mmio_phys = bar0 & ~0xF;
    if ((bar0 & 0x6) == 0x4) { // 64-bit BAR
        uint32_t bar0_upper = pci_config_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x14);
        mmio_phys |= ((uint64_t)bar0_upper << 32);
    }
    uint64_t mmio_kva = 0xFFFFC00000000000ULL + mmio_phys;
    for (uint32_t off = 0; off < 0x20000; off += 0x1000)
        vmm_map_page(mmio_phys + off, mmio_kva + off, PAGE_PRESENT | PAGE_WRITE);
    {
        uint64_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
    }

    e1000_mmio = (volatile uint8_t *)mmio_kva;
    e1000_irq = pci_dev->irq_line;
    pci_enable_bus_mastering(pci_dev);

    printk(KERN_INFO "E1000: MMIO at phys=%p virt=%p IRQ=%d\n", (void*)mmio_phys, (void*)mmio_kva, e1000_irq);

    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | E1000_CTRL_RST);
    for (volatile int i = 0; i < 100000; i++) __builtin_ia32_pause();

    e1000_write(E1000_CTRL, E1000_CTRL_SLU | E1000_CTRL_ASDE);
    for (volatile int i = 0; i < 10000; i++) __builtin_ia32_pause();

    uint32_t status = e1000_read(E1000_STATUS);
    e1000_link_up = (status & E1000_STATUS_LU) ? 1 : 0;
    printk(KERN_INFO "E1000: Status=0x%x Link=%s\n", status, e1000_link_up ? "UP" : "DOWN");

    uint32_t mac_low = e1000_read(E1000_RAL);
    uint32_t mac_high = e1000_read(E1000_RAH);
    e1000_mac[0] = mac_low & 0xFF;
    e1000_mac[1] = (mac_low >> 8) & 0xFF;
    e1000_mac[2] = (mac_low >> 16) & 0xFF;
    e1000_mac[3] = (mac_low >> 24) & 0xFF;
    e1000_mac[4] = mac_high & 0xFF;
    e1000_mac[5] = (mac_high >> 8) & 0xFF;
    printk(KERN_INFO "E1000: MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           e1000_mac[0], e1000_mac[1], e1000_mac[2],
           e1000_mac[3], e1000_mac[4], e1000_mac[5]);

    tx_descs_phys = (uint64_t)pmm_alloc_page();
    tx_descs_virt = (struct e1000_tx_desc *)(tx_descs_phys + 0xFFFFFFFF80000000);
    memset(tx_descs_virt, 0, 4096);

    for (int i = 0; i < NUM_TX_DESC; i++) {
        tx_buffers_phys[i] = (uint64_t)pmm_alloc_page();
        tx_buffers_virt[i] = (uint8_t *)(tx_buffers_phys[i] + 0xFFFFFFFF80000000);
        memset(tx_buffers_virt[i], 0, 4096);
        tx_descs_virt[i].addr = tx_buffers_phys[i];
    }

    e1000_write(E1000_TDBAL, (uint32_t)(tx_descs_phys & 0xFFFFFFFF));
    e1000_write(E1000_TDBAH, (uint32_t)(tx_descs_phys >> 32));
    e1000_write(E1000_TDLEN, NUM_TX_DESC * sizeof(struct e1000_tx_desc));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);

    rx_descs_phys = (uint64_t)pmm_alloc_page();
    rx_descs_virt = (struct e1000_rx_desc *)(rx_descs_phys + 0xFFFFFFFF80000000);
    memset(rx_descs_virt, 0, 4096);

    for (int i = 0; i < NUM_RX_DESC; i++) {
        rx_buffers_phys[i] = (uint64_t)pmm_alloc_page();
        rx_buffers_virt[i] = (uint8_t *)(rx_buffers_phys[i] + 0xFFFFFFFF80000000);
        rx_descs_virt[i].addr = rx_buffers_phys[i];
    }

    e1000_write(E1000_RDBAL, (uint32_t)(rx_descs_phys & 0xFFFFFFFF));
    e1000_write(E1000_RDBAH, (uint32_t)(rx_descs_phys >> 32));
    e1000_write(E1000_RDLEN, NUM_RX_DESC * sizeof(struct e1000_rx_desc));
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, NUM_RX_DESC - 1);

    e1000_write(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_UPE |
               E1000_RCTL_MPE | E1000_RCTL_BAM | E1000_RCTL_BSIZE_2K);
    e1000_write(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP |
               (0x10 << E1000_TCTL_CT_SHIFT) |
               (0x40 << E1000_TCTL_COLD_SHIFT));
    e1000_write(E1000_TIPG, 0x0060200A);

    e1000_write(E1000_IMS, E1000_ICR_RXT0 | E1000_ICR_RXO | E1000_ICR_LSC);

    memset(&e1000_dev, 0, sizeof(net_device_t));
    e1000_dev.name = "e1000";
    memcpy(e1000_dev.mac, e1000_mac, 6);
    e1000_dev.ip = 0x0A000001;
    e1000_dev.netmask = 0x00FFFFFF;
    e1000_dev.gateway = 0x0A0000FE;
    e1000_dev.send_packet = e1000_send_packet;
    net_register_device(&e1000_dev);

    printk(KERN_INFO "E1000: Driver initialized\n");
}
