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
    print("Enter filename for wc: ");
    char filename[128];
    int len = 0;
    while (len < 127) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') { print("\n"); break; }
        if (c == '\b' || c == 127) {
            if (len > 0) { print("\b \b"); len--; }
        } else {
            sb_push(1, &c, 1);
            filename[len++] = c;
        }
    }
    filename[len] = '\0';
    if (len == 0) sb_terminate(1);

    int fd = sb_acquire(filename, 0);
    if (fd < 0) { print("wc: "); print(filename); print(": No such file\n"); sb_terminate(1); }

    char buf[512];
    uint64_t bytes = 0;
    uint64_t words = 0;
    uint64_t lines = 0;
    int in_word = 0;
    
    while (1) {
        int r = sb_pull(fd, buf, 512);
        if (r <= 0) break;
        for (int i = 0; i < r; i++) {
            bytes++;
            char c = buf[i];
            if (c == '\n') lines++;
            if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
                in_word = 0;
            } else {
                if (!in_word) {
                    words++;
                    in_word = 1;
                }
            }
        }
    }
    sb_release(fd);
    
    print(" Lines: "); print_uint(lines);
    print(" Words: "); print_uint(words);
    print(" Bytes: "); print_uint(bytes);
    print(" "); print(filename); print("\n");
    
    sb_terminate(0);
}
