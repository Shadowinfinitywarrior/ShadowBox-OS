#include "keyboard.h"
#include "io.h"
#include "kernel.h"
#include "pic.h"
#include "apic.h"

#define PS2_CMD  0x64
#define PS2_DATA 0x60

#define CMD_READ_CONFIG  0x20
#define CMD_WRITE_CONFIG 0x60
#define CMD_ENABLE_P2    0xA8
#define CMD_DISABLE_P2   0xA7
#define CMD_TEST_P2      0xA9

#define MOUSE_CMD_ENABLE     0xF4
#define MOUSE_CMD_DISABLE    0xF5
#define MOUSE_CMD_SET_DEFAULTS 0xF6
#define MOUSE_CMD_SET_SAMPLE 0xF3
#define MOUSE_CMD_GET_ID     0xF2
#define MOUSE_CMD_SET_RES    0xE8

#define AUX_BUF 3
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[4];
static uint8_t prev_btn_state = 0;
static int has_wheel = 0;

static int mouse_initialized = 0;

static void ps2_wait_write(void) {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(PS2_CMD) & 2)) return;
    }
}

static void ps2_wait_read(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_CMD) & 1) return;
    }
}

static uint8_t ps2_read(void) {
    ps2_wait_read();
    return inb(PS2_DATA);
}

static void mouse_write(uint8_t val) {
    ps2_wait_write();
    outb(0xD4, PS2_CMD);
    ps2_wait_write();
    outb(val, PS2_DATA);
}

static uint8_t mouse_read_ack(void) {
    for (int i = 0; i < 10000; i++) {
        uint8_t ack = ps2_read();
        if (ack == 0xFA) return 1;  // ACK
        if (ack == 0xFE) return 2;  // Resend
    }
    return 0;  // Timeout
}

static uint8_t mouse_write_with_retry(uint8_t val) {
    int retries = 5;
    while (retries--) {
        mouse_write(val);
        uint8_t ack = mouse_read_ack();
        if (ack == 1) return 1;  // Success
        if (ack != 2) return ack; // Error/timeout, not resend
        // ack == 2 (resend) - retry the command
        for (volatile int i = 0; i < 50000; i++);  // Small delay
    }
    return 0;  // Failed after retries
}

void mouse_handler(void) {
    uint8_t data = inb(PS2_DATA);

    if (!mouse_initialized) return;

    // CRITICAL: First byte (cycle 0) MUST have bit 3 set (sync bit)
    // If not, we're out of sync - discard and wait for next sync byte
    if (mouse_cycle == 0 && !(data & 0x08)) {
        printk(KERN_WARN "[MOUSE] OUT OF SYNC, discarding byte=0x%x\n", data);
        return; // Discard out-of-sync byte
    }

    mouse_packet[mouse_cycle] = data;
    mouse_cycle++;

    int max_cycle = has_wheel ? 4 : 3;

    if (mouse_cycle == max_cycle) {
        mouse_cycle = 0;

        uint8_t flags = mouse_packet[0];
        // Check overflow bits - discard packet if overflow
        if (flags & 0x40 || flags & 0x80) {
            printk(KERN_WARN "[MOUSE] Packet overflow, discarding\n");
            return;
        }

        int dx = (int8_t)mouse_packet[1];
        int dy = (int8_t)mouse_packet[2];

        // Sign extend using overflow bits
        if (flags & 0x10) dx |= 0xFFFFFF00;
        if (flags & 0x20) dy |= 0xFFFFFF00;

        // PS/2 Y is inverted
        dy = -dy;

        if (dx != 0 || dy != 0) {
            input_push(2, 0, (int16_t)dx, (int16_t)dy);
        }

        // Button events: use button index (0=left, 1=right, 2=middle)
        uint8_t btn_changes = (flags ^ prev_btn_state) & 0x07;
        if (btn_changes & 1) input_push(3, 0, 0, (flags & 1));
        if (btn_changes & 2) input_push(3, 0, 1, (flags & 2) ? 1 : 0);
        if (btn_changes & 4) input_push(3, 0, 2, (flags & 4) ? 1 : 0);

        prev_btn_state = flags & 0x07;

        // Scroll wheel (IntelliMouse 4th byte)
        if (has_wheel) {
            int8_t wheel = (int8_t)mouse_packet[3];
            if (wheel != 0) {
                input_push(3, 3, 0, (int16_t)wheel);
            }
        }
    }
}

void mouse_init(void) {
    printk(KERN_INFO "PS2: Initializing mouse...\n");

    ps2_wait_write();
    outb(0xAD, PS2_CMD);
    ps2_wait_write();
    outb(CMD_DISABLE_P2, PS2_CMD);

    ps2_read();

    ps2_wait_write();
    outb(CMD_READ_CONFIG, PS2_CMD);
    uint8_t config = ps2_read();

    config |= 0x02;
    config &= ~0x20;

    ps2_wait_write();
    outb(CMD_WRITE_CONFIG, PS2_CMD);
    ps2_wait_write();
    outb(config, PS2_DATA);

    ps2_wait_write();
    outb(CMD_ENABLE_P2, PS2_CMD);

    ps2_wait_write();
    outb(CMD_TEST_P2, PS2_CMD);
    ps2_read();

    ps2_wait_write();
    outb(0xAA, PS2_CMD);
    ps2_read();

    ps2_wait_write();
    outb(CMD_WRITE_CONFIG, PS2_CMD);
    ps2_wait_write();
    outb(config, PS2_DATA);

    mouse_write_with_retry(MOUSE_CMD_SET_DEFAULTS);

    // Try to enable IntelliMouse (scroll wheel) by setting sample rate sequence
    mouse_write_with_retry(MOUSE_CMD_DISABLE);

    mouse_write_with_retry(MOUSE_CMD_SET_SAMPLE);
    mouse_write_with_retry(200);
    mouse_write_with_retry(MOUSE_CMD_SET_SAMPLE);
    mouse_write_with_retry(100);
    mouse_write_with_retry(MOUSE_CMD_SET_SAMPLE);
    mouse_write_with_retry(80);

    // Get mouse ID to check for IntelliMouse
    mouse_write_with_retry(MOUSE_CMD_GET_ID);
    uint8_t id = ps2_read();
    if (id == 0x03) {
        has_wheel = 1;
        printk(KERN_INFO "PS2: IntelliMouse (scroll wheel) detected.\n");
    } else {
        has_wheel = 0;
        printk(KERN_INFO "PS2: Standard mouse (no scroll wheel).\n");
        // Re-initialize for standard mouse
        mouse_write_with_retry(MOUSE_CMD_SET_DEFAULTS);
    }

    mouse_write_with_retry(MOUSE_CMD_SET_RES);
    mouse_write_with_retry(3);

    mouse_write_with_retry(MOUSE_CMD_SET_SAMPLE);
    mouse_write_with_retry(100);

    mouse_write_with_retry(MOUSE_CMD_ENABLE);

    mouse_initialized = 1;
    mouse_cycle = 0;
    prev_btn_state = 0;
    mouse_packet[0] = 0;
    mouse_packet[1] = 0;
    mouse_packet[2] = 0;
    mouse_packet[3] = 0;

    ps2_wait_write();
    outb(0xAE, PS2_CMD);

    pic_clear_mask(12);
    ioapic_route_irq(12, 44, 0);

    printk(KERN_INFO "PS2: Mouse initialized.\n");
}
