#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("Enter filename for strings extraction: ");
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
    if (fd < 0) { print("File not found.\n"); sb_terminate(1); }

    char buf[512];
    char str[512];
    int slen = 0;
    
    while (1) {
        int r = sb_pull(fd, buf, 512);
        if (r <= 0) break;
        for (int i = 0; i < r; i++) {
            char c = buf[i];
            if (c >= 32 && c <= 126) {
                if (slen < 511) str[slen++] = c;
            } else {
                if (slen >= 4) {
                    str[slen] = '\n';
                    sb_push(1, str, slen + 1);
                }
                slen = 0;
            }
        }
    }
    if (slen >= 4) {
        str[slen] = '\n';
        sb_push(1, str, slen + 1);
    }
    sb_release(fd);
    sb_terminate(0);
}
