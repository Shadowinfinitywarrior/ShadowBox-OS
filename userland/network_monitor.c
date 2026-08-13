#include "sys.h"
#include <string.h>

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

void _start(void) {
    // Header for Network Monitor
    const char *header = "Network Monitor\n--------------------\n";
    print(header);

    // Try to read network info from sysfs
    int fd = sb_acquire("/sys/net_info", 0);
    if (fd >= 0) {
        char buf[4096];
        uint64_t total = 0;
        uint64_t n;
        while ((n = sb_pull(fd, buf + total, sizeof(buf) - total)) > 0) {
            total += n;
            if (total >= sizeof(buf)) break;
        }
        sb_release(fd);
        if (total > 0) {
            sb_push(1, buf, total);
        } else {
            const char *msg = "No network devices found.\n";
            print(msg);
        }
    } else {
        const char *msg = "Unable to open /sys/net_info.\n";
        print(msg);
    }

    sb_terminate(0);
}
