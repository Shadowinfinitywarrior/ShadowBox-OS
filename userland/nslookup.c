#include "sys.h"

/* nslookup - resolve a hostname to an IPv4 address via the kernel DNS client */

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

int main(int argc, char **argv) {
    if (argc < 2) {
        print("Usage: nslookup <hostname>\n");
        sb_terminate(1);
    }
    uint32_t ip = 0;
    int rc = sb_resolve(argv[1], &ip);
    if (rc != 0) {
        print("nslookup: no address found for ");
        print(argv[1]);
        print("\n");
        sb_terminate(1);
    }
    print(argv[1]);
    print(" resolves to ");
    print_ip(ip);
    print("\n");
    sb_terminate(0);
}