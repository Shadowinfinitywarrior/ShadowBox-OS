#include "sys.h"

/* sort - sort lines from stdin (or a file) in byte order */

static int line_less(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a < (unsigned char)*b;
}

int main(int argc, char **argv) {
    int fd = 0;
    if (argc > 1) {
        fd = sb_acquire(argv[1], 0);
        if (fd < 0) {
            sb_push(1, "sort: cannot open ", 18);
            sb_push(1, argv[1], strlen(argv[1]));
            sb_push(1, "\n", 1);
            sb_terminate(1);
        }
    }

    const uint64_t CHUNK = 65536;
    char *data = (char *)sys_sbrk(CHUNK);
    uint64_t cap = CHUNK, used = 0;
    for (;;) {
        if (used + 512 > cap) {
            sys_sbrk(CHUNK);
            cap += CHUNK;
        }
        int r = sb_pull(fd, data + used, 512);
        if (r <= 0) break;
        used += r;
    }
    if (fd != 0) sb_release(fd);
    if (used == 0) { sb_terminate(0); }

    /* Split into lines */
    const uint64_t MAX_LINES = 8192;
    char **lines = (char **)sys_sbrk(MAX_LINES * sizeof(char *));
    uint64_t n = 0;
    lines[n++] = data;
    for (uint64_t i = 0; i < used && n < MAX_LINES; i++) {
        if (data[i] == '\n') {
            data[i] = '\0';
            if (i + 1 < used) lines[n++] = &data[i + 1];
        }
    }

    /* Insertion sort (byte order) */
    for (uint64_t i = 1; i < n; i++) {
        char *key = lines[i];
        uint64_t j = i;
        while (j > 0 && line_less(key, lines[j - 1])) {
            lines[j] = lines[j - 1];
            j--;
        }
        lines[j] = key;
    }

    for (uint64_t i = 0; i < n; i++) {
        sb_push(1, lines[i], strlen(lines[i]));
        sb_push(1, "\n", 1);
    }
    sb_terminate(0);
    return 0;
}
