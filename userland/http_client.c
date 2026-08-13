// HTTP client for ShadowBox OS
// Simple client that performs a GET request to a given IPv4 address and path.
// Usage: http_client <IP> <path>
// Example: http_client 93.184.216.34 /index.html

#include "sys.h"
#include "socket.h"

// Helper: print a string to stdout using sb_push
static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

// Helper: print integer in decimal
static void print_uint(uint64_t val) {
    char buf[32];
    int idx = 0;
    if (val == 0) {
        buf[idx++] = '0';
    } else {
        while (val > 0 && idx < 31) {
            buf[idx++] = '0' + (val % 10);
            val /= 10;
        }
    }
    // reverse
    for (int i = idx - 1; i >= 0; i--) {
        sb_push(1, &buf[i], 1);
    }
}

static uint16_t htons(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static uint32_t htonl(uint32_t x) {
    return ((x >> 24) & 0xFF) |
           ((x >> 8)  & 0xFF00) |
           ((x << 8)  & 0xFF0000) |
           ((x << 24) & 0xFF000000);
}

// Simple dotted‑decimal IPv4 parser (no error handling)
static uint32_t parse_ip(const char *s) {
    uint32_t ip = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t part = 0;
        while (*s && *s != '.') {
            part = part * 10 + (*s - '0');
            s++;
        }
        ip = (ip << 8) | (part & 0xFF);
        if (*s == '.') s++;
    }
    return ip;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print("Usage: http_client <IP> <path>\n");
        sb_terminate(1);
        return 1;
    }

    const char *ip_str = argv[1];
    const char *path = argv[2];

    uint32_t ip_host = parse_ip(ip_str);
    uint32_t ip_net = htonl(ip_host);
    uint16_t port = htons(80);

    // Minimal sockaddr_in compatible structure
    struct sockaddr_in {
        uint16_t sin_family;
        uint16_t sin_port;
        uint32_t sin_addr;
    } addr;
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr = ip_net;

    // Create TCP socket (AF_INET, SOCK_STREAM)
    int fd = (int)syscall3(SB_SOCKET_CREATE, AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        print("Error: socket creation failed\n");
        sb_terminate(1);
    }

    // Connect to remote host
    int ret = (int)syscall3(SB_SOCKET_CONNECT, fd, (uint64_t)&addr, sizeof(addr));
    if (ret < 0) {
        print("Error: connect failed\n");
        sb_release(fd);
        sb_terminate(1);
    }

    // Build HTTP GET request
    char request[256];
    int len = 0;
    // Simple sprintf implementation using manual copy (no stdio)
    const char *fmt = "GET ";
    while (fmt[len]) { request[len] = fmt[len]; len++; }
    const char *p = path;
    while (*p) { request[len++] = *p++; }
    const char *fmt2 = " HTTP/1.0\r\nHost: ";
    p = fmt2;
    while (*p) { request[len++] = *p++; }
    p = ip_str;
    while (*p) { request[len++] = *p++; }
    const char *fmt3 = "\r\n\r\n";
    p = fmt3;
    while (*p) { request[len++] = *p++; }

    // Send request
    sb_push(fd, request, len);

    // Receive response (single read, up to 4KB)
    char resp[4096];
    uint64_t got = sb_pull(fd, resp, sizeof(resp) - 1);
    if (got > 0) {
        resp[got] = 0; // null‑terminate for safety
        sb_push(1, resp, got);
    }

    // Clean up
    sb_release(fd);
    sb_terminate(0);
    return 0;
}
