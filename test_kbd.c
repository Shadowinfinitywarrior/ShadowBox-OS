#include "hid_kbd.h"
#include "hid.h"
#include "input.h"

int main(){
    hid_device_t dev = {0};
    dev.transport = HID_TRANSPORT_PS2;
    // Simulate right Ctrl press: E0 prefix then 0x1D press (no 0x80)
    uint8_t report1[1] = {0xE0};
    hid_kbd_process_report(&dev, report1, 1);
    uint8_t report2[1] = {0x1D};
    hid_kbd_process_report(&dev, report2, 1);
    // After press, current_modifiers should have CTRL bit set (0x2)
    // Release right Ctrl (add 0x80)
    uint8_t report3[1] = {0x9D}; // 0x80|0x1D
    hid_kbd_process_report(&dev, report3, 1);
    // Test Escape key press (no modifiers)
    uint8_t esc[1] = {0x01};
    hid_kbd_process_report(&dev, esc, 1);
    return 0;
}
