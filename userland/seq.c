#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

static void print_uint(uint64_t val) {
    char buf[24]; int idx = 0;
    if (val == 0) buf[idx++] = '0';
    uint64_t n = val;
    while (n > 0) { buf[idx++] = '0' + (n % 10); n /= 10; }
    for (int i = idx - 1; i >= 0; i--) sb_push(1, &buf[i], 1);
}

void _start(void) {
    print("Enter a number for sequence generation: ");
    char buf[64];
    int len = 0;
    while (len < 63) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') { print("\n"); break; }
        if (c == '\b' || c == 127) {
            if (len > 0) { print("\b \b"); len--; }
        } else {
            sb_push(1, &c, 1);
            buf[len++] = c;
        }
    }
    buf[len] = '\0';
    
    uint64_t num = 0;
    for (int i=0; i<len; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            num = num * 10 + (buf[i] - '0');
        }
    }
    
    if (num == 0 || num > 100000) {
        print("Invalid number (max 100000).\n");
        sb_terminate(1);
    }
    
    for (uint64_t i = 1; i <= num; i++) {
        print_uint(i);
        print("\n");
    }
    
    sb_terminate(0);
}
