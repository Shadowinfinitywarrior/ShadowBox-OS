#include "sys.h"
#include <string.h>

static void print(const char *s) { sb_push(1, s, strlen(s)); }
static void newline(void) { print("\n"); }

int _start(void) {
    print("Disk Inspector:\n");
    int fd = sb_acquire("/sys/block_info", 0);
    if (fd < 0) {
        print("Failed to open /sys/block_info\n");
        sb_terminate(1);
    }
    char buf[1024];
    while (1) {
        int r = sb_pull(fd, buf, sizeof(buf));
        if (r <= 0) break;
        sb_push(1, buf, r);
    }
    sb_release(fd);
    sb_terminate(0);
    return 0; // never reached
}
