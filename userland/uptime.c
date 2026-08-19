#include "sys.h"

/* uptime - how long the system has been running */

static void print(const char *s) { sb_push(1, s, strlen(s)); }
static void print_num(uint64_t v) {
    char buf[24]; int idx = 0;
    if (v == 0) buf[idx++] = '0';
    while (v > 0) { buf[idx++] = '0' + (v % 10); v /= 10; }
    while (idx > 0) sb_push(1, &buf[--idx], 1);
}

void _start(void) {
    uint64_t tbuf[2];
    sys_times(tbuf);
    uint64_t hz = tbuf[1] ? tbuf[1] : 100;
    uint64_t secs = tbuf[0] / hz;

    uint64_t d = secs / 86400;
    uint64_t h = (secs % 86400) / 3600;
    uint64_t m = (secs % 3600) / 60;
    uint64_t s = secs % 60;

    print(" up ");
    if (d) { print_num(d); print(" day"); if (d != 1) print("s"); print(", "); }
    print_num(h); print(":");
    if (m < 10) print("0"); print_num(m);
    print(":");
    if (s < 10) print("0"); print_num(s);
    print("\n");
    sb_terminate(0);
}