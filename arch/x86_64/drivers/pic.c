#include "pic.h"
#include "kernel.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20

#define ICW1_ICW4      0x01
#define ICW1_INIT      0x10
#define ICW4_8086      0x01

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_init(void) {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    // Starts the initialization sequence (in cascade mode)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // ICW2: Master PIC vector offset
    outb(PIC1_DATA, 0x20); // IRQ 0..7 mapped to INT 32..39
    // ICW2: Slave PIC vector offset
    outb(PIC2_DATA, 0x28); // IRQ 8..15 mapped to INT 40..47

    // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 4);
    // ICW3: tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 2);

    // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Unmask IRQ0 (Timer), IRQ1 (Keyboard), and IRQ2 (Cascade to Slave PIC)
    outb(PIC1_DATA, 0xF8);
    outb(PIC2_DATA, 0xFF);
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
        // CRITICAL: When unmasking slave IRQ, also unmask cascade IRQ 2 on master
        value = inb(PIC1_DATA) & ~(1 << 2);
        outb(PIC1_DATA, value);
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}
