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
    uint64_t tbuf[2]; sys_times(tbuf);
    uint64_t seed = tbuf[0];
    int target = (seed % 100) + 1;
    int attempts = 0;
    
    print("=== Guess the Number (1-100) ===\n");
    while (1) {
        print("Your guess: ");
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
        if (len == 0) continue;
        
        int guess = 0;
        for (int i=0; i<len; i++) {
            if (buf[i] >= '0' && buf[i] <= '9') {
                guess = guess * 10 + (buf[i] - '0');
            }
        }
        
        attempts++;
        if (guess < target) print("Too low!\n");
        else if (guess > target) print("Too high!\n");
        else {
            print("Correct! You guessed it in ");
            print_uint(attempts);
            print(" attempts.\n");
            break;
        }
    }
    sb_terminate(0);
}
