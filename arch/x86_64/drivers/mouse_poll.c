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
    static int poll_count = 0;
    printk(KERN_DEBUG "MOUSE POLL THREAD STARTING\n");
    int loop = 0;
    while (1) {
        uint64_t flags;
        __asm__ volatile("pushfq\npop %0\ncli\n" : "=r"(flags) : : "memory");
        uint8_t status = inb(PS2_CMD);
        if ((status & PS2_STATUS_OUTPUT) && (status & PS2_STATUS_AUX)) {
            uint8_t b = inb(PS2_DATA);
            if (poll_count < 10) {
                printk(KERN_DEBUG "MOUSE POLL byte=0x%02x status=0x%02x\n", b, status);
                poll_count++;
            }
            mouse_process_byte(b);
        } else if (loop < 3 || (status != 0 && poll_count < 10)) {
            if (poll_count < 10) {
                printk(KERN_DEBUG "MOUSE POLL status=0x%02x OUT=%d AUX=%d\n", status, !!(status & PS2_STATUS_OUTPUT), !!(status & PS2_STATUS_AUX));
                poll_count++;
            }
        }
        __asm__ volatile("push %0\npopfq\n" : : "r"(flags) : "memory");
        for (volatile int i = 0; i < 1000; ++i) {
            __asm__ volatile("pause" ::: "memory");
        }
        loop++;
    }
}

// Export symbol for thread creation.
void mouse_start_poll_thread(void) {
    extern struct process* kthread_create(void (*entry)(void*), void *arg, const char *name);
    kthread_create(mouse_poll_thread, NULL, "mouse-poll");
}
