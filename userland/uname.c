#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("ShadowBoxOS darkdevil404 0.3.0 #1 SMP x86_64\n");
    sb_terminate(0);
}
