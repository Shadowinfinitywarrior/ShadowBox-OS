// Minimal IPv6 stub implementation for kernel build.
// This file provides placeholder definitions to satisfy compilation.
// No functional IPv6 support is implemented.

#include "net.h"

// Initialize IPv6 subsystem (stub).
void ipv6_init(void) {
    // No operation – placeholder for future IPv6 support.
}

// Handle an incoming IPv6 packet (stub).
// Parameters match typical IPv6 handler signatures; currently does nothing.
void ipv6_handle_packet(net_device_t *dev, uint8_t *packet, uint32_t len) {
    (void)dev;
    (void)packet;
    (void)len;
    // Stub implementation – packet is ignored.
}
