#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }
static void print_uint(uint64_t val) {
    char buf[24]; int idx = 0;
    if (val == 0) buf[idx++] = '0';
    uint64_t n = val;
    while (n > 0) { buf[idx++] = '0' + (n % 10); n /= 10; }
    for (int i = idx - 1; i >= 0; i--) sb_push(1, &buf[i], 1);
}

static int read_line(const char *prompt, char *buf, int max_len) {
    print(prompt);
    int len = 0;
    while (len < max_len - 1) {
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
    return len;
}

void _start(void) {
    char file1[64];
    char file2[64];
    
    if (read_line("File 1: ", file1, 64) == 0) sb_terminate(1);
    if (read_line("File 2: ", file2, 64) == 0) sb_terminate(1);
    
    int fd1 = sb_acquire(file1, 0);
    if (fd1 < 0) { print("cmp: cannot open file 1\n"); sb_terminate(1); }
    
    int fd2 = sb_acquire(file2, 0);
    if (fd2 < 0) { print("cmp: cannot open file 2\n"); sb_release(fd1); sb_terminate(1); }
    
    char b1, b2;
    int line = 1;
    int byte = 1;
    
    while (1) {
        int r1 = sb_pull(fd1, &b1, 1);
        int r2 = sb_pull(fd2, &b2, 1);
        
        if (r1 <= 0 && r2 <= 0) {
            print("Files are identical.\n");
            break;
        } else if (r1 <= 0) {
            print("cmp: EOF on "); print(file1); print("\n");
            break;
        } else if (r2 <= 0) {
            print("cmp: EOF on "); print(file2); print("\n");
            break;
        }
        
        if (b1 != b2) {
            print(file1); print(" "); print(file2); print(" differ: byte "); print_uint(byte); print(", line "); print_uint(line); print("\n");
            break;
        }
        
        if (b1 == '\n') line++;
        byte++;
    }
    
    sb_release(fd1);
    sb_release(fd2);
    sb_terminate(0);
}
