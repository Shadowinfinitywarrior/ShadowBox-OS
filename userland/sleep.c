#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("Enter seconds to sleep: ");
    char buf[16];
    int len = 0;
    while (len < 15) {
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
    
    int secs = 0;
    for (int i=0; i<len; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            secs = secs * 10 + (buf[i] - '0');
        }
    }
    
    if (secs > 0) {
        uint64_t tbuf[2];
        sys_times(tbuf);
        uint64_t hz = tbuf[1]; if (!hz) hz = 100;
        uint64_t end = tbuf[0] + (secs * hz);
        
        while (1) {
            sys_times(tbuf);
            if (tbuf[0] >= end) break;
            for(volatile int i=0; i<10000; i++);
        }
    }
    sb_terminate(0);
}
