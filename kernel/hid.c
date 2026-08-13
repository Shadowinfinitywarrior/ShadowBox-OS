#include "hid.h"
#include "kernel.h"
#include "malloc.h"
#include "hid_kbd.h"

static hid_device_t *hid_devices = NULL;

void hid_core_init(void) {
    printk(KERN_INFO "Initializing HID Core Layer\n");
    
    hid_kbd_init();
    hid_pointer_init();
    hid_touchpad_init();
}

int hid_register_device(hid_device_t *hdev) {
    if (!hdev) return -1;
    
    // Parse Report Descriptor here (stubbed for now)
    printk(KERN_INFO "HID: Registering device type=%d, transport=%d\n", hdev->type, hdev->transport);
    
    hdev->next = hid_devices;
    hid_devices = hdev;
    return 0;
}

void hid_unregister_device(hid_device_t *hdev) {
    if (!hdev) return;
    
    hid_device_t **curr = &hid_devices;
    while (*curr) {
        if (*curr == hdev) {
            *curr = hdev->next;
            break;
        }
        curr = &(*curr)->next;
    }
}

void hid_process_report(hid_device_t *hdev, uint8_t *report, size_t len) {
    if (!hdev || !report || len == 0) return;
    
    // Route report to the correct subsystem based on parsed collection
    switch (hdev->type) {
        case HID_TYPE_KEYBOARD:
            hid_kbd_process_report(hdev, report, len);
            break;
        case HID_TYPE_MOUSE:
            hid_pointer_process_report(hdev, report, len);
            break;
        case HID_TYPE_TOUCHPAD:
            hid_touchpad_process_report(hdev, report, len);
            break;
        default:
            printk(KERN_WARN "HID: Unhandled report for device type %d\n", hdev->type);
            break;
    }
}

/* --- Pointer Subsystem --- */
void hid_pointer_init(void) {
    printk(KERN_INFO "HID: Initialized Pointer Subsystem\n");
}

void hid_pointer_process_report(hid_device_t *hdev, uint8_t *report, size_t len) {
    // Process relative/absolute motion, scroll axis, button states
    (void)hdev; (void)report; (void)len;
}

/* --- Touchpad Subsystem --- */
void hid_touchpad_init(void) {
    printk(KERN_INFO "HID: Initialized Touchpad Subsystem (PTP/Synaptics)\n");
}

void hid_touchpad_process_report(hid_device_t *hdev, uint8_t *report, size_t len) {
    // Process multi-finger gestures, pressure sensitivity, palm detection
    (void)hdev; (void)report; (void)len;
}
