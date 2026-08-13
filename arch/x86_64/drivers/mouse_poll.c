#include "mouse.h"
#include "io.h"
#include "kernel.h"
#include "input.h"

// Simple mouse polling thread for environments where PS/2 mouse IRQs are not delivered.
// Uses direct port polling of the i8042 controller to fetch pending mouse bytes.

#define PS2_CMD 0x64
#define PS2_DATA 0x60
#define PS2_STATUS_OUTPUT 0x01
#define PS2_STATUS_AUX 0x20

static void mouse_poll_thread(void *arg) {
(void)arg;
while (1) {
uint8_t status = inb(PS2_CMD);
if ((status & PS2_STATUS_OUTPUT) && (status & PS2_STATUS_AUX)) {
uint8_t b = inb(PS2_DATA);
printk(KERN_INFO "mouse_poll: byte %x\n", b);
mouse_process_byte(b);
}
// Simple delay to avoid busy-wait hogging the CPU.
for (volatile int i = 0; i < 1000; ++i) {
__asm__ volatile("pause" ::: "memory");
}
}
}

// Export symbol for thread creation.
void mouse_start_poll_thread(void) {
extern struct process* kthread_create(void (*entry)(void*), void *arg, const char *name);
kthread_create(mouse_poll_thread, NULL, "mouse-poll");
}