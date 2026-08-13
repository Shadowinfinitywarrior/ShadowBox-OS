#include "keyboard.h"
#include "input.h"
#include "io.h"
#include "kernel.h"
#include "pic.h"
#include "apic.h"

#include "mouse.h"

extern void mouse_start_poll_thread(void);

#define PS2_STATUS_OUTPUT 0x01
#define PS2_STATUS_AUX    0x20

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
    outb(PS2_CMD, 0xD4);
    ps2_wait_write();
    outb(PS2_DATA, val);
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

void mouse_process_byte(uint8_t data) {
    if (!mouse_initialized) return;

    // First byte (cycle 0) MUST have bit 3 set (sync bit)
    if (mouse_cycle == 0 && !(data & 0x08)) {
        return;
    }

    mouse_packet[mouse_cycle] = data;
    mouse_cycle++;

    int max_cycle = has_wheel ? 4 : 3;

    if (mouse_cycle != max_cycle) return;

    mouse_cycle = 0;

    uint8_t flags = mouse_packet[0];
    if (flags & 0x40 || flags & 0x80) return;

    int dx = (int8_t)mouse_packet[1];
    int dy = (int8_t)mouse_packet[2];

    if (flags & 0x10) dx |= 0xFFFFFF00;
    if (flags & 0x20) dy |= 0xFFFFFF00;

    dy = -dy;

    if (dx != 0 || dy != 0) {
        input_push(INPUT_EVENT_MOUSE_MOVE, 0, (int16_t)dx, (int16_t)dy);
    }

    uint8_t btn_changes = (flags ^ prev_btn_state) & 0x07;
    if (btn_changes & 1) input_push(INPUT_EVENT_MOUSE_BTN, 0, (flags & 1) ? 1 : 0, 0);
    if (btn_changes & 2) input_push(INPUT_EVENT_MOUSE_BTN, 1, (flags & 2) ? 1 : 0, 0);
    if (btn_changes & 4) input_push(INPUT_EVENT_MOUSE_BTN, 2, (flags & 4) ? 1 : 0, 0);

    prev_btn_state = flags & 0x07;

    if (has_wheel) {
        int8_t wheel = (int8_t)mouse_packet[3];
        if (wheel != 0) {
            input_push(INPUT_EVENT_MOUSE_BTN, 3, 1, (int16_t)wheel);
        }
    }
}

void mouse_handler(void) {
    while ((inb(PS2_CMD) & PS2_STATUS_OUTPUT) && (inb(PS2_CMD) & PS2_STATUS_AUX)) {
        uint8_t b = inb(PS2_DATA);
        mouse_process_byte(b);
    }
}

void mouse_init(void) {
    printk(KERN_INFO "PS2: Initializing mouse...\n");

    pic_set_mask(12);

    ps2_wait_write();
    outb(PS2_CMD, 0xAD);
    ps2_wait_write();
    outb(PS2_CMD, CMD_DISABLE_P2);

    ps2_read();

    ps2_wait_write();
    outb(PS2_CMD, CMD_READ_CONFIG);
    uint8_t config = ps2_read();

    config |= 0x02;
    config &= ~0x20;

    ps2_wait_write();
    outb(PS2_CMD, CMD_WRITE_CONFIG);
    ps2_wait_write();
    outb(PS2_DATA, config);

    ps2_wait_write();
    outb(PS2_CMD, CMD_ENABLE_P2);

    ps2_wait_write();
    outb(PS2_CMD, CMD_TEST_P2);
    ps2_read();

    ps2_wait_write();
    outb(PS2_CMD, 0xAA);
    ps2_read();

    ps2_wait_write();
    outb(PS2_CMD, CMD_WRITE_CONFIG);
    ps2_wait_write();
    outb(PS2_DATA, config);

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
    outb(PS2_CMD, 0xAE);

    pic_clear_mask(12);
    ioapic_route_irq(12, 44, 0);

    printk(KERN_INFO "PS2: Mouse initialized.\n");
    mouse_start_poll_thread();
}
