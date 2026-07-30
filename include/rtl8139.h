#ifndef SHADOWBOX_RTL8139_H
#define SHADOWBOX_RTL8139_H

#include "types.h"
#include "pci.h"
#include "net.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

#define RTL8139_RBSTART   0x30
#define RTL8139_CMD       0x37
#define RTL8139_CAPR      0x38
#define RTL8139_IMR       0x3C
#define RTL8139_ISR       0x3E
#define RTL8139_TCR       0x40
#define RTL8139_RCR       0x44
#define RTL8139_CONFIG1   0x52
#define RTL8139_TX_START  0x20
#define RTL8139_TX_STATUS 0x10
#define RTL8139_RX_BUF    0x1000

#define RX_BUF_LEN 8192

void rtl8139_init(pci_device_t *pci_dev);
int rtl8139_send_packet(net_device_t *dev, void *data, uint32_t len);
void rtl8139_handle_irq(void);

#endif
