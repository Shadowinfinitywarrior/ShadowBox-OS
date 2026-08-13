#include "sys.h"

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

static void print_uint(uint64_t val) {
    char buf[24]; int idx = 0;
    if (val == 0) buf[idx++] = '0';
    uint64_t n = val;
    while (n > 0) { buf[idx++] = '0' + (n % 10); n /= 10; }
    for (int i = idx - 1; i >= 0; i--) sb_push(1, &buf[i], 1);
}

void _start(void) {
    print("=== ShadowBox Calculator ===\n");
    print("Enter expression (e.g., 5 + 3), or type 'q' to quit.\n");
    
    while (1) {
        print("> ");
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
        
        if (buf[0] == 'q' || buf[0] == 'Q') break;
        if (len == 0) continue;
        
        uint64_t a = 0, b = 0;
        char op = 0;
        int i = 0;
        while (buf[i] == ' ') i++;
        while (buf[i] >= '0' && buf[i] <= '9') {
            a = a * 10 + (buf[i] - '0');
            i++;
        }
        while (buf[i] == ' ') i++;
        op = buf[i++];
        while (buf[i] == ' ') i++;
        while (buf[i] >= '0' && buf[i] <= '9') {
            b = b * 10 + (buf[i] - '0');
            i++;
        }
        
        uint64_t res = 0;
        if (op == '+') res = a + b;
        else if (op == '-') res = a - b;
        else if (op == '*') res = a * b;
        else if (op == '/') {
            if (b != 0) res = a / b;
            else { print("Div by zero!\n"); continue; }
        } else {
            print("Unknown operator.\n");
            continue;
        }
        
        print("= "); print_uint(res); print("\n");
    }
    sb_terminate(0);
}