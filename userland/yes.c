#include "sys.h"

void _start(int argc, char **argv) __attribute__((noreturn));
int main(int argc, char **argv);

int main(int argc, char **argv) {
    const char *word = "y";
    if (argc > 1 && argv[1] && *argv[1]) word = argv[1];
    uint64_t len = strlen(word);
    for (;;) {
        sb_push(1, word, len);
        sb_push(1, "\n", 1);
    }
}

void _start(int argc, char **argv) {
    sb_terminate(main(argc, argv));
    for (;;) {}
}
