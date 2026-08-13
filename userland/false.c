#include "sys.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 1;
}

void _start(int argc, char **argv) __attribute__((noreturn));
void _start(int argc, char **argv) {
    sb_terminate(main(argc, argv));
    for (;;) {}
}
