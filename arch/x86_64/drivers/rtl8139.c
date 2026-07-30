#include "rtl8139.h"
#include "kernel.h"
#include "pci.h"
#include "net.h"
#include "malloc.h"
#include "kstring.h"
#include "errno.h"
#include "io.h"

static uint16_t rtl8139_io_base = 0;
static uint8_t rtl8139_mac[6];
static net_device_t rtl8139_dev;
static uint8_t *rx_buffer = 0;
static int rx_idx = 0;
static int tx_cur = 0;

static void rtl8139_reset(void) {
    outb(rtl8139_io_base + 0x52, 0x00);
    outb(rtl8139_io_base + RTL8139_CMD, 0x10);
    while (inb(rtl8139_io_base + RTL8139_CMD) & 0x10);
}

int rtl8139_send_packet(net_device_t *dev, void *data, uint32_t len) {
    (void)dev;
    if (len > 1792) return 0;
    uint32_t status = inl(rtl8139_io_base + RTL8139_TX_STATUS + tx_cur * 4);
    if (!(status & 0x2000)) {
        for (int timeout = 0; timeout < 1000; timeout++) {
            status = inl(rtl8139_io_base + RTL8139_TX_STATUS + tx_cur * 4);
            if (status & 0x2000) break;
        }
        if (!(status & 0x2000)) return 0;
    }
    outl(rtl8139_io_base + RTL8139_TX_START + tx_cur * 4, (uint32_t)(uint64_t)data);
    outl(rtl8139_io_base + RTL8139_TX_STATUS + tx_cur * 4, len | 0x8000);
    tx_cur = (tx_cur + 1) % 4;
    return len;
}

void rtl8139_handle_irq(void) {
    uint16_t status = inw(rtl8139_io_base + RTL8139_ISR);
    outw(rtl8139_io_base + RTL8139_ISR, status);
    if (status & 0x01) {
        while ((inb(rtl8139_io_base + RTL8139_CMD) & 0x01) == 0) {
            uint32_t rx_status = *(uint32_t *)(rx_buffer + rx_idx);
            uint32_t rx_len = (rx_status >> 16) & 0x3FFF;
            if (rx_len == 0xFFF0) break;
            if (rx_len > 0 && rx_len < 5000) {
                uint8_t *packet = rx_buffer + rx_idx + 4;
                net_handle_packet(&rtl8139_dev, packet, rx_len - 4);
            }
            rx_idx = (rx_idx + rx_len + 4 + 3) & ~3;
            rx_idx %= RX_BUF_LEN;
            outw(rtl8139_io_base + RTL8139_CAPR, rx_idx - 16);
        }
    }
}

void rtl8139_init(pci_device_t *pci_dev) {
    if (!pci_dev) return;
    rtl8139_io_base = pci_dev->bar0 & 0xFFFE;
    rtl8139_reset();
    for (int i = 0; i < 6; i++) {
        rtl8139_mac[i] = inb(rtl8139_io_base + i);
    }
    rx_buffer = kmalloc(RX_BUF_LEN + 16);
    memset(rx_buffer, 0, RX_BUF_LEN + 16);
    outl(rtl8139_io_base + RTL8139_RBSTART, (uint32_t)(uint64_t)rx_buffer);
    outw(rtl8139_io_base + RTL8139_IMR, 0x0005);
    outb(rtl8139_io_base + RTL8139_CMD, 0x0C);
    outb(rtl8139_io_base + RTL8139_CONFIG1, 0x00);
    outl(rtl8139_io_base + RTL8139_RCR, 0x00000F0E);
    memset(&rtl8139_dev, 0, sizeof(net_device_t));
    rtl8139_dev.name = "rtl8139";
    for (int i = 0; i < 6; i++) rtl8139_dev.mac[i] = rtl8139_mac[i];
    rtl8139_dev.ip = 0x0A000001;
    rtl8139_dev.netmask = 0x00FFFFFF;
    rtl8139_dev.gateway = 0x0A0000FE;
    rtl8139_dev.send_packet = rtl8139_send_packet;
    net_register_device(&rtl8139_dev);
    printk(KERN_INFO "RTL8139: Initialized at IO 0x%x (MAC %x:%x:%x:%x:%x:%x)\n",
           rtl8139_io_base,
           rtl8139_mac[0], rtl8139_mac[1], rtl8139_mac[2],
           rtl8139_mac[3], rtl8139_mac[4], rtl8139_mac[5]);
}
