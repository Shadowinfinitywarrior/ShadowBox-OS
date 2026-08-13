/*
 * Minimal stub for XHCI USB driver
 * This file provides placeholder definitions to satisfy compilation.
 * No functional implementation is provided.
 */

#include <stddef.h>

/* Forward declaration of driver structures (placeholders) */
struct xhci_controller {
    void *base_address; /* Memory-mapped I/O base */
    int   max_slots;
};

struct xhci_endpoint {
    int id;
    /* ... other fields omitted ... */
};

/* Stub initialization function */
static int xhci_init(void) {
    /* Normally would set up the controller, allocate resources, etc. */
    return 0; /* Success */
}

/* Stub probe function – called when a compatible device is found */
static int xhci_probe(void *dev) {
    (void)dev; /* suppress unused parameter warning */
    return xhci_init();
}

/* Stub remove function – cleanup resources */
static void xhci_remove(void *dev) {
    (void)dev; /* suppress unused parameter warning */
    /* No resources to free in stub */
}

/* Exported driver structure – adjust as needed by the kernel build system */
struct driver {
    const char *name;
    int (*probe)(void *dev);
    void (*remove)(void *dev);
};

static const struct driver xhci_driver = {
    .name   = "xhci",
    .probe  = xhci_probe,
    .remove = xhci_remove,
};

/* Registration macro – replace with actual kernel registration if available */
__attribute__((unused)) static const struct driver *registered_xhci_driver = &xhci_driver;
