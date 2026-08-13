/*
 * EHCI (Enhanced Host Controller Interface) stub driver
 * This file provides a minimal placeholder implementation to satisfy
 * compilation of the kernel build. Real functionality is not included.
 */

#include "usb.h"
#include "kernel.h"
#include "driver.h"

extern bus_type_t usb_bus_type;

extern driver_t ehci_driver;

/* Device probe – called when the EHCI controller is detected */
static int ehci_probe(device_t *dev) {
    (void)dev; // unused parameter placeholder
    printk(KERN_INFO "EHCI controller probed (stub)\n");
    return 0;
}

/* Device removal – called when the controller is being removed */
static int ehci_remove(device_t *dev) {
    (void)dev; // unused parameter placeholder
    printk(KERN_INFO "EHCI controller removed (stub)\n");
    return 0;
}

/* Driver initialization entry point */
void ehci_driver_init(void) {
    // Register the driver with the driver framework
    ehci_driver.bus = &usb_bus_type;
    driver_register(&ehci_driver);

    printk(KERN_INFO "EHCI driver initialized (stub)\n");
}

/* Optional driver registration structure – adjust as needed by the kernel */
driver_t ehci_driver = {
    "ehci",
    ehci_probe,
    ehci_remove,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

