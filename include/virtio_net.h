#ifndef SHADOWBOX_VIRTIO_NET_H
#define SHADOWBOX_VIRTIO_NET_H

#include "types.h"
#include "pci.h"
#include "net.h"

#define VIRTIO_NET_VENDOR_ID 0x1AF4
#define VIRTIO_NET_DEVICE_ID 0x1000

void virtio_net_init(pci_device_t *pci_dev);

#endif // SHADOWBOX_VIRTIO_NET_H
