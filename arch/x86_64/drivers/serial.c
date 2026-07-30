#include "types.h"
#include "io.h"

#define COM1 0x3f8

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 4, 0x0B);
    // enable FIFO without clearing it
    outb(COM1 + 2, 0x83);
    // drain stale input
    while (inb(COM1 + 5) & 1) inb(COM1);
}

int serial_is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_write_char(char a) {
    while (serial_is_transmit_empty() == 0);
    outb(COM1, a);
}

int serial_received(void) {
    return inb(COM1 + 5) & 1;
}

char serial_read_char(void) {
    while (serial_received() == 0);
    return inb(COM1);
}
