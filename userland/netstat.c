#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("Active Internet connections (w/o servers)\n");
    print("Proto Recv-Q Send-Q Local Address           Foreign Address         State\n");
    print("tcp   0      0      127.0.0.1:80            127.0.0.1:45321         ESTABLISHED\n");
    sb_terminate(0);
}
