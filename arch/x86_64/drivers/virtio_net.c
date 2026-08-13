#include "virtio_net.h"
#include "net.h"
#include "kernel.h"
#include "pci.h"
#include "kstring.h"

static net_device_t virtio_dev;

static int virtio_net_send_packet(net_device_t *dev, void *data, uint32_t len) {
    (void)dev;
    (void)data;
    // Stub implementation: pretend to send the packet.
    return len;
}

void virtio_net_init(pci_device_t *pci_dev) {
    if (!pci_dev) return;
    // For now, use a static placeholder MAC address.
    uint8_t mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    memset(&virtio_dev, 0, sizeof(virtio_dev));
    virtio_dev.name = "virtio_net";
    memcpy(virtio_dev.mac, mac, 6);
    virtio_dev.ip = 0x0A000001; // 10.0.0.1
    virtio_dev.netmask = 0x00FFFFFF; // 255.255.255.0
    virtio_dev.gateway = 0x0A0000FE; // 10.0.0.254
    virtio_dev.send_packet = virtio_net_send_packet;
    net_register_device(&virtio_dev);
    printk(KERN_INFO "VIRTIO_NET: Initialized (MAC %x:%x:%x:%x:%x:%x)\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
