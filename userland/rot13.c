#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("Enter text to ROT13: ");
    char buf[512];
    int len = 0;
    while (len < 511) {
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
    
    print("ROT13: ");
    for (int i = 0; i < len; i++) {
        char c = buf[i];
        if (c >= 'a' && c <= 'z') {
            c = ((c - 'a' + 13) % 26) + 'a';
        } else if (c >= 'A' && c <= 'Z') {
            c = ((c - 'A' + 13) % 26) + 'A';
        }
        sb_push(1, &c, 1);
    }
    print("\n");
    sb_terminate(0);
}
