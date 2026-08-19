// SPDX-License-Identifier: GPL-2.0
/*
 * Minimal Bluetooth driver stub for the Shadowbox OS kernel.
 *
 * This file provides placeholder symbols so that the build system can
 * compile without pulling in a full Bluetooth stack.  The implementation is
 * deliberately empty – real functionality will be added later.
 */

#include "kernel.h"
#include "net.h"
#include "kstring.h"
#include "spinlock.h"
#include "errno.h"

static bt_device_t *bluetooth_devices = NULL;

/* Initialise the Bluetooth subsystem. Called from kernel start‑up.
 * The function simply prints a message so that the build verifies the
 * symbol exists.
 */
void bluetooth_init(void)
{
    printk(KERN_INFO "BLUETOOTH: stub initialisation – no hardware support\n");
    /* No actual hardware probing – placeholder only */
}

/* Register a Bluetooth device. Returns 0 on success, -EINVAL on bad args.
 * The stub merely adds the device to a linked list.
 */
int bluetooth_register_device(bt_device_t *dev)
{
    if (!dev || !dev->name)
        return -EINVAL;

    dev->next = bluetooth_devices;
    bluetooth_devices = dev;
    return 0;
}

/* Send a packet over Bluetooth – stub implementation.
 * Returns the number of bytes "sent" (always 0) and prints a debug line.
 */
int bluetooth_send_packet(bt_device_t *dev, void *data, uint32_t len)
{
    (void)dev; (void)data; (void)len;
    printk(KERN_DEBUG "BLUETOOTH: send_packet stub called (len=%u)\n", len);
    return 0; // No real transmission performed
}

/* Receive a packet – stub returns -ENODEV to indicate no device.
 */
int bluetooth_receive_packet(bt_device_t *dev, void *buffer, uint32_t max_len)
{
    (void)dev; (void)buffer; (void)max_len;
    return -ENODEV; // Not implemented
}

/* Clean‑up function – currently a no‑op. */
void bluetooth_cleanup(void)
{
    printk(KERN_INFO "BLUETOOTH: cleanup stub\n");
}

/* Return the number of registered Bluetooth devices. */
uint32_t bluetooth_device_count(void)
{
    uint32_t count = 0;
    bt_device_t *dev = bluetooth_devices;

    while (dev) {
        count++;
        dev = dev->next;
    }
    return count;
}
