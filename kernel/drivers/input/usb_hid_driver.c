/* USB HID Driver for ShadowBox */
#include "usb.h"
#include "hid.h"
#include "kernel.h"
#include "driver.h"

extern bus_type_t usb_bus_type;

extern driver_t usb_hid_driver;



static int usb_hid_probe(device_t *dev) {
    (void)dev;
    printk(KERN_INFO "USB HID Device Attached\n");
    return 0;
}

static int usb_hid_remove(device_t *dev) {
    (void)dev;
    printk(KERN_INFO "USB HID Device Removed\n");
    return 0;
}

void usb_hid_driver_init(void) {
    printk(KERN_INFO "USB HID Driver initialized\n");
    // Register the driver
    usb_hid_driver.bus = &usb_bus_type;
    driver_register(&usb_hid_driver);
}

/* Driver registration structure for USB HID */
driver_t usb_hid_driver = {
    "usb_hid",
    usb_hid_probe,
    usb_hid_remove,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
