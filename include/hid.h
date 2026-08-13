#ifndef SHADOWBOX_HID_H
#define SHADOWBOX_HID_H

#include "types.h"
#include "device.h"
#include "input.h"

/* HID Core Layer */

typedef enum {
    HID_TYPE_UNKNOWN = 0,
    HID_TYPE_KEYBOARD,
    HID_TYPE_MOUSE,
    HID_TYPE_TOUCHPAD,
    HID_TYPE_JOYSTICK,
    HID_TYPE_GENERIC
} hid_type_t;

typedef enum {
    HID_TRANSPORT_USB,
    HID_TRANSPORT_I2C,
    HID_TRANSPORT_PS2,
    HID_TRANSPORT_BLUETOOTH
} hid_transport_t;

typedef struct hid_device {
    device_t *base_dev;
    hid_type_t type;
    hid_transport_t transport;
    
    /* Report Descriptor Parser Data */
    uint8_t *report_desc;
    uint32_t report_desc_size;
    
    int (*get_report)(struct hid_device *dev, uint8_t report_id, uint8_t *buf, size_t len);
    int (*set_report)(struct hid_device *dev, uint8_t report_id, const uint8_t *buf, size_t len);
    
    void *driver_data;
    struct hid_device *next;
} hid_device_t;

void hid_core_init(void);
int hid_register_device(hid_device_t *hdev);
void hid_unregister_device(hid_device_t *hdev);

/* Route an incoming report from the transport layer to the appropriate subsystem */
void hid_process_report(hid_device_t *hdev, uint8_t *report, size_t len);


/* 
 * Keyboard Subsystem 
 * Features: Scancode Set 1/2/3, Keymap, Dead Keys, Modifiers, Hotkeys, LED ctrl, Repeat rate
 */
typedef struct {
    uint8_t scancode_set;     // 1, 2, or 3
    uint16_t repeat_rate_ms;
    uint16_t repeat_delay_ms;
    uint8_t leds;             // NumLock, CapsLock, ScrollLock
    uint8_t modifiers;        // Shift, Ctrl, Alt, Meta
} kbd_subsystem_t;

void hid_kbd_init(void);
void hid_kbd_process_report(hid_device_t *hdev, uint8_t *report, size_t len);


/* 
 * Touchpad / Trackpad Subsystem 
 * Features: Precision Touchpad (Microsoft PTP spec), Synaptics/ALPS/Elan protocols,
 * Multi-finger support, Pressure sensitivity, Palm detection, Gesture engine.
 */
typedef struct {
    uint8_t active_fingers;
    uint8_t palm_detected;
    uint16_t pressure_threshold;
    // Protocol e.g. PTP, Synaptics, Elan, ALPS
    uint8_t protocol;
} touchpad_subsystem_t;

void hid_touchpad_init(void);
void hid_touchpad_process_report(hid_device_t *hdev, uint8_t *report, size_t len);


/* 
 * Pointer Subsystem 
 * Features: Absolute coords, Relative motion, Button states, Scroll axis
 */
typedef struct {
    int32_t x, y;
    uint32_t buttons;
    int32_t scroll_x, scroll_y;
    uint8_t is_absolute;
} pointer_subsystem_t;

void hid_pointer_init(void);
void hid_pointer_process_report(hid_device_t *hdev, uint8_t *report, size_t len);

#endif
