/* Minimal stub for USB UVC (USB Video Class) driver implementation */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Placeholder structure representing a UVC device */
struct usb_uvc_device {
    int dummy;
};

/* Initialization function for the UVC driver */
int usb_uvc_init(void) {
    return 0;
}

/* Cleanup function for the UVC driver */
int usb_uvc_exit(void) {
    return 0;
}

/* Register a UVC device with the core USB subsystem */
int usb_uvc_register_device(struct usb_uvc_device *dev) {
    (void)dev;
    return 0;
}

/* Unregister a UVC device */
int usb_uvc_unregister_device(struct usb_uvc_device *dev) {
    (void)dev;
    return 0;
}

#ifdef __cplusplus
}
#endif
