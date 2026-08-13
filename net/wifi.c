// Minimal stub for Wi‑Fi driver
// This provides just enough symbols so the kernel can compile
// and register a dummy network device. No actual hardware support.

#include "net.h"
#include "kernel.h"

// Dummy send_packet implementation – does nothing and returns success.
static int wifi_send_packet(net_device_t *dev, void *data, uint32_t len)
{
    (void)dev; (void)data; (void)len;
    // In a real driver this would transmit over the Wi‑Fi hardware.
    return 0;
}

// Define a static network device descriptor for the Wi‑Fi interface.
static net_device_t wifi_device = {
    .name = "wifi0",
    .mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}, // locally administered MAC
    .ip = 0,          // No IP assigned yet
    .netmask = 0,
    .gateway = 0,
    .send_packet = wifi_send_packet,
    .next = NULL,
};

// Stub packet handler – simply discards packets.
void wifi_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len)
{
    (void)dev; (void)packet; (void)len;
    // No processing performed.
}

// Initialization function called from the kernel startup.
void wifi_init(void)
{
    printk(KERN_INFO "WIFI: Initializing dummy Wi‑Fi driver...\n");
    net_register_device(&wifi_device);
}
