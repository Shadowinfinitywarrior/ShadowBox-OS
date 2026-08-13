#include "sys.h"

/* tee - read stdin, write to stdout and to each file argument */
#define MAX_FILES 8

int main(int argc, char **argv) {
    int fds[MAX_FILES];
    int nfiles = 0;

    for (int i = 1; i < argc && nfiles < MAX_FILES; i++) {
        int fd = sb_acquire(argv[i], 0x1 | 0x40 | 0x200); /* O_WRONLY | O_CREAT | O_TRUNC */
        if (fd < 0) {
            sb_push(1, "tee: cannot open ", 17);
            sb_push(1, argv[i], strlen(argv[i]));
            sb_push(1, "\n", 1);
            continue;
        }
        fds[nfiles++] = fd;
    }

    char buf[512];
    for (;;) {
        int r = sb_pull(0, buf, 512);
        if (r <= 0) break;
        sb_push(1, buf, r);
        for (int i = 0; i < nfiles; i++) sb_push(fds[i], buf, r);
    }

    for (int i = 0; i < nfiles; i++) sb_release(fds[i]);
    sb_terminate(0);
    return 0;
}
