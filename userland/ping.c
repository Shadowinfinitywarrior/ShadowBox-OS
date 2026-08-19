#include "sys.h"

/* ping - send ICMP echo requests and report round-trip times */

static void print(const char *s) { sb_push(1, s, strlen(s)); }
static void print_num(uint64_t v) {
    char buf[24]; int idx = 0;
    if (v == 0) buf[idx++] = '0';
    while (v > 0) { buf[idx++] = '0' + (v % 10); v /= 10; }
    while (idx > 0) sb_push(1, &buf[--idx], 1);
}
static void print_ip(uint32_t ip) {
    print_num(ip >> 24); print(".");
    print_num((ip >> 16) & 0xFF); print(".");
    print_num((ip >> 8) & 0xFF); print(".");
    print_num(ip & 0xFF);
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

static int atoi(const char *s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print("Usage: ping <host|ip> [count]\n");
        sb_terminate(1);
    }
    uint32_t ip = 0;
    if (sb_resolve(argv[1], &ip) != 0) {
        print("ping: cannot resolve ");
        print(argv[1]);
        print("\n");
        sb_terminate(1);
    }
    int count = (argc >= 3) ? atoi(argv[2]) : 4;
    if (count <= 0) count = 4;
    if (count > 10) count = 10;

    print("PING ");
    print(argv[1]);
    print(" (");
    print_ip(ip);
    print(")\n");

    int ok = 0;
    for (int i = 0; i < count; i++) {
        int rtt = sb_ping(ip, 2000);
        if (rtt >= 0) {
            print("reply from ");
            print_ip(ip);
            print(": time=");
            print_num((uint64_t)rtt);
            print(" ms\n");
            ok++;
        } else {
            print("timeout\n");
        }
    }
    print("--- ping statistics ---\n");
    print("sent="); print_num(count);
    print(" received="); print_num(ok);
    print(" lost="); print_num(count - ok);
    print("\n");
    sb_terminate(ok ? 0 : 1);
}