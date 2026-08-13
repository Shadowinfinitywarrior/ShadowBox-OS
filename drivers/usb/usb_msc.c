/* Minimal stub for USB MSC (Mass Storage Class) driver */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Placeholder structure for MSC device */
struct usb_msc_device {
    int dummy;
};

/* Initialize MSC subsystem */
int usb_msc_init(void) {
    return 0;
}

/* Cleanup MSC subsystem */
int usb_msc_exit(void) {
    return 0;
}

/* Register an MSC device */
int usb_msc_register(struct usb_msc_device *dev) {
    (void)dev;
    return 0;
}

/* Unregister an MSC device */
int usb_msc_unregister(struct usb_msc_device *dev) {
    (void)dev;
    return 0;
}

#ifdef __cplusplus
}
#endif
