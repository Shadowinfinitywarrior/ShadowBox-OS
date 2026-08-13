#include "sys.h"
void _start(void) {
    uint64_t t1 = sys_times(0);
    uint64_t i = 0;
    while(i < 10000000) i++;
    uint64_t t2 = sys_times(0);
    if (t1 == t2) sb_push(1, "TIME_FROZEN\n", 12);
    else sb_push(1, "TIME_OK\n", 8);
    sb_terminate(0);
}
