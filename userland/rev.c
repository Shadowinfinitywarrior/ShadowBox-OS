#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("Enter text to reverse: ");
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
    
    print("Reversed: ");
    for (int i = len - 1; i >= 0; i--) {
        sb_push(1, &buf[i], 1);
    }
    print("\n");
    
    sb_terminate(0);
}
