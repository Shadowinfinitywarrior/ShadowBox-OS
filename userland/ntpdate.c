#include "sys.h"

/* ntpdate - sync the system clock against an NTP server (default QEMU user-net) */

static void print(const char *s) { sb_push(1, s, strlen(s)); }
static void print_num(uint64_t v) {
    char buf[24]; int idx = 0;
    if (v == 0) buf[idx++] = '0';
    while (v > 0) { buf[idx++] = '0' + (v % 10); v /= 10; }
    while (idx > 0) sb_push(1, &buf[--idx], 1);
}

static uint32_t parse_ip(const char *s) {
    uint32_t ip = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t part = 0;
        while (*s && *s != '.') { part = part * 10 + (*s - '0'); s++; }
        ip = (ip << 8) | (part & 0xFF);
        if (*s == '.') s++;
    }
    return ip;
}

static void print_ip(uint32_t ip) {
    print_num(ip >> 24); print(".");
    print_num((ip >> 16) & 0xFF); print(".");
    print_num((ip >> 8) & 0xFF); print(".");
    print_num(ip & 0xFF);
}

int main(int argc, char **argv) {
    uint32_t server = 0x0A000203; /* 10.0.2.3 */
    if (argc >= 2) server = parse_ip(argv[1]);

    print("ntpdate: syncing with ");
    print_ip(server);
    print(" ...\n");

    int64_t offset = 0;
    int rc = sb_ntp_sync(server, &offset);
    if (rc != 0) {
        print("ntpdate: sync failed (no network or timeout)\n");
        sb_terminate(1);
    }
    print("ntpdate: clock adjusted by ");
    if (offset >= 0) print("+");
    print_num((uint64_t)(offset < 0 ? -offset : offset));
    print(" seconds\n");
    sb_terminate(0);
}