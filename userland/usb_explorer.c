// USB Device Explorer - lists registered USB drivers via /sys/usb
// Minimal userland utility using the ShadowBox syscalls.
// Compile with: $(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/usb_explorer.c -o usb_explorer.elf

#include "sys.h"

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

void _start(void) {
    // Open the sysfs file for USB information.
    int fd = sb_acquire("/sys/usb", 0);
    if (fd < 0) {
        print("Failed to open /sys/usb\n");
        sb_terminate(1);
    }

    char buf[256];
    while (1) {
        uint64_t r = sb_pull(fd, buf, sizeof(buf));
        if (r <= 0) break;
        sb_push(1, buf, r);
    }
    sb_release(fd);
    sb_terminate(0);
}
