#include "sys.h"

void _start(void) {
    sb_push(1, "[Input Service] Started\n", 24);
    while (1) {
        // simple idle loop, yielding CPU
        syscall1(SYS_SCHED_YIELD, 0);
    }
}
