#include "sys.h"

void _start(void) {
    sb_push(1, "\033[2J\033[H", 7);
    sb_terminate(0);
}
