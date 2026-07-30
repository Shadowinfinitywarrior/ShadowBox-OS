#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

static uint64_t rand_state = 12345;
static uint32_t rand(void) {
    rand_state = rand_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return rand_state >> 32;
}

void _start(void) {
    uint64_t tbuf[2]; sys_times(tbuf);
    rand_state = tbuf[0];

    print("\033[2J\033[H"); // Clear screen
    print("\033[32m");      // Green text
    
    int cols = 80;
    int drops[80];
    for (int i=0; i<cols; i++) drops[i] = -(rand() % 50);

    for (int frame = 0; frame < 200; frame++) { 
        for (int i=0; i<cols; i++) {
            if (drops[i] > 0 && drops[i] < 24) {
                int y = drops[i];
                int x = i + 1;
                char ybuf[8], xbuf[8];
                int yi = 0, xi = 0;
                if (y == 0) ybuf[yi++] = '0';
                else { int ty = y; while(ty > 0) { ybuf[yi++] = '0' + (ty % 10); ty /= 10; } }
                if (x == 0) xbuf[xi++] = '0';
                else { int tx = x; while(tx > 0) { xbuf[xi++] = '0' + (tx % 10); tx /= 10; } }
                
                print("\033[");
                for (int j = yi - 1; j >= 0; j--) sb_push(1, &ybuf[j], 1);
                print(";");
                for (int j = xi - 1; j >= 0; j--) sb_push(1, &xbuf[j], 1);
                print("H");
                
                char c = 33 + (rand() % 94); 
                sb_push(1, &c, 1);
            }
            drops[i]++;
            if (drops[i] > 24 && (rand() % 10 == 0)) {
                drops[i] = 0;
            }
        }
        for (volatile int delay=0; delay<500000; delay++);
    }
    
    print("\033[0m\033[2J\033[H");
    sb_terminate(0);
}
