#include "sys.h"

void _start(void) {
    sb_push(1, "[Desktop Service] Started\n", 27);
    while (1) {
        syscall1(SYS_SCHED_YIELD, 0);
    }
}
